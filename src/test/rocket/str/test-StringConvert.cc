/*
 * test-StringConvert.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/enum.h"
#include "rocket/log/log.h"
#include "rocket/str/StringConvert.h"

using namespace rocket::str;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(StringConvert, bool) {
  using type = bool;

  EXPECT_EQ(toType<type>("false"), false);
  EXPECT_EQ(toType<type>("0"), false);
  EXPECT_EQ(toType<type>("NIL"), false);
  EXPECT_EQ(toType<type>("NONE"), false);
  EXPECT_EQ(toType<type>("NULL"), false);
  EXPECT_EQ(toType<type>("oFF"), false);
  EXPECT_EQ(toType<type>("falsch"), true);
  EXPECT_EQ(toType<type>("1"), true);
  EXPECT_EQ(toType<type>("42"), true);
}

TEST(StringConvert, i32) {
  using type = i32;

  EXPECT_EQ(toType<type>("-999999"), -999'999);
  EXPECT_EQ(toType<type>("-2147483648"), numeric_limits<type>::min());

  EXPECT_EQ(toType<type>("foo").error().message, "Cannot scan \"foo\" as `int`");
  EXPECT_EQ(toType<type>("1x").error().message, "Cannot scan \"1x\" as `int`");
}

TEST(StringConvert, i128) {
  using type = i128;

  EXPECT_EQ(toType<type>("-999999"), -999'999);
  EXPECT_EQ(toType<type>("-170141183460469231731687303715884105728"), numeric_limits<type>::min());

  EXPECT_THAT(
    toType<type>("foo").error().message,
    matchesRegex("Cannot scan \"foo\" as `.*int128`"));
  EXPECT_THAT(
    toType<type>("1x").error().message,
    matchesRegex("Cannot scan \"1x\" as `.*int128`"));
}

TEST(StringConvert, enum) {
  using type = log::LogLevel;

  EXPECT_EQ(toType<type>("trace"), log::LogLevel::trace);

  EXPECT_EQ(toType<type>("foo").error().message, "Cannot scan \"foo\" as `rocket::log::LogLevel`");
  EXPECT_EQ(toType<type>("tracex").error().message, "Cannot scan \"tracex\" as `rocket::log::LogLevel`");
}

// EOF
