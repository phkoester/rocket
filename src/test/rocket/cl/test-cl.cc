/*
 * test-cl.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/cl/cl.h"
#include "rocket/log/log.h"

using namespace rocket::cl;
using namespace rocket::unicode;

// Local functions ------------------------------------------------------------------------------------------

namespace {

optional<bool> parseCommandOmit; // CL 1 "-o"
optional<bool> parseCommandHelp; // CL 1 "-?"
optional<bool> parseCommandList; // CL 2 "-l"
optional<bool> parseCommandListHelp; // CL 2 "-?"
optional<bool> parseCommandShow; // CL 2 "-s"
optional<bool> parseCommandShowHelp; // CL 2 "-?"
optional<bool> parseCommandShowTest; // CL 2 "-t"

string parseCommandCommand; // CL 1 command
vector<string> parseCommandArgs; // Remaining arguments after the command
vector<string> parseCommandListFiles; // CL 2 files
optional<vector<string>> parseCommandShowArgs; // CL 2 args

/**
 * Usage: cmd [-o | -?] list [-? | -l] FILE...
 *   or   cmd [-o | -?] show [-? | -s | -t] [ARG]...
 */
void
parseCommand(const vector<string>& args, nio::Sink& out = nio::out, nio::Sink& err = nio::err) {
  // Reset

  parseCommandOmit = nullopt;
  parseCommandHelp = nullopt;
  parseCommandList = nullopt;
  parseCommandListHelp = nullopt;
  parseCommandShow = nullopt;
  parseCommandShowHelp = nullopt;
  parseCommandShowTest = nullopt;

  parseCommandCommand.clear();
  parseCommandArgs.clear();

  // Parse, step 1

  OptionGroup general { "General control" };
  OptionGroup misc { "Miscellaneous" };

  CommandLineParams config {
    .usages={ "[OPTION]... list [OPTION]... FILE...", "[OPTION]... show [OPTION]... [ARG]..." },
    .prolog="List FILEs or show ARGs.\n\nThis is yet another paragraph.",
    .epilog="Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur. Hi omnes lingua, institutis, legibus inter se differunt. Gallos ab Aquitanis Garunna flumen, a Belgis Matrona et Sequana dividit."
  };

  auto command = Parameter::of("COMMAND", { "list", "show" }, "`list` or `show`", "a command", parseCommandCommand);
  command.consumeOptions = true;

  CommandLine cl({
    // The group order matters!
    Option::of(&general, "omit", "o"_cv, nullopt, "omit what is not important", parseCommandOmit),
    Option::helpOf(&misc, parseCommandHelp)
  }, {
    command,
    Parameter::of("ARG", "ARG", "command-line argument", parseCommandArgs)
  }, config);

  if (not cl.parse(args, out, err, false)) {
    return;
  }

  // Parse, step 2

  if (parseCommandCommand == "list") {
    // 1. "list" with "FILE..." (mandatory)

    OptionGroup list { "List control" };

    CommandLineParams listParams {
      .command = config.command + " list",
      .usages={ "[OPTION]... FILE..." },
      .rocketOpts=false
    };

    CommandLine listCl({
      Option::helpOf(&list, parseCommandListHelp),
      Option::of(&list, "list", "l"_cv, nullopt, "a list option that is good for nothing", parseCommandList)
    }, {
      Parameter::of("FILE", "FILE", "a file to list", parseCommandListFiles)
    }, listParams);

    if (not listCl.parse(parseCommandArgs, out, err, false)) {
      return;
    }

    out.writeln("Listing ...");
  } else {
    // 2. "show" with "[ARG]..." (optional)

    OptionGroup show { "Show control" };

    CommandLineParams showParams {
      .command = config.command + " show",
      .usages={ "[OPTION]... [ARG]..." },
      .rocketOpts=false
    };

    CommandLine showCl({
      Option::helpOf(&show, parseCommandShowHelp),
      Option::of(&show, "show", "s"_cv, nullopt, "a show option that is good for nothing", parseCommandShow),
      Option::of(&show, "test", "t"_cv, nullopt, "test something, or don't", parseCommandShowTest)
    }, {
      Parameter::of("ARG", "ARG", "a thing to show", parseCommandShowArgs)
    }, showParams);

    if (not showCl.parse(parseCommandArgs, out, err, false)) {
      return;
    }

    out.writeln("Showing ...");
  }
}

} // namespace

// #TEST ----------------------------------------------------------------------------------------------------

TEST(cl, parseNoOpts) {
  vector<string> args;
  CommandLine cl {
    {}, {
      Parameter::of("ARG", "ARG", "command-line argument", args)
    }
  };
  nio::StringSink buf;
  EXPECT_TRUE(cl.parse({ "a", "b", "c" }, buf, buf, false));
  EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
}

TEST(cl, parseOptBool) {
  optional<bool> flag;
  optional<vector<string>> args;

  CommandLine cl( {
    // 🧑‍🌾: U+1F9D1 (ADULT), U+200D (ZERO WIDTH JOINER), U+1F33E (EAR OF RICE)
    Option::of(nullptr, "flag", "🧑‍🌾"_cv, nullopt, nullopt, flag)
  }, {
    Parameter::of("ARG", "ARG", "command-line argument", args)
  });

  // Test no options
  {
    flag = nullopt;
    args = nullopt;
    EXPECT_TRUE(cl.parse({ "a", "b", "c" }, nio::out, nio::err, false));
    EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
    EXPECT_FALSE(flag.value_or(false));
  }

  // Test mixed order
  {
    flag = nullopt;
    args = nullopt;
    EXPECT_TRUE(cl.parse({ "a", "--flag", "b", "c" }, nio::out, nio::err, false));
    EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
    EXPECT_TRUE(flag);
  }

  // Test Unicode code point
  {
    flag = nullopt;
    args = nullopt;
    EXPECT_TRUE(cl.parse({ "a", "-🧑‍🌾", "b", "c" }, nio::out, nio::err, false));
    EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
    EXPECT_TRUE(flag);
  }

  // Test option-end tag
  {
    flag = nullopt;
    args = nullopt;
    EXPECT_TRUE(cl.parse({ "a", "--", "--flag", "b", "c" }, nio::out, nio::err, false));
    EXPECT_EQ(args, (vector<string> { "a", "--flag", "b", "c" }));
    EXPECT_FALSE(flag.value_or(false));
  }

  // Test assignment for flag option, by name
  {
    flag = nullopt;
    args = nullopt;
    EXPECT_TRUE(cl.parse({ "a", "--flag=false", "b", "c" }, nio::out, nio::err, false));
    EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
    EXPECT_FALSE(flag.value_or(false));
  }

  // Test assignment for flag option, by short name
  {
    flag = true;
    args = nullopt;
    EXPECT_TRUE(cl.parse({ "a", "-🧑‍🌾=0", "b", "c" }, nio::out, nio::err, false));
    EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
    EXPECT_FALSE(flag.value_or(false));
  }

  // Test error when assigning a value
  {
    flag = false;
    args = nullopt;
    nio::StringSink buf;
    EXPECT_FALSE(cl.parse({ "a", "-🧑‍🌾=hello", "b", "c" }, buf, buf, false));
    EXPECT_EQ(buf.str(), "test-rocket-cl: error: Option `-🧑‍🌾` cannot take a value\n");
  }
}

TEST(cl, parseOptInt) {
  i32 num;
  optional<vector<string>> args;

  CommandLine cl( {
    Option::of(nullptr, "num", "n"_cv, "NUM", nullopt, num)
  }, {
    Parameter::of("ARG", "ARG", "command-line argument", args)
  });

  // Test mixed order
  {
    num = 0;
    args = nullopt;
    EXPECT_TRUE(cl.parse({ "a", "--num", "12", "b", "c" }, nio::out, nio::err, false));
    EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
    EXPECT_EQ(num, 12);
  }

  // Test assignment via '='
  {
    num = 0;
    args = nullopt;
    EXPECT_TRUE(cl.parse({ "a", "--num=12", "b", "c" }, nio::out, nio::err, false));
    EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
    EXPECT_EQ(num, 12);
  }

  // Test error when missing value
  {
    num = 0;
    args = nullopt;
    nio::StringSink buf;
    EXPECT_FALSE(cl.parse({ "a", "-n" }, buf, buf, false));
    EXPECT_EQ(buf.str(), "test-rocket-cl: error: Missing value for option `-n`\n");
  }

  // Test error when conversion fails
  {
    num = 0;
    args = nullopt;
    nio::StringSink buf;
    EXPECT_FALSE(cl.parse({ "a", "-n", "hello" }, buf, buf, false));
    EXPECT_EQ(buf.str(), "test-rocket-cl: error: Option `-n`: Cannot scan \"hello\" as `int`; expected NUM\n");
  }
}

TEST(cl, parseOptEnum) {
  log::LogLevel level;
  vector<string> args;

  CommandLine cl( {
    Option::of(nullptr, "level", "l"_cv, "LEVEL", nullopt, level)
  }, {
    Parameter::of("ARG", "ARG", "command-line argument", args)
  });

  // Test mixed order
  {
    level = log::LogLevel::none;
    EXPECT_TRUE(cl.parse({ "a", "--level", "info", "b", "c" }, nio::out, nio::err, false));
    EXPECT_EQ(args, (vector<string> { "a", "b", "c" }));
    EXPECT_EQ(level, log::LogLevel::info);
  }

  // Test error when conversion fails
  {
    level = log::LogLevel::none;
    nio::StringSink buf;
    EXPECT_FALSE(cl.parse({ "a", "-l", "nonsense" }, buf, buf, false));
    EXPECT_EQ(buf.str(), "test-rocket-cl: error: Option `-l`: Cannot scan \"nonsense\" as `rocket::log::LogLevel`; expected LEVEL\n");
  }
}

TEST(cl, parseOptVector) {
  vector<string> names;
  vector<string> args;

  CommandLine cl( {
    Option::of(nullptr, "name", "n"_cv, "NAME", nullopt, names)
  }, {
    Parameter::of("ARG", "ARG", "a command-line argument", args)
  });

  // Test multiple values
  {
    names.clear();
    args.clear();
    EXPECT_TRUE(cl.parse({ "a", "--name", "Shirley", "-n", "Deborah", "--name=Julie", "-n=Jane", "b" }, nio::out, nio::err, false));
    EXPECT_EQ(names, (vector<string> { "Shirley", "Deborah", "Julie", "Jane" }));
    EXPECT_EQ(args, (vector<string> { "a", "b" }));
  }
}

TEST(cl, parseShortOptions) {
  bool ignore;
  string name;
  bool verbose;
  vector<string> args;

  CommandLine cl( {
    Option::of(nullptr, "ignore", "i"_cv, nullopt, nullopt, ignore),
    Option::of(nullptr, "name", "n"_cv, "NAME", nullopt, name),
    Option::of(nullptr, "verbose", "v"_cv, nullopt, nullopt, verbose)
  }, {
    Parameter::of("ARG", "ARG", "a command-line argument", args)
  });

  // Test without '='
  ignore = false;
  name.clear();
  verbose = false;
  args.clear();
  EXPECT_TRUE(cl.parse({ "a", "-ivnSue", "b" }, nio::out, nio::err, false));
  EXPECT_TRUE(ignore);
  EXPECT_TRUE(verbose);
  EXPECT_EQ(name, "Sue");
  EXPECT_EQ(args, (vector<string> { "a", "b" }));

  // Test with '='
  ignore = false;
  name.clear();
  verbose = false;
  args.clear();
  EXPECT_TRUE(cl.parse({ "a", "-ivn=Sue", "b" }, nio::out, nio::err, false));
  EXPECT_TRUE(ignore);
  EXPECT_TRUE(verbose);
  EXPECT_EQ(name, "Sue");
  EXPECT_EQ(args, (vector<string> { "a", "b" }));

  // Test with ' '
  ignore = false;
  name.clear();
  verbose = false;
  args.clear();
  EXPECT_TRUE(cl.parse({ "a", "-ivn", "Sue", "b" }, nio::out, nio::err, false));
  EXPECT_TRUE(ignore);
  EXPECT_TRUE(verbose);
  EXPECT_EQ(name, "Sue");
  EXPECT_EQ(args, (vector<string> { "a", "b" }));
}

/**
 * Usage: cmd [-o | -?] list [-? | -l] FILE...
 *   or   cmd [-o | -?] show [-? | -s | -t] [ARG]...
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
        "test-rocket-cl: error: Parameter COMMAND: Invalid value `walk`; expected `list` or `show`\n"
        "Usage: test-rocket-cl [OPTION]... list [OPTION]... FILE...\n"
        "  or   test-rocket-cl [OPTION]... show [OPTION]... [ARG]...\n"
        "Try `test-rocket-cl --help` for more information.\n");
  }

  // Test help
  {
    nio::StringSink buf;
    parseCommand({ "--help" }, buf, buf);
    string str = buf.str();
    std::replace(str.begin(), str.end(), '\n', '~');

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
        "  -\\?, --help~"
        "          display this help text and exit~"
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
      "  -?, --help\n"
      "          display this help text and exit\n"
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
        "test-rocket-cl: error: Missing required argument for parameter FILE\n"
        "Usage: test-rocket-cl list [OPTION]... FILE...\n"
        "Try `test-rocket-cl list --help` for more information.\n");
  }

  // Test successful list command
  {
    nio::StringSink buf;
    parseCommand({ "list", "-l", "a", "b" }, buf, buf);
    EXPECT_EQ(parseCommandCommand, "list");
    EXPECT_TRUE(parseCommandList);
    EXPECT_EQ(parseCommandListFiles, (vector<string> { "a", "b" }));
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
      "  -?, --help\n"
      "          display this help text and exit\n"
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
    EXPECT_EQ(parseCommandListFiles, (vector<string> { "a", "b" }));
    EXPECT_EQ(buf.str(), "Showing ...\n");
  }
}

// EOF
