/*
 * test-StringConvert.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/enum.h"
#include "rocket/log/log.h"
#include "rocket/str/StringConvert.h"

using namespace rocket::str;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(StringConvert, bool) {
  using type = bool;

  EXPECT_EQ(toType<type>("false"), false);
  EXPECT_EQ(toType<type>("0"), false);
  EXPECT_EQ(toType<type>("true"), true);
  EXPECT_EQ(toType<type>("1"), true);

  EXPECT_THAT(
      [] { toType<type>("foo"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"foo\" as `bool`")));
  EXPECT_THAT(
      [] { toType<type>("falsex"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"falsex\" as `bool`")));
}

TEST(StringConvert, i32) {
  using type = i32;

  EXPECT_EQ(toType<type>("-999999"), -999'999);
  EXPECT_EQ(toType<type>("-2147483648"), numeric_limits<type>::min());

  EXPECT_THAT(
      [] { toType<type>("foo"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"foo\" as `int`")));
  EXPECT_THAT(
      [] { toType<type>("1x"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"1x\" as `int`")));
}

TEST(StringConvert, i128) {
  using type = i128;

  EXPECT_EQ(toType<type>("-999999"), -999'999);
  EXPECT_EQ(toType<type>("-170141183460469231731687303715884105728"), numeric_limits<type>::min());

  EXPECT_THAT(
      [] { toType<type>("foo"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"foo\" as `__int128`")));
  EXPECT_THAT(
      [] { toType<type>("1x"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"1x\" as `__int128`")));
}

TEST(StringConvert, f128) {
  using type = f128;

  const numeric_limits<type> limits;

  EXPECT_EQ(toType<type>("-inf"), -limits.infinity());
  EXPECT_EQ(toType<type>("inf"), limits.infinity());
  EXPECT_EQ(toType<type>("+inf"), limits.infinity());
  EXPECT_TRUE(isnan(toType<type>("nan")));
  EXPECT_TRUE(isnan(toType<type>("snan")));
  EXPECT_EQ(toType<type>("-12.34"), -12.34L);

  EXPECT_THAT(
      [] { toType<type>("foo"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"foo\" as `long double`")));
  EXPECT_THAT(
      [] { toType<type>("1x"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"1x\" as `long double`")));
}

TEST(StringConvert, enum) {
  using type = log::LogLevel;

  EXPECT_EQ(toType<type>("trace"), log::LogLevel::trace);

  EXPECT_THAT(
      [] { toType<type>("foo"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"foo\" as `rocket::log::LogLevel`")));
  EXPECT_THAT(
      [] { toType<type>("tracex"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"tracex\" as `rocket::log::LogLevel`")));
}

// EOF
