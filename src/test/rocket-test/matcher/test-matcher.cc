// test-matcher.cc

#include "rocket-test/rocket-test.h"

#include "rocket/InputFailure.h"

#define MODULE rocketTestMatcher

// Tests ====================================================================================================

TEST(MODULE, containsRegex) {
  EXPECT_THAT("Rocket", containsRegex("ock"));
  EXPECT_THAT("Rocket", containsRegex(".ck"));
  EXPECT_THAT("Rocket", containsRegex("o.+e"));
}

TEST(MODULE, matchesRegex) {
  EXPECT_THAT("main.cc:42: foo", matchesRegex(".*\\.cc:\\d+: foo"));
}

TEST(MODULE, throwsInputFailure) {
  EXPECT_THAT(
    [&] { throw InputFailure(2, "oops"); },
    throwsInputFailure(Eq(2), EndsWith("oops")));

  EXPECT_THAT(
    [&] { throw InputFailure(2, "oops"); },
    throwsInputFailure(2, EndsWith("oops")));

  EXPECT_THAT(
    [&] { throw InputFailure(2, { { 1, 2 }, { 3, 4 } }, "oops"); },
    throwsInputFailure(Eq(2), Eq(str::Ranges { { 1, 2 }, { 3, 4 } }), EndsWith("oops")));

  EXPECT_THAT(
    [&] { throw InputFailure(2, { { 1, 2 }, { 3, 4 } }, "oops"); },
    throwsInputFailure(2, { { 1, 2 }, { 3, 4 } }, EndsWith("oops")));
}

// EOF
