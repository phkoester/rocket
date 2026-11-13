/*
 * test-system.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/log/log.h"
#include "rocket/system/system.h"

#include <algorithm>
#include <csignal>
#include <filesystem>

namespace fs = std::filesystem;

using namespace rocket::system;

namespace {

// Local constants ------------------------------------------------------------------------------------------

/// A command line that writes `out` to stdout, then `err` to stderr.
#ifdef ROCKET_OS_WINDOWS
const string OUT_THEN_ERR = "echo out& echo err>&2";
#else
const string OUT_THEN_ERR = "echo out; echo err >&2";
#endif

// Local functions ------------------------------------------------------------------------------------------

/// Returns @p line with the line break that `echo` writes. The output is captured as is, with no conversion.
string
nl(string_view line) {
  string ret(line);
  ret.append(NL);
  return ret;
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(system, envBool) {
  using type = bool;

  const char* name = "MY_BOOL";

  env::unset(name);
  EXPECT_EQ(env::get<type>(name).error(), env::GetError::NotFound);

  env::set(name, true);
  // Check twice, there was a bad surprise with `std::putenv` ...
  EXPECT_EQ(env::get<type>(name), true);
  EXPECT_EQ(env::get<type>(name), true);

  env::unset(name);
  EXPECT_EQ(env::get<type>(name).error(), env::GetError::NotFound);

  env::set(name, "0");
  EXPECT_EQ(env::get<type>(name), false);
  env::set(name, "1");
  EXPECT_EQ(env::get<type>(name), true);
  env::set(name, "foo");
  EXPECT_EQ(env::get<type>(name), true);
}

TEST(system, envF64) {
  using type = f64;

  const char* name = "MY_F64";

  env::unset(name);
  EXPECT_EQ(env::get<type>(name).error(), env::GetError::NotFound);

  env::set(name, -1.2);
  // Check twice, there was a bad surprise with `std::putenv` ...
  EXPECT_EQ(env::get<type>(name), -1.2);
  EXPECT_EQ(env::get<type>(name), -1.2);

  env::unset(name);
  EXPECT_EQ(env::get<type>(name).error(), env::GetError::NotFound);

  env::set(name, -1.3);
  EXPECT_EQ(env::get<type>(name), -1.3);
}

TEST(system, envLogLevel) {
  using type = log::LogLevel;

  const char* name = "MY_LOG_LEVEL";

  env::unset(name);
  EXPECT_EQ(env::get<type>(name).error(), env::GetError::NotFound);
  // Convert to `optional<LogLevel>` and check if it is empty
  EXPECT_EQ(
    env::get<type>(name).transform([](type val) { return optional<log::LogLevel>(val); }).value_or({}),
    optional<type>());

  env::set(name, type::debug);
  // Check twice, there was a bad surprise with `std::putenv` ...
  EXPECT_EQ(env::get<type>(name), type::debug);
  EXPECT_EQ(env::get<type>(name), type::debug);

  env::set(name, "something");
  EXPECT_EQ(env::get<type>(name).error(), env::GetError::ConversionFailed);
  // Convert to `optional<LogLevel>` and check if it is empty
  EXPECT_EQ(
    env::get<type>(name).transform([](type val) { return optional<log::LogLevel>(val); }).value_or({}),
    optional<type>());

  env::unset(name);
  EXPECT_EQ(env::get<type>(name).error(), env::GetError::NotFound);
}

TEST(system, envString) {
  using type = string;

  const char* name = "MY_STRING";

  env::unset(name);
  EXPECT_EQ(env::get<type>(name).error(), env::GetError::NotFound);

  env::set(name, "some text"sv);
  // Check twice, there was a bad surprise with `std::putenv` ...
  EXPECT_EQ(env::get<type>(name), "some text");
  EXPECT_EQ(env::get<type>(name), "some text");

  env::unset(name);
  EXPECT_EQ(env::get<type>(name).error(), env::GetError::NotFound);
}

TEST(system, envGet) {
  env::set("MY_ENV1", "");
  env::set("MY_ENV2", "value");
  auto env = env::get();
#ifdef ROCKET_OS_WINDOWS
  EXPECT_FALSE(env.contains("MY_ENV1"));
#else
  EXPECT_EQ(env.at("MY_ENV1"), "");
#endif
  EXPECT_EQ(env.at("MY_ENV2"), "value");
}

TEST(system, execEcho) {
  const auto result = exec("echo Hello");
  EXPECT_TRUE(result);
  EXPECT_EQ(result.out, nl("Hello"));
  EXPECT_EQ(result.err, "");
  EXPECT_EQ(result.status, 0);
  EXPECT_EQ(result.signal, nullopt);
  EXPECT_FALSE(result.error);
}

TEST(system, execStderr) {
  const auto result = exec("echo Hello>&2");
  EXPECT_TRUE(result);
  EXPECT_EQ(result.out, "");
  EXPECT_EQ(result.err, nl("Hello"));
}

TEST(system, execSeparate) {
  const auto result = exec(OUT_THEN_ERR);
  EXPECT_TRUE(result);
  EXPECT_EQ(result.out, nl("out"));
  EXPECT_EQ(result.err, nl("err"));
}

TEST(system, execCombined) {
  const auto result = exec(OUT_THEN_ERR, true);
  EXPECT_TRUE(result);
  EXPECT_EQ(result.out, nl("out") + nl("err"));
  EXPECT_EQ(result.err, "");
}

TEST(system, execCombinedArgs) {
  const string executable = binExecutable("print-args").string();

  const auto result = exec({ executable, "a" }, true);
  EXPECT_TRUE(result);
  EXPECT_THAT(result.out, HasSubstr("1=a="));
  EXPECT_EQ(result.err, "");
}

TEST(system, execLargeOutput) {
  // Much more output to both streams than a pipe holds. Reading one stream after the other would block
  constexpr i32 LINES = 10'000;
#ifdef ROCKET_OS_WINDOWS
  const string cl = fmt::format("for /L %i in (1,1,{}) do @(echo xxxxxxxxxx& echo yyyyyyyyyy>&2)", LINES);
#else
  const string cl = fmt::format(
    "i=0; while [ $i -lt {} ]; do echo xxxxxxxxxx; echo yyyyyyyyyy >&2; i=$((i+1)); done", LINES);
#endif

  const auto result = exec(cl);
  EXPECT_TRUE(result);
  EXPECT_EQ(ranges::count(result.out, '\n'), LINES);
  EXPECT_EQ(ranges::count(result.err, '\n'), LINES);
  EXPECT_EQ(ranges::count(result.out, 'y'), 0);
  EXPECT_EQ(ranges::count(result.err, 'x'), 0);
}

TEST(system, execExitStatus) {
  const auto result = exec("exit 3");
  EXPECT_FALSE(result);
  EXPECT_EQ(result.status, 3);
  EXPECT_EQ(result.signal, nullopt);
  EXPECT_FALSE(result.error);
}

#ifndef ROCKET_OS_WINDOWS
TEST(system, execSignal) {
  // `kill` is built into the shell, so the shell terminates itself
  const auto result = exec("kill -TERM $$");
  EXPECT_FALSE(result);
  EXPECT_EQ(result.status, nullopt);
  EXPECT_EQ(result.signal, SIGTERM);
  EXPECT_FALSE(result.error);
}
#endif // ROCKET_OS_WINDOWS

TEST(system, execNotFound) {
  const auto result = exec(vector<string_view> { "rocket-no-such-program" });
  EXPECT_FALSE(result);
  EXPECT_EQ(result.status, nullopt);
  EXPECT_EQ(result.signal, nullopt);
  EXPECT_TRUE(result.error);
}

TEST(system, execNotFoundShell) {
  const auto result = exec("rocket-no-such-program");
  EXPECT_FALSE(result);
#ifdef ROCKET_OS_WINDOWS
  // `cmd.exe /c` exits with 9009 on older builds of Windows and with 1 on newer ones
  EXPECT_THAT(result.status, Optional(AnyOf(1, 9'009)));
#else
  EXPECT_EQ(result.status, 127);
#endif
  EXPECT_FALSE(result.error);
  EXPECT_FALSE(result.err.empty());
}

TEST(system, execInvalid) {
  EXPECT_THAT(
    [&] { exec(string("a\0b", 3)); },
    ThrowsMessage<InvalidArgument>(EndsWith("May not contain NUL characters")));
  EXPECT_THAT(
    [&] { exec(vector<string_view> {}); },
    ThrowsMessage<InvalidArgument>(EndsWith("May not be empty")));
  EXPECT_THAT(
    [&] { exec(vector<string_view> { "" }); },
    ThrowsMessage<InvalidArgument>(EndsWith("Program may not be empty")));
}

#ifdef ROCKET_OS_WINDOWS
TEST(system, execBatchFile) {
  for (const string_view program : { "a.bat", "a.cmd", "A.BAT", "a.Cmd", "a.bat.", "a.bat. ." }) {
    SCOPED_TRACE(string(program));
    EXPECT_THAT(
      [&] { exec({ program, "arg" }); },
      ThrowsMessage<InvalidArgument>(EndsWith("Program may not be a batch file on Windows")));
  }
}
#endif // ROCKET_OS_WINDOWS

TEST(system, execPrintf) {
  const auto out = exec("printf \"%s\" Hello").out;
  EXPECT_EQ(out, "Hello");
}

TEST(system, execPrintArgs) {
  const fs::path printArgs = binExecutable("print-args");
  const string executable = printArgs.string();

  {
    // Test spaces, quotes and backslash
    const auto out = exec({ executable, "a", "b c", " d ", "'", "\"", "\\" }).out;
    EXPECT_THAT(out, HasSubstr("1=a="));
    EXPECT_THAT(out, HasSubstr("2=b c="));
    EXPECT_THAT(out, HasSubstr("3= d ="));
    EXPECT_THAT(out, HasSubstr("4='="));
    EXPECT_THAT(out, HasSubstr("5=\"="));
    EXPECT_THAT(out, HasSubstr("6=\\="));
  }

  {
    // Test Unicode
    const auto out = exec({ executable, "€ ÄÖÜ € 🧑‍🌾" }).out;
    EXPECT_THAT(out, HasSubstr("1=€ ÄÖÜ € 🧑‍🌾="));
  }
}

TEST(system, execPrintArgsWithSpace) {
  // Copy `print-args` to `print args`, se we have a space in the executable name
  const fs::path printArgs = binExecutable("print-args");
  const fs::path printArgsWithSpace =
    printArgs.parent_path() / fmt::format("print args{}", executableSuffix());
  fs::copy_file(printArgs, printArgsWithSpace, fs::copy_options::overwrite_existing);
  const string executable = printArgsWithSpace.string();

  {
    // Test spaces and quotes, and backslash
    const auto out = exec({ executable, "a", "b c", " d ", "'", "\"", "\\" }).out;
    EXPECT_THAT(out, HasSubstr("1=a="));
    EXPECT_THAT(out, HasSubstr("2=b c="));
    EXPECT_THAT(out, HasSubstr("3= d ="));
    EXPECT_THAT(out, HasSubstr("4='="));
    EXPECT_THAT(out, HasSubstr("5=\"="));
    EXPECT_THAT(out, HasSubstr("6=\\="));
  }

  {
    // Test Unicode
    const auto out = exec({ executable, "€ ÄÖÜ € 🧑‍🌾" }).out;
    EXPECT_THAT(out, HasSubstr("1=€ ÄÖÜ € 🧑‍🌾="));
  }
}

TEST(system, execArgsAreLiteral) {
  const string executable = binExecutable("print-args").string();

  // None of these may be interpreted by a shell, on any platform
  const vector<string_view> values {
    "$HOME",
    "${HOME}",
    "$(echo injected)",
    "`echo injected`",
    "a;echo injected",
    "a|echo injected",
    "a&echo injected",
    "a&&echo injected",
    "a\techo injected",
    "a>b",
    "a<b",
    "*",
    "?",
    "[a-z]",
    "~",
    "#a",
    "(a)",
    "{a,b}",
    "!a",
    "%PATH%",
    "^",
    "a^b",
    "a=b",
    "a,b",
  };

  for (const string_view val : values) {
    SCOPED_TRACE(string(val));

    const auto out = exec({ executable, val }).out;
    EXPECT_THAT(out, HasSubstr(fmt::format("1={}=", val)));
    EXPECT_THAT(out, Not(HasSubstr("2="))); // The value must not be split
    EXPECT_THAT(out, Not(HasSubstr("\ninjected"))); // An injected command would print on a line of its own
  }
}

TEST(system, execArgsEmpty) {
  const string executable = binExecutable("print-args").string();

  const auto out = exec({ executable, "", "a", "", "" }).out;
  EXPECT_THAT(out, HasSubstr("1=="));
  EXPECT_THAT(out, HasSubstr("2=a="));
  EXPECT_THAT(out, HasSubstr("3=="));
  EXPECT_THAT(out, HasSubstr("4=="));
  EXPECT_THAT(out, Not(HasSubstr("5=")));
}

TEST(system, execArgsApostrophes) {
  const string executable = binExecutable("print-args").string();

  for (const string_view val : { "it's"sv, "''"sv, "'a b'"sv, "\"'\""sv, "'\\''"sv }) {
    SCOPED_TRACE(string(val));

    const auto out = exec({ executable, val }).out;
    EXPECT_THAT(out, HasSubstr(fmt::format("1={}=", val)));
    EXPECT_THAT(out, Not(HasSubstr("2=")));
  }
}

TEST(system, execArgsBackslashes) {
  const string executable = binExecutable("print-args").string();

  // Windows treats backslashes specially in front of quotation marks, which includes the closing one
  const vector<string_view> values { "a\\ b", "a\\", "a\\\\", "\\\\server\\share", "a\\\"b", "a\\\\\"b" };

  for (const string_view val : values) {
    SCOPED_TRACE(string(val));

    const auto out = exec({ executable, val }).out;
    EXPECT_THAT(out, HasSubstr(fmt::format("1={}=", val)));
    EXPECT_THAT(out, Not(HasSubstr("2=")));
  }
}

TEST(system, execArgsLineBreak) {
  const string executable = binExecutable("print-args").string();

  // No shell is involved, so line breaks can be passed on Windows as well. The bytes are checked rather than
  // the echoed text because Windows translates each `\n` written to stdout into `\r\n`.
  const vector<pair<string_view, string_view>> cases {
    { "a\nb"sv, "1: [97, 10, 98]"sv },
    { "a\r\nb"sv, "1: [97, 13, 10, 98]"sv },
  };

  for (const auto& [val, bytes] : cases) {
    SCOPED_TRACE(string(val));

    const auto out = exec({ executable, val }).out;
    EXPECT_THAT(out, HasSubstr(bytes));
    EXPECT_THAT(out, Not(HasSubstr("2:")));
  }
}

TEST(system, execArgsNul) {
  const string executable = binExecutable("print-args").string();

  EXPECT_THAT(
    [&] { exec({ executable, "a\0b"sv }); },
    ThrowsMessage<InvalidArgument>(EndsWith("May not contain NUL characters")));
}

TEST(system, makeArgs) {
  EXPECT_EQ(makeArgs(""), vector<string> {});
  EXPECT_EQ(makeArgs(" \r\n\t   "), vector<string> {});
  EXPECT_EQ(makeArgs(" \r\na\t   "), vector<string> { "a" });

  EXPECT_EQ(makeArgs("a 'b c' d"), (vector<string> { "a", "b c", "d" }));
  EXPECT_EQ(makeArgs("a'b c'd"), (vector<string> { "ab cd" }));
  EXPECT_EQ(makeArgs("a'\\\"'b"), (vector<string> { "a\\\"b" }));

  EXPECT_EQ(makeArgs("a \"b c\" d"), (vector<string> { "a", "b c", "d" }));
  EXPECT_EQ(makeArgs("a\"b c\"d"), (vector<string> { "ab cd" }));
  EXPECT_EQ(makeArgs("\"\\\"\""), (vector<string> { "\"" }));
  EXPECT_EQ(makeArgs("\"\\\\\""), (vector<string> { "\\" }));

  EXPECT_THAT(
    [&] { makeArgs(" a'"); },
    throwsInputFailure(3, EndsWith("Unterminated single quote")));
  EXPECT_THAT(
    [&] { makeArgs(" a\""); },
    throwsInputFailure(3, EndsWith("Unterminated double quote")));
  EXPECT_THAT(
    [&] { makeArgs("\\"); },
    throwsInputFailure(0, EndsWith("Invalid escape sequence")));
  EXPECT_THAT(
    [&] { makeArgs("\"\\"); },
    throwsInputFailure(1, EndsWith("Invalid escape sequence")));
}

// EOF
