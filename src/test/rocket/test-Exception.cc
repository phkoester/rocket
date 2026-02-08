/*
 * test-Exception.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/assert.h"
#include "rocket/Exception.h"
#include "rocket/nio/nio.h"

#include <fmt/xchar.h>

// #TEST ----------------------------------------------------------------------------------------------------

TEST(Exception, WrappedExceptionFormat) {
  try  {
    ROCKET_FAIL("oops1");
  } catch (const exception& ex1) {
    EXPECT_THAT(fmt::format("{}", WrappedException(ex1)), matchesRegex(".*\\.cc:\\d+: oops1"));
    EXPECT_THAT(fmt::format("{:t}", WrappedException(ex1)), matchesRegex("`rocket::InvalidState`: .*\\.cc:\\d+: oops1"));

    const u32string s32 = fmt::format(U"{}", WrappedException(ex1));
    EXPECT_NE(s32.find(U"oops1"), u32string::npos);

    auto msg = fmt::format("{:?}", WrappedException(ex1));
    std::replace(msg.begin(), msg.end(), '\n', '|');
    EXPECT_THAT(msg, matchesRegex(".*\\.cc:\\d+: oops1\\|.*|.*|.*"));

    msg = fmt::format("{:?t}", WrappedException(ex1));
    std::replace(msg.begin(), msg.end(), '\n', '|');
    EXPECT_THAT(msg, matchesRegex("`rocket::InvalidState`: .*\\.cc:\\d+: oops1\\|.*|.*|.*"));

    try {
      throw_with_nested(InvalidArgument("name", "oops2"));
    } catch (const exception& ex2) {
      // Test #WrappedException with #std::exception
      EXPECT_THAT(
        fmt::format("{}", WrappedException(ex2)),
        matchesRegex(".*\\.cc:\\d+: Parameter `name`: oops2 \\(Because: .*\\.cc:\\d+: oops1\\)"));
      EXPECT_THAT(
        fmt::format("{:t}", WrappedException(ex2)),
        matchesRegex("`std::_.*ested.*<rocket::InvalidArgument>`: .*\\.cc:\\d+: Parameter `name`: oops2 \\(Because: .*\\.cc:\\d+: oops1\\)"));

      // Test #WrappedException with #std::exception_ptr
      EXPECT_THAT(
        fmt::format("{}", WrappedException(current_exception())),
        matchesRegex(".*\\.cc:\\d+: Parameter `name`: oops2 \\(Because: .*\\.cc:\\d+: oops1\\)"));
      EXPECT_THAT(
        fmt::format("{:t}", WrappedException(current_exception())),
        matchesRegex("`std::_.*ested.*<rocket::InvalidArgument>`: .*\\.cc:\\d+: Parameter `name`: oops2 \\(Because: .*\\.cc:\\d+: oops1\\)"));
    }
  }
}

TEST(Exception, printException1) { // NOLINT(*-complexity)
  try {
    throw "oops1";
  } catch (...) {
    try {
      throw_with_nested(InvalidArgument("name", "oops2"));
    } catch (...) {
      try {
        throw_with_nested(InvalidState("oops3"));
      } catch (const InvalidState& ex3) {
        nio::StringSink str1;
        printException(str1, ex3);
        EXPECT_THAT(str1.str(), AllOf(
            // Linux: #std::_Nested_exception, Windows: #std::_With_nested_v2
            containsRegex("An instance of `std::_.*ested.*<rocket::InvalidState>` was thrown: .*\\.cc:\\d+: oops3\n"),
            containsRegex("Caused by an instance of `std::_.*ested.*<rocket::InvalidArgument>`: .*\\.cc:\\d+: Parameter `name`: oops2\n"),
            // Linux: `char const*`, Windows: `charconst*__ptr64`
            containsRegex("Caused by an instance of `char.*const\\*.*`: \"oops1\"\n")));

        nio::StringSink str2;
        printException(str2, current_exception());
        EXPECT_EQ(str2.str(), str1.str());
      }
    }
  }
}

TEST(Exception, printException2) {
  try {
    throw "oops";
  } catch (...) {
    nio::StringSink str;
    printException(str, current_exception());
    // Linux: `char const*`, Windows: `charconst*__ptr64`
    EXPECT_THAT(str.str(), matchesRegex("An instance of `char.*const\\*.*` was thrown: \"oops\"\n"));
  }
}

TEST(Exception, what1) { // NOLINT(*-complexity)
  try {
    throw "oops1";
  } catch (...) {
    try {
      throw_with_nested(InvalidArgument("name", "oops2"));
    } catch (...) {
      try {
        throw_with_nested(InvalidState("oops3"));
      } catch (const InvalidState& ex3) {
        const string str1 = what(ex3);
        EXPECT_THAT(str1, matchesRegex(".*\\.cc:\\d+: oops3 \\(Because: .*\\.cc:\\d+: Parameter `name`: oops2 \\(Because: \"oops1\"\\)\\)"));

        const string str2 = what(current_exception());
        EXPECT_EQ(str2, str1);
      }
    }
  }
}

TEST(Exception, what2) {
  try {
    throw "oops";
  } catch (...) {
    EXPECT_EQ(what(current_exception()), "\"oops\"");
  }
}

// EOF
