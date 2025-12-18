/*
 * test-matcher.cc
 */

#include "rocket-gtest/testing.h"

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
    [&] { throw except::ParseFailure<char>(is, 2, "oops"); },
    throwsParseFailure<char>(Eq(2), HasSubstr("oops")));

  EXPECT_THAT(
    [&] { throw except::ParseFailure<char>(is, 2, "oops"); },
    throwsParseFailure<char>(2, HasSubstr("oops")));

  EXPECT_THAT(
    [&] { throw except::ParseFailure<char>(is, 2, { { 1, 2 }, { 3, 4 } }, "oops"); },
    throwsParseFailure<char>(Eq(2), Eq(text::Ranges { { 1, 2 }, { 3, 4 } }), HasSubstr("oops")));

  EXPECT_THAT(
    [&] { throw except::ParseFailure<char>(is, 2, { { 1, 2 }, { 3, 4 } }, "oops"); },
    throwsParseFailure<char>(2, { { 1, 2 }, { 3, 4 } }, HasSubstr("oops")));
}

// EOF
