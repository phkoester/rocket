/*
 * test-Exception.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/Exception.h"

#include "rocket-gtest/matcher.h"

using namespace rocket;
using namespace rocket::gtest::matcher;
using namespace std;
using namespace testing;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(except, printException1) {
  try {
    throw "oops1";
  } catch (...) {
    try {
      throw_with_nested(InvalidArgument("name", "oops2"));
    } catch (...) {
      try {
        throw_with_nested(InvalidState("oops3"));
      } catch (const InvalidState& ex3) {
        nio::StringSink s1;
        printException(s1, ex3);
        EXPECT_THAT(s1.str(), AllOf(
            containsRegex("An instance of `std::_Nested_exception<rocket::InvalidState>` was thrown: .*\\.cc:\\d+: oops3\n"),
            containsRegex("Caused by an instance of `std::_Nested_exception<rocket::InvalidArgument>`: .*\\.cc:\\d+: Parameter `name`: oops2\n"),
            containsRegex("Caused by an instance of `char const\\*`: \"oops1\"\n")));

        nio::StringSink s2;
        printException(s2, current_exception());
        EXPECT_EQ(s2.str(), s1.str());
      }
    }
  }
}

TEST(except, printException2) {
  try {
    throw "oops";
  } catch (...) {
    nio::StringSink s;
    printException(s, current_exception());
    EXPECT_EQ(s.str(), "An instance of `char const*` was thrown: \"oops\"\n");
  }
}

TEST(except, what1) {
  try {
    throw "oops1";
  } catch (...) {
    try {
      throw_with_nested(InvalidArgument("name", "oops2"));
    } catch (...) {
      try {
        throw_with_nested(InvalidState("oops3"));
      } catch (const InvalidState& ex3) {
        string s1 = what(ex3);
        EXPECT_THAT(s1, matchesRegex(".*\\.cc:\\d+: oops3 \\(Because: .*: Parameter `name`: oops2 \\(Because: \"oops1\"\\)\\)"));

        string s2 = what(current_exception());
        EXPECT_EQ(s2, s1);
      }
    }
  }
}

TEST(except, what2) {
  try {
    throw "oops";
  } catch (...) {
    EXPECT_EQ(what(current_exception()), "\"oops\"");
  }
}

// EOF
