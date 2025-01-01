/*
 * test-except.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/except.h"

#include "rocket-gtest/match.h"

using namespace rocket;
using namespace rocket::except;
using namespace rocket::gtest::match;
using namespace std;
using namespace testing;

// 'TEST' ---------------------------------------------------------------------------------------------------

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
        ostringstream os1;
        printException(os1, ex3);
        string s1 = os1.str();
        EXPECT_THAT(s1, AllOf(
            containsRegex("An instance of 'std::_Nested_exception<rocket::except::InvalidState>' was thrown: .*\\.cc:\\d+: oops3\n"),
            containsRegex("Caused by an instance of 'std::_Nested_exception<rocket::except::InvalidArgument>': .*\\.cc:\\d+: Parameter 'name': oops2\n"),
            containsRegex("Caused by an instance of 'char const\\*': \"oops1\"\n")));

        ostringstream os2;
        printException(os2, current_exception());
        string s2 = os2.str();
        EXPECT_EQ(s2, s1);
      }
    }
  }
}

TEST(except, printException2) {
  try {
    throw "oops";
  } catch (...) {
    ostringstream os;
    printException(os, current_exception());
    string s = os.str();
    EXPECT_EQ(s, "An instance of 'char const*' was thrown: \"oops\"\n");
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
        EXPECT_THAT(s1, matchesRegex(".*\\.cc:\\d+: oops3 \\(Because: .*: Parameter 'name': oops2 \\(Because: \"oops1\"\\)\\)"));

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
