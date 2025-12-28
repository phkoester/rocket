/*
 * test-StringConvert.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/StringConvert.h"
#include "rocket/log.h"

#include "rocket-gtest/matcher.h"

using namespace rocket;
using namespace rocket::gtest::matcher;
using namespace std;
using namespace testing;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(StringConvert, bool) {
  using type = bool;

  EXPECT_EQ(toType<type>("false"), false);
  EXPECT_EQ(toType<type>("0"), false);
  EXPECT_EQ(toType<type>("true"), true);
  EXPECT_EQ(toType<type>("1"), true);

  EXPECT_THAT(
      [] { toType<type>("foo"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot parse \"foo\" as `bool`")));
  EXPECT_THAT(
      [] { toType<type>("falsex"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot parse \"falsex\" as `bool`")));
}

TEST(StringConvert, int) {
  using type = int;

  EXPECT_EQ(toType<type>("-999999"), -999'999);
  EXPECT_EQ(toType<type>("-2147483648"), numeric_limits<type>::min());

  EXPECT_THAT(
      [] { toType<type>("foo"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot parse \"foo\" as `int`")));
  EXPECT_THAT(
      [] { toType<type>("1x"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot parse \"1x\" as `int`")));
}

TEST(StringConvert, int128) {
  using type = int128_t;

  EXPECT_EQ(toType<type>("-999999"), -999'999);
  EXPECT_EQ(toType<type>("-170141183460469231731687303715884105728"), numeric_limits<type>::min());

  EXPECT_THAT(
      [] { toType<type>("foo"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot parse \"foo\" as `__int128`")));
  EXPECT_THAT(
      [] { toType<type>("1x"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot parse \"1x\" as `__int128`")));
}

TEST(StringConvert, longDouble) {
  using type = long double;

  const numeric_limits<type> limits;

  EXPECT_EQ(toType<type>("-inf"), -limits.infinity());
  EXPECT_EQ(toType<type>("inf"), limits.infinity());
  EXPECT_EQ(toType<type>("+inf"), limits.infinity());
  EXPECT_TRUE(isnan(toType<type>("nan")));
  EXPECT_TRUE(isnan(toType<type>("snan")));
  EXPECT_EQ(toType<type>("-12.34"), -12.34L);

  EXPECT_THAT(
      [] { toType<type>("foo"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot parse \"foo\" as `long double`")));
  EXPECT_THAT(
      [] { toType<type>("1x"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot parse \"1x\" as `long double`")));
}

TEST(StringConvert, LogLevel) {
  using type = log::LogLevel;

  EXPECT_EQ(toType<type>("trace"), log::LogLevel::trace);

  EXPECT_THAT(
      [] { toType<type>("foo"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot parse \"foo\" as `rocket::log::LogLevel")));
  EXPECT_THAT(
      [] { toType<type>("tracex"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot parse \"tracex\" as `rocket::log::LogLevel`")));
}

// EOF
