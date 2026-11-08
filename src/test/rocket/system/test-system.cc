/*
 * test-system.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/log/log.h"
#include "rocket/system/system.h"

#include <filesystem>

namespace fs = std::filesystem;

using namespace rocket::system;

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
  const auto out = exec("echo Hello");
  EXPECT_EQ(out, "Hello\n");
}

TEST(system, execPrintf) {
  const auto out = exec("printf \"%s\" Hello");
  EXPECT_EQ(out, "Hello");
}

TEST(system, execPrintArgs) {
  const fs::path printArgs = binExecutable("print-args");
  const string executable = printArgs.string();

  {
    // Test spaces, quotes and backslash
    const auto out = exec({ executable, "a", "b c", " d ", "'", "\"", "\\" });
    EXPECT_THAT(out, HasSubstr("1=a="));
    EXPECT_THAT(out, HasSubstr("2=b c="));
    EXPECT_THAT(out, HasSubstr("3= d ="));
    EXPECT_THAT(out, HasSubstr("4='="));
    EXPECT_THAT(out, HasSubstr("5=\"="));
    EXPECT_THAT(out, HasSubstr("6=\\="));
  }

  {
    // Test Unicode
    const auto out = exec({ executable, "€ ÄÖÜ € 🧑‍🌾" });
    EXPECT_THAT(out, HasSubstr("1=€ ÄÖÜ € 🧑‍🌾="));
  }
}

TEST(system, execPrintArgsWithSpace) {
  // Copy `print-args` to `print args`, se we have a space in the executable name
  const fs::path printArgs = binExecutable("print-args");
  const fs::path printArgsWithSpace = printArgs.parent_path() / fmt::format("print args{}", executableSuffix());
  fs::copy_file(printArgs, printArgsWithSpace, fs::copy_options::overwrite_existing);
  const string executable = printArgsWithSpace.string();

  {
    // Test spaces and quotes, and backslash
    const auto out = exec({ executable, "a", "b c", " d ", "'", "\"", "\\" });
    EXPECT_THAT(out, HasSubstr("1=a="));
    EXPECT_THAT(out, HasSubstr("2=b c="));
    EXPECT_THAT(out, HasSubstr("3= d ="));
    EXPECT_THAT(out, HasSubstr("4='="));
    EXPECT_THAT(out, HasSubstr("5=\"="));
    EXPECT_THAT(out, HasSubstr("6=\\="));
  }

  {
    // Test Unicode
    const auto out = exec({ executable, "€ ÄÖÜ € 🧑‍🌾" });
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

    const auto out = exec({ executable, val });
    EXPECT_THAT(out, HasSubstr(fmt::format("1={}=", val)));
    EXPECT_THAT(out, Not(HasSubstr("2="))); // The value must not be split
    EXPECT_THAT(out, Not(HasSubstr("\ninjected"))); // An injected command would print on a line of its own
  }
}

TEST(system, execArgsEmpty) {
  const string executable = binExecutable("print-args").string();

  const auto out = exec({ executable, "", "a", "", "" });
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

    const auto out = exec({ executable, val });
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

    const auto out = exec({ executable, val });
    EXPECT_THAT(out, HasSubstr(fmt::format("1={}=", val)));
    EXPECT_THAT(out, Not(HasSubstr("2=")));
  }
}

TEST(system, execArgsLineBreak) {
  const string executable = binExecutable("print-args").string();

#ifdef ROCKET_OS_WINDOWS
  EXPECT_THAT(
    [&] { exec({ executable, "a\nb" }); },
    ThrowsMessage<InvalidArgument>(EndsWith("May not contain line breaks on Windows")));
#else
  const auto out = exec({ executable, "a\nb" });
  EXPECT_THAT(out, HasSubstr("1=a\nb="));
  EXPECT_THAT(out, Not(HasSubstr("2=")));
#endif
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
