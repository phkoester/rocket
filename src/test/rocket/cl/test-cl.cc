/*
 * test-cl.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/cl/cl.h"
#include "rocket/log/log.h"
#include "rocket/str/str.h"

using namespace rocket::cl;
using namespace rocket::unicode;

// Local functions ------------------------------------------------------------------------------------------

namespace {

bool parseCommandOmit; // CL 1 "-o"
bool parseCommandHelp; // CL 1 "-h"
string parseCommandCommand; // CL 1 command
bool parseCommandList; // CL 2 "-l"
bool parseCommandListHelp; // CL 2 "-h"
bool parseCommandShow; // CL 2 "-s"
bool parseCommandShowHelp; // CL 2 "-h"
bool parseCommandShowTest; // CL 2 "-t"
vector<string> parseCommandArgs; // CL 2 args

vector<string>
parse(const CommandLine& cl, const vector<string>& args, nio::Sink& err = nio::stderr) {
  try {
    return cl.parse(args);
  } catch (const exception& ex) {
    cl.handleException(ex, err, EXIT_SUCCESS);
    return {};
  }
}

/**
 * Usage: cmd [-o | -h] list [-h | -l] FILE...
 *   or   cmd [-o | -h] show [-h | -s | -t] [ARG]...
 */
void
parseCommand(const vector<string>& args, nio::Sink& out = nio::stdout, nio::Sink& err = nio::stderr) {
  // Reset

  parseCommandOmit = false;
  parseCommandHelp = false;
  parseCommandCommand.clear();
  parseCommandList = false;
  parseCommandListHelp = false;
  parseCommandShow = false;
  parseCommandShowHelp = false;
  parseCommandShowTest = false;
  parseCommandArgs.clear();

  // Parse, step 1

  OptionGroup general { "General control" };
  OptionGroup misc { "Miscellaneous" };

  CommandLineParams params {
    .usages={ "[OPTION]... list [OPTION]... FILE...", "[OPTION]... show [OPTION]... [ARG]..." },
    .prolog="List FILEs or show ARGs.\n\nThis is yet another paragraph.",
    .epilog="Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur. Hi omnes lingua, institutis, legibus inter se differunt. Gallos ab Aquitanis Garunna flumen, a Belgis Matrona et Sequana dividit."
  };

  CommandLine cl({
    Option::of(&general, "omit", "o"_cv, nullopt, "omit what is not important", parseCommandOmit),
    Option::of(&misc, "help", "h"_cv, nullopt, "display this help text", parseCommandHelp)
  }, params);

  auto take = [](string_view arg) -> CommandLine::Took {
    if (arg == "list" || arg == "show") {
      parseCommandCommand = arg;
      return CommandLine::Stop;
    }
    ROCKET_FAIL("Invalid command `{}`", arg);
  };

  vector<string> localArgs;
  try {
    localArgs = cl.parse(args, take);
    if (parseCommandHelp) {
      cl.help(out, false);
      return;
    }
  } catch (const exception& ex) {
    cl.handleException(ex, err, EXIT_SUCCESS);
    return;
  }

  // Parse, step 2

  if (parseCommandCommand == "list") {
    // 1. "list" with "FILE..." (mandatory)

    OptionGroup list { "List control" };

    CommandLineParams listParams {
      .command = params.command + " list",
      .usages={ "[OPTION]... FILE..." },
      .rocketOpts=false
    };

    CommandLine listCl({
      Option::of(&list, "help", "h"_cv, nullopt, "display this help text", parseCommandListHelp),
      Option::of(&list, "list", "l"_cv, nullopt, "a list option that is good for nothing", parseCommandList)
    }, listParams);

    try {
      parseCommandArgs = listCl.parse(localArgs);
      if (parseCommandListHelp) {
        listCl.help(out, false);
        return;
      }
    } catch (const exception& ex) {
      listCl.handleException(ex, err, EXIT_SUCCESS);
      return;
    }

    if (parseCommandArgs.empty()) {
      listCl.error(err, EXIT_SUCCESS);
      return;
    }

    out.writeln("Listing ...");
  } else {
    // 2. "show" with "[ARG]..." (optional)

    OptionGroup show { "Show control" };

    CommandLineParams showParams {
      .command = params.command + " show",
      .usages={ "[OPTION]... [ARG]..." },
      .rocketOpts=false
    };

    CommandLine showCl({
      Option::of(&show, "help", "h"_cv, nullopt, "display this help text", parseCommandShowHelp),
      Option::of(&show, "show", "s"_cv, nullopt, "a show option that is good for nothing", parseCommandShow),
      Option::of(&show, "test", "t"_cv, nullopt, "test something, or don't", parseCommandShowTest)
    }, showParams);

    try {
      parseCommandArgs = showCl.parse(localArgs);
      if (parseCommandShowHelp) {
        showCl.help(out, false);
        return;
      }
    } catch (const exception& ex) {
      showCl.handleException(ex, err, EXIT_SUCCESS);
      return;
    }

    out.writeln("Showing ...");
  }
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(cl, parseNoOpts) {
  CommandLine cl;
  nio::StringSink buf;
  auto args = parse(cl, { "a", "b", "c" }, buf);
  EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
}

TEST(cl, parseOptBool) {
  bool flag;

  CommandLine cl( {
    // 🧑‍🌾: U+1F9D1 (ADULT), U+200D (ZERO WIDTH JOINER), U+1F33E (EAR OF RICE)
    Option::of(nullptr, "flag", "🧑‍🌾"_cv, nullopt, nullopt, flag)
  });

  // Test no options
  {
    flag = false;
    auto args = parse(cl, { "a", "b", "c" });
    EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
    EXPECT_FALSE(flag);
  }

  // Test mixed order
  {
    flag = false;
    auto args = parse(cl, { "a", "--flag", "b", "c" });
    EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
    EXPECT_TRUE(flag);
  }

  // Test Unicode code point
  {
    flag = false;
    auto args = parse(cl, { "a", "-🧑‍🌾", "b", "c" });
    EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
    EXPECT_TRUE(flag);
  }

  // Test option-end tag
  {
    flag = false;
    auto args = parse(cl, { "a", "--", "--flag", "b", "c" });
    EXPECT_EQ(args, (vector<string> { "a", "--flag", "b", "c" }));
    EXPECT_FALSE(flag);
  }

  // Test assignment for flag option, by name
  {
    flag = true;
    auto args = parse(cl, { "a", "--flag=false", "b", "c" });
    EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
    EXPECT_FALSE(flag);
  }

  // Test assignment for flag option, by short name
  {
    flag = true;
    auto args = parse(cl, { "a", "-🧑‍🌾=0", "b", "c" });
    EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
    EXPECT_FALSE(flag);
  }

  // Test error when assigning other value
  {
    flag = false;
    nio::StringSink buf;
    auto args = parse(cl, { "a", "-🧑‍🌾=hello", "b", "c" }, buf);
    EXPECT_EQ(args, vector<string>());
    EXPECT_FALSE(flag);
    EXPECT_EQ(buf.str(), "test-rocket-cl: error: Option `-🧑‍🌾` cannot take a value\n");
  }
}

TEST(cl, parseOptInt) {
  i32 num;

  CommandLine cl( {
    Option::of(nullptr, "num", "n"_cv, "NUM", nullopt, num)
  });

  // Test mixed order
  {
    num = 0;
    auto args = parse(cl, { "a", "--num", "12", "b", "c" });
    EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
    EXPECT_EQ(num, 12);
  }

  // Test assignment via '='
  {
    num = 0;
    auto args = parse(cl, { "a", "--num=12", "b", "c" });
    EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
    EXPECT_EQ(num, 12);
  }

  // Test error when missing value
  {
    num = 0;
    nio::StringSink buf;
    auto args = parse(cl, { "a", "-n" }, buf);
    EXPECT_EQ(args, vector<string>());
    EXPECT_EQ(num, 0);
    EXPECT_EQ(buf.str(), "test-rocket-cl: error: Missing value for option `-n`\n");
  }

  // Test error when conversion fails
  {
    num = 0;
    nio::StringSink buf;
    auto args = parse(cl, { "a", "-n", "hello" }, buf);
    EXPECT_EQ(args, vector<string>());
    EXPECT_EQ(num, 0);
    EXPECT_EQ(buf.str(), "test-rocket-cl: error: Option `-n`: Cannot scan \"hello\" as `int`; expected NUM\n");
  }
}

TEST(cl, parseOptEnum) {
  log::LogLevel level;

  CommandLine cl( {
    Option::of(nullptr, "level", "l"_cv, "LEVEL", nullopt, level)
  });

  // Test mixed order
  {
    level = log::LogLevel::none;
    auto args = parse(cl, { "a", "--level", "info", "b", "c" });
    EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
    EXPECT_EQ(level, log::LogLevel::info);
  }

  // Test error when conversion fails
  {
    level = log::LogLevel::none;
    nio::StringSink buf;
    auto args = parse(cl, { "a", "-l", "nonsense" }, buf);
    EXPECT_EQ(args, vector<string>());
    EXPECT_EQ(level, log::LogLevel::none);
    EXPECT_EQ(buf.str(), "test-rocket-cl: error: Option `-l`: Cannot scan \"nonsense\" as `rocket::log::LogLevel`; expected LEVEL\n");
  }
}

TEST(cl, parseOptVector) {
  vector<string> names;

  CommandLine cl( {
    Option::of(nullptr, "name", "n"_cv, "NAME", nullopt, names)
  });

  // Test multiple values
  {
    names.clear();
    auto args = parse(cl, { "a", "--name", "Shirley", "-n", "Deborah", "--name=Julie", "-n=Jane", "b" });
    EXPECT_EQ(args, (vector<string> { "a", "b" }));
    EXPECT_EQ(names, (vector<string> { "Shirley", "Deborah", "Julie", "Jane" }));
  }
}

TEST(cl, parseShortOptions) {
  bool ignore;
  bool verbose;
  string name;

  CommandLine cl( {
    Option::of(nullptr, "ignore", "i"_cv, nullopt, nullopt, ignore),
    Option::of(nullptr, "verbose", "v"_cv, nullopt, nullopt, verbose),
    Option::of(nullptr, "name", "n"_cv, "NAME", nullopt, name)
  });

  // Test without '='
  ignore = false; verbose = false; name.clear();
  auto args = parse(cl, { "a", "-ivnSue", "b" });
  EXPECT_EQ(args, (vector<string> { "a", "b" }));
  EXPECT_TRUE(ignore);
  EXPECT_TRUE(verbose);
  EXPECT_EQ(name, "Sue");

  // Test with '='
  ignore = false; verbose = false; name.clear();
  args = parse(cl, { "a", "-ivn=Sue", "b" });
  EXPECT_EQ(args, (vector<string> { "a", "b" }));
  EXPECT_TRUE(ignore);
  EXPECT_TRUE(verbose);
  EXPECT_EQ(name, "Sue");

  // Test with ' '
  ignore = false; verbose = false; name.clear();
  args = parse(cl, { "a", "-ivn", "Sue", "b" });
  EXPECT_EQ(args, (vector<string> { "a", "b" }));
  EXPECT_TRUE(ignore);
  EXPECT_TRUE(verbose);
  EXPECT_EQ(name, "Sue");
}

/**
 * Usage: cmd [-o | -h] list [-h | -l] FILE...
 *   or   cmd [-o | -h] show [-h | -s | -t] [ARG]...
 */
TEST(cl, parseCommand) {
  // Test invalid option
  {
    nio::StringSink buf;
    parseCommand({ "-p", "list", "a" }, buf, buf);
    EXPECT_EQ(
        buf.str(),
        "test-rocket-cl: error: Unknown option `-p`\n"
        "Usage: test-rocket-cl [OPTION]... list [OPTION]... FILE...\n"
        "  or   test-rocket-cl [OPTION]... show [OPTION]... [ARG]...\n"
        "Try `test-rocket-cl --help` for more information.\n");
  }

  // Test invalid command
  {
    nio::StringSink buf;
    parseCommand({ "-o", "walk", "dog" }, buf, buf);
    EXPECT_EQ(
        buf.str(),
        "test-rocket-cl: error: Invalid command `walk`\n"
        "Usage: test-rocket-cl [OPTION]... list [OPTION]... FILE...\n"
        "  or   test-rocket-cl [OPTION]... show [OPTION]... [ARG]...\n"
        "Try `test-rocket-cl --help` for more information.\n");
  }

  // Test help
  {
    nio::StringSink buf;
    parseCommand({ "--help" }, buf, buf);
    string str = buf.str();
    str::replaceIn<char>(str, "\n", "~");

    EXPECT_THAT(str, matchesRegex(
        "Usage: test-rocket-cl \\[OPTION\\]\\.\\.\\. list \\[OPTION\\]\\.\\.\\. FILE\\.\\.\\.~"
        "  or   test-rocket-cl \\[OPTION\\]\\.\\.\\. show \\[OPTION\\]\\.\\.\\. \\[ARG\\]\\.\\.\\.~"
        "~"
        "List FILEs or show ARGs\\.~"
        "~"
        "This is yet another paragraph\\.~"
        "~"
        "Logging control:~"
        "~"
        ".*"
        "~"
        "General control:~"
        "~"
        "  -o, --omit~"
        "          omit what is not important~"
        "~"
        "Miscellaneous:~"
        "~"
        "  -h, --help~"
        "          display this help text~"
        "~"
        "Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam~"
        "Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur. Hi omnes~"
        "lingua, institutis, legibus inter se differunt. Gallos ab Aquitanis Garunna~"
        "flumen, a Belgis Matrona et Sequana dividit.~"
      ));
  }

  // Test list help
  {
    nio::StringSink buf;
    parseCommand({ "list", "--help" }, buf, buf);
    EXPECT_EQ(
      buf.str(),
      "Usage: test-rocket-cl list [OPTION]... FILE...\n"
      "\n"
      "List control:\n"
      "\n"
      "  -h, --help\n"
      "          display this help text\n"
      "  -l, --list\n"
      "          a list option that is good for nothing\n");
  }

  // Test invalid list option
  {
    nio::StringSink buf;
    parseCommand({ "list", "-Q" }, buf, buf);
    EXPECT_EQ(
        buf.str(),
        "test-rocket-cl: error: Unknown option `-Q`\n"
        "Usage: test-rocket-cl list [OPTION]... FILE...\n"
        "Try `test-rocket-cl list --help` for more information.\n");
  }

  // Test missing FILE
  {
    nio::StringSink buf;
    parseCommand({ "list", "-l" }, buf, buf);
    EXPECT_EQ(
        buf.str(),
        "Usage: test-rocket-cl list [OPTION]... FILE...\n"
        "Try `test-rocket-cl list --help` for more information.\n");
  }

  // Test successful list command
  {
    nio::StringSink buf;
    parseCommand({ "list", "-l", "a", "b" }, buf, buf);
    EXPECT_EQ(parseCommandCommand, "list");
    EXPECT_TRUE(parseCommandList);
    EXPECT_EQ(parseCommandArgs, (vector<string> { "a", "b" }));
    EXPECT_EQ(buf.str(), "Listing ...\n");
  }

  // Test show help
  {
    nio::StringSink buf;
    parseCommand({ "show", "--help" }, buf, buf);
    EXPECT_EQ(
      buf.str(),
      "Usage: test-rocket-cl show [OPTION]... [ARG]...\n"
      "\n"
      "Show control:\n"
      "\n"
      "  -h, --help\n"
      "          display this help text\n"
      "  -s, --show\n"
      "          a show option that is good for nothing\n"
      "  -t, --test\n"
      "          test something, or don't\n");
  }

  // Test invalid show option
  {
    nio::StringSink buf;
    parseCommand({ "show", "-Q" }, buf, buf);
    EXPECT_EQ(
        buf.str(),
        "test-rocket-cl: error: Unknown option `-Q`\n"
        "Usage: test-rocket-cl show [OPTION]... [ARG]...\n"
        "Try `test-rocket-cl show --help` for more information.\n");
  }

  // Test successful show command
  {
    nio::StringSink buf;
    parseCommand({ "show", "a", "-st", "b" }, buf, buf);
    EXPECT_EQ(parseCommandCommand, "show");
    EXPECT_TRUE(parseCommandShow);
    EXPECT_TRUE(parseCommandShowTest);
    EXPECT_EQ(parseCommandArgs, (vector<string> { "a", "b" }));
    EXPECT_EQ(buf.str(), "Showing ...\n");
  }
}

// EOF
