/*
 * test-Exception.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Exception.h"
#include "rocket/nio/nio.h"

// #TEST ----------------------------------------------------------------------------------------------------

TEST(Exception, printException1) {
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

TEST(Exception, what1) {
  try {
    throw "oops1";
  } catch (...) {
    try {
      throw_with_nested(InvalidArgument("name", "oops2"));
    } catch (...) {
      try {
        throw_with_nested(InvalidState("oops3"));
      } catch (const InvalidState& ex3) {
        string str1 = what(ex3);
        EXPECT_THAT(str1, matchesRegex(".*\\.cc:\\d+: oops3 \\(Because: .*\\.cc:\\d+: Parameter `name`: oops2 \\(Because: \"oops1\"\\)\\)"));

        string str2 = what(current_exception());
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
