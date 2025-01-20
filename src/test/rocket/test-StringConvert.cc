/*
 * test-StringConvert.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-std-decl.h"
#include "rocket/codec-std.h"

#include "rocket/StringConvert.h"
#include "rocket/except.h"
#include "rocket/log.h"

#include "rocket-gtest/match.h"

#include <limits>

using namespace rocket;
using namespace rocket::gtest::match;
using namespace std;
using namespace testing;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(StringConvert, bool) {
  using type = bool;

  EXPECT_EQ(stringToType<type>("0"), false);
  EXPECT_EQ(stringToType<type>("false"), false);
  EXPECT_EQ(stringToType<type>("1"), true);
  EXPECT_EQ(stringToType<type>("true"), true);

  EXPECT_THAT(
      [] { stringToType<type>("foo"); },
      throwsParseFailure<char>(1, { 0, 2 }, HasSubstr("\"fo\" does not match any of {\"0\", \"1\", \"false\", \"true\"}")));
  EXPECT_THAT(
      [] { stringToType<type>("falsex"); },
      throwsParseFailure<char>(0, { 0, 6 }, HasSubstr("Cannot parse \"falsex\" as `bool`")));

  EXPECT_EQ(typeToString(false), "false");
  EXPECT_EQ(typeToString(true), "true");
}

TEST(StringConvert, int128_t) {
  using type = int128_t;

  EXPECT_EQ(stringToType<type>("-999'999"), -999'999);
  EXPECT_EQ(stringToType<type>("-170141183460469231731687303715884105728"), numeric_limits<type>::min());

  EXPECT_THAT(
      [] { stringToType<type>("foo"); },
      throwsParseFailure<char>(0, { 0, 3 }, HasSubstr("Cannot parse \"foo\" as `__int128`")));
  EXPECT_THAT(
      [] { stringToType<type>("1x"); },
      throwsParseFailure<char>(0, { 0, 2 }, HasSubstr("Cannot parse \"1x\" as `__int128`")));

  EXPECT_EQ(typeToString(type(-999'999)), "-999'999");
}

TEST(StringConvert, LogLevel) {
  using type = log::LogLevel;

  EXPECT_EQ(stringToType<type>("trace"), log::LogLevel::trace);

  EXPECT_THAT(
      [] { stringToType<type>("foo"); },
      throwsParseFailure<char>(0, { 0, 3 }, HasSubstr("Cannot parse \"foo\" as `rocket::log::LogLevel")));
  EXPECT_THAT(
      [] { stringToType<type>("tracex"); },
      throwsParseFailure<char>(0, { 0, 6 }, HasSubstr("Cannot parse \"tracex\" as `rocket::log::LogLevel`")));

  EXPECT_EQ(typeToString(log::LogLevel::debug), "debug");

  EXPECT_THAT(
      [] { typeToString(static_cast<type>(6)); },
      ThrowsMessage<except::InvalidArgument>(HasSubstr("Invalid `rocket::log::LogLevel`: 6")));
}

TEST(StringConvert, long_double) {
  using type = long double;

  const numeric_limits<type> limits;

  EXPECT_EQ(stringToType<type>("-inf"), -limits.infinity());

  EXPECT_EQ(typeToString(limits.quiet_NaN()), "qnan");
  EXPECT_EQ(typeToString(limits.signaling_NaN()), "snan");
}

// EOF
