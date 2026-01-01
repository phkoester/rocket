/*
 * test-matcher.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/InputFailure.h"

#include "rocket-gtest/matcher/matcher.h"

using namespace rocket;
using namespace rocket::gtest::matcher;
using namespace std;
using namespace testing;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(match, containsRegex) {
  EXPECT_THAT("Rocket", containsRegex("ock"));
  EXPECT_THAT("Rocket", containsRegex(".ck"));
  EXPECT_THAT("Rocket", containsRegex("o.+e"));
}

TEST(match, matchesRegex) {
  EXPECT_THAT("main.cc:42: foo", matchesRegex(".*\\.cc:\\d+: foo"));
}

TEST(match, throwsInputFailure) {
  EXPECT_THAT(
    [&] { throw InputFailure(2, "oops"); },
    throwsInputFailure(Eq(2), HasSubstr("oops")));

  EXPECT_THAT(
    [&] { throw InputFailure(2, "oops"); },
    throwsInputFailure(2, HasSubstr("oops")));

  EXPECT_THAT(
    [&] { throw InputFailure(2, { { 1, 2 }, { 3, 4 } }, "oops"); },
    throwsInputFailure(Eq(2), Eq(text::Ranges { { 1, 2 }, { 3, 4 } }), HasSubstr("oops")));

  EXPECT_THAT(
    [&] { throw InputFailure(2, { { 1, 2 }, { 3, 4 } }, "oops"); },
    throwsInputFailure(2, { { 1, 2 }, { 3, 4 } }, HasSubstr("oops")));
}

// EOF
