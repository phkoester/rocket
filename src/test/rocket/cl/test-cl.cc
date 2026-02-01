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

  CommandLineConfig config {
    .usages={ "[OPTION]... list [OPTION]... FILE...", "[OPTION]... show [OPTION]... [ARG]..." },
    .prolog="List FILEs or show ARGs.\n\nThis is yet another paragraph.",
    .epilog="Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur. Hi omnes lingua, institutis, legibus inter se differunt. Gallos ab Aquitanis Garunna flumen, a Belgis Matrona et Sequana dividit."
  };

  auto command = Parameter::of("COMMAND", { "list", "show" }, "a command", parseCommandCommand);
  command.consumeOpts = true;

  CommandLine cl({
    // The group order matters!
    Option::of(&general, "omit", "o"_c, nullopt, "omit what is not important", parseCommandOmit),
    Option::helpOf(&misc, parseCommandHelp)
  }, {
    command,
    Parameter::of("ARG", nullopt, "a command-line argument", parseCommandArgs)
  }, config);

  if (not cl.parse(args, out, err, false)) {
    return;
  }

  // Parse, step 2

  if (parseCommandCommand == "list") {
    // 1. "list" with "FILE..." (mandatory)

    OptionGroup list { "List control" };

    CommandLineConfig listConfig {
      .command = config.command + " list",
      .usages={ "[OPTION]... FILE..." },
      .rocketOpts=false
    };

    CommandLine listCl({
      Option::helpOf(&list, parseCommandListHelp),
      Option::of(&list, "list", "l"_c, nullopt, "a list option that is good for nothing", parseCommandList)
    }, {
      Parameter::of("FILE", "file", "a file to list", parseCommandListFiles)
    }, listConfig);

    if (not listCl.parse(parseCommandArgs, out, err, false)) {
      return;
    }

    out.writeln("Listing ...");
  } else {
    // 2. "show" with "[ARG]..." (optional)

    OptionGroup show { "Show control" };

    CommandLineConfig showConfig {
      .command = config.command + " show",
      .usages={ "[OPTION]... [ARG]..." },
      .rocketOpts=false
    };

    CommandLine showCl({
      Option::helpOf(&show, parseCommandShowHelp),
      Option::of(&show, "show", "s"_c, nullopt, "a show option that is good for nothing", parseCommandShow),
      Option::of(&show, "test", "t"_c, nullopt, "test something, or don't", parseCommandShowTest)
    }, {
      Parameter::of("ARG", nullopt, "a thing to show", parseCommandShowArgs)
    }, showConfig);

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
      Parameter::of("ARG", nullopt, "a command-line argument", args)
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
    Option::of(nullptr, "flag", "🧑‍🌾"_c, nullopt, nullopt, flag)
  }, {
    Parameter::of("ARG", nullopt, "a command-line argument", args)
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
    Option::of(nullptr, "num", "n"_c, "number", "a number", num)
  }, {
    Parameter::of("ARG", nullopt, "a command-line argument", args)
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
    EXPECT_EQ(buf.str(), "test-rocket-cl: error: Option `-n`: Cannot scan \"hello\" as `int`; expected number\n");
  }
}

TEST(cl, parseOptEnum) {
  log::LogLevel level;
  vector<string> args;

  CommandLine cl( {
    Option::of(nullptr, "level", "l"_c, nullopt, nullopt, level)
  }, {
    Parameter::of("ARG", nullopt, "a command-line argument", args)
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
    EXPECT_EQ(buf.str(), "test-rocket-cl: error: Option `-l`: Cannot scan \"nonsense\" as `rocket::log::LogLevel`\n");
  }
}

TEST(cl, parseOptVector) {
  vector<string> names;
  vector<string> args;

  CommandLine cl( {
    Option::of(nullptr, "name", "n"_c, nullopt, nullopt, names)
  }, {
    Parameter::of("ARG", nullopt, "a command-line argument", args)
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
    Option::of(nullptr, "ignore", "i"_c, nullopt, nullopt, ignore),
    Option::of(nullptr, "name", "n"_c, nullopt, nullopt, name),
    Option::of(nullptr, "verbose", "v"_c, nullopt, nullopt, verbose)
  }, {
    Parameter::of("ARG", nullopt, "a command-line argument", args)
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
        "test-rocket-cl: error: Parameter `COMMAND`: Invalid value `walk`; expected `list` or `show`\n"
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
        "Parameters:~"
        "~"
        "  COMMAND `list` or `show` \\(required\\)~"
        "          a command~"
        "  ARG \\(required\\)~"
        "          a command-line argument~"
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
      "Parameters:\n"
      "\n"
      "  FILE file (required)\n"
      "          a file to list\n"
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
        "test-rocket-cl: error: Missing required argument for parameter `FILE`\n"
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
      "Parameters:\n"
      "\n"
      "  ARG\n"
      "          a thing to show\n"
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
