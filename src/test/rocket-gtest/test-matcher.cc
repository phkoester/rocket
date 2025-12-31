/*
 * test-matcher.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/io.h"

#include "rocket-gtest/matcher.h"

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

TEST(match, throwsParseFailure) {
  auto is = io::is();

  EXPECT_THAT(
    [&] { throw io::ParseFailure(is, 2, "oops"); },
    throwsParseFailure(Eq(2), HasSubstr("oops")));

  EXPECT_THAT(
    [&] { throw io::ParseFailure(is, 2, "oops"); },
    throwsParseFailure(2, HasSubstr("oops")));

  EXPECT_THAT(
    [&] { throw io::ParseFailure(is, 2, { { 1, 2 }, { 3, 4 } }, "oops"); },
    throwsParseFailure(Eq(2), Eq(text::Ranges { { 1, 2 }, { 3, 4 } }), HasSubstr("oops")));

  EXPECT_THAT(
    [&] { throw io::ParseFailure(is, 2, { { 1, 2 }, { 3, 4 } }, "oops"); },
    throwsParseFailure(2, { { 1, 2 }, { 3, 4 } }, HasSubstr("oops")));
}

// EOF
