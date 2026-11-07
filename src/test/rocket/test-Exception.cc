/*
 * test-Exception.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/assert.h"
#include "rocket/Exception.h"
#include "rocket/TypeId.h"
#include "rocket/nio/nio.h"

using namespace std;

namespace {

// Local functions ------------------------------------------------------------------------------------------

string
nestedException(const string& type) {
#if defined(ROCKET_OS_LINUX)
  // Linux
  return "`std::_Nested_exception<" + type + ">`";
#elif defined(ROCKET_OS_MACOS)
  // macOS
  return "`std::__nested<" + type + ">`";
#else
  // Windows
  return "`std::_With_nested_v2<" + type + ">`";
#endif
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

// Segfaults on Windows with Clang 22.1.3
TEST(Exception, WrappedExceptionFormat) { // NOLINT(*-complexity)
  try  {
    ROCKET_FAIL("oops1");
  } catch (const exception& ex1) {
    EXPECT_THAT(fmt::format("{}", WrappedException(ex1)), matchesRegex(".*\\.cc:\\d+: oops1"));
    EXPECT_THAT(fmt::format("{:t}", WrappedException(ex1)), matchesRegex("`rocket::InvalidState`: .*\\.cc:\\d+: oops1"));

    const u32string s32 = fmt::format(U"{}", WrappedException(ex1));
    EXPECT_NE(s32.find(U"oops1"), u32string::npos);

    auto msg = fmt::format("{:?}", WrappedException(ex1));
    ranges::replace(msg, '\n', '|');
    EXPECT_THAT(msg, matchesRegex(".*\\.cc:\\d+: oops1\\|.*|.*|.*"));

    msg = fmt::format("{:?t}", WrappedException(ex1));
    ranges::replace(msg, '\n', '|');
    EXPECT_THAT(msg, matchesRegex("`rocket::InvalidState`: .*\\.cc:\\d+: oops1\\|.*|.*|.*"));

    try {
      throw_with_nested(InvalidArgument("name", "oops2"));
    } catch (const exception& ex2) {
      // Test `WrappedException` with `std::exception`
      EXPECT_THAT(
        fmt::format("{}", WrappedException(ex2)),
        matchesRegex(".*\\.cc:\\d+: Parameter `name`: oops2 \\(Because: .*\\.cc:\\d+: oops1\\)"));
      EXPECT_THAT(
        fmt::format("{:t}", WrappedException(ex2)),
        matchesRegex(nestedException("rocket::InvalidArgument") + ": .*\\.cc:\\d+: Parameter `name`: oops2 \\(Because: .*\\.cc:\\d+: oops1\\)"));

      // Test `WrappedException` with `std::exception_ptr`
      EXPECT_THAT(
        fmt::format("{}", WrappedException(current_exception())),
        matchesRegex(".*\\.cc:\\d+: Parameter `name`: oops2 \\(Because: .*\\.cc:\\d+: oops1\\)"));
#ifdef __GLIBCXX__
      EXPECT_THAT(
        fmt::format("{:t}", WrappedException(current_exception())),
        matchesRegex(nestedException("rocket::InvalidArgument") + ": .*\\.cc:\\d+: Parameter `name`: oops2 \\(Because: .*\\.cc:\\d+: oops1\\)"));
#else
      EXPECT_THAT(
        fmt::format("{:t}", WrappedException(current_exception())),
        matchesRegex(".*\\.cc:\\d+: Parameter `name`: oops2 \\(Because: .*\\.cc:\\d+: oops1\\)"));
#endif
    }
  }
}

// Segfaults on Windows with Clang 22.1.3
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
          containsRegex("An instance of " + nestedException("rocket::InvalidState") + " was thrown: .*\\.cc:\\d+: oops3\n"),
          containsRegex("Caused by an instance of " + nestedException("rocket::InvalidArgument") + ": .*\\.cc:\\d+: Parameter `name`: oops2\n"),
          containsRegex("Caused by an instance of `char const\\*`: \"oops1\"\n")));

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
    EXPECT_THAT(str.str(), matchesRegex("An instance of `char const\\*` was thrown: \"oops\"\n"));
  }
}

// Segfaults on Windows with Clang 22.1.3
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

TEST(Exception, nestedExceptionTypeId) {
  try {
    try {
      throw "oops";
    } catch (...) {
      throw_with_nested(InvalidArgument("name", "oops2"));
    }
  } catch (const InvalidArgument& ex) {
    EXPECT_EQ(fmt::format("{}", TypeId(typeid(ex))), nestedException("rocket::InvalidArgument"));
  }
}

// EOF
