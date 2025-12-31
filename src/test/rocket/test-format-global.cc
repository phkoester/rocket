/*
 * test-format-global.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/rocket.h"
#include "rocket/format.h"

#include <limits>
#include <type_traits>

using namespace std;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(format_global, boolFormat) {
  EXPECT_EQ(fmt::format("{}", false), "false");
  EXPECT_EQ(fmt::format("{}", true), "true");
}

TEST(format_global, charFormat) {
  EXPECT_EQ(fmt::format("{}", 'a'), "a");
  EXPECT_EQ(fmt::format("{:?}", 'a'), "'a'");
  EXPECT_EQ(fmt::format("{}", '\x01'), "\x01");
  EXPECT_EQ(fmt::format("{:?}", '\x01'), "'\\x01'");
  EXPECT_EQ(fmt::format("{:?}", '\xff'), "'\\xff'");
}

TEST(format_global, unsignedCharFormat) {
  EXPECT_EQ(fmt::format("{}", static_cast<unsigned char>('a')), "97");
  EXPECT_EQ(fmt::format("{:X}", static_cast<unsigned char>(-1)), "FF");
  EXPECT_EQ(fmt::format("{:#x}", static_cast<unsigned char>(-1)), "0xff");
}

// No support for `char32_t`!

TEST(format_global, int8Format) {
  EXPECT_EQ(fmt::format("{:+}", static_cast<int8_t>(42)), "+42");
}

TEST(format_global, uint8Format) {
  EXPECT_EQ(fmt::format("{}", static_cast<uint8_t>(-1)), "255");
}

TEST(format_global, int16Format) {
  EXPECT_EQ(fmt::format("{:+}", static_cast<int16_t>(42)), "+42");
}

TEST(format_global, uint16Format) {
  EXPECT_EQ(fmt::format("{}", static_cast<uint16_t>(-1)), "65535");
}

TEST(format_global, int32Format) {
  EXPECT_EQ(fmt::format("{:+}", static_cast<int32_t>(42)), "+42");
}

TEST(format_global, uint32Format) {
  EXPECT_EQ(fmt::format("{}", static_cast<uint32_t>(-1)), "4294967295");
}

TEST(format_global, int64Format) {
  EXPECT_EQ(fmt::format("{:+}", static_cast<int64_t>(42)), "+42");
}

TEST(format_global, uint64Format) {
  EXPECT_EQ(fmt::format("{}", static_cast<uint64_t>(-1)), "18446744073709551615");
}

TEST(format_global, int128Format) {
  static_assert(sizeof(int128_t) == 16);
  EXPECT_EQ(fmt::format("{:+}", static_cast<int128_t>(42)), "+42");
}

TEST(format_global, uint128Format) {
  static_assert(sizeof(uint128_t) == 16);
  EXPECT_EQ(fmt::format("{}", static_cast<uint128_t>(-1)), "340282366920938463463374607431768211455");
}

TEST(format_global, floatFormat) {
  static_assert(sizeof(float) == 4);

  EXPECT_EQ(fmt::format("{}", numeric_limits<float>::quiet_NaN()), "nan");
  EXPECT_EQ(fmt::format("{}", numeric_limits<float>::signaling_NaN()), "nan");
  EXPECT_EQ(fmt::format("{}", numeric_limits<float>::infinity()), "inf");
  EXPECT_EQ(fmt::format("{}", -numeric_limits<float>::infinity()), "-inf");

  EXPECT_EQ(fmt::format("{:.5}", 0.999'999f), "1");
  EXPECT_EQ(fmt::format("{:.5}", 0.999'99f), "0.99999");
  EXPECT_EQ(fmt::format("{:.3f}", 12.1236f), "12.124"); // Round to 3 significant digits after the decimal point
  static_assert(is_same_v<decltype(1.0f / 3), float> == true);
  EXPECT_EQ(fmt::format("{:.5}", 1.0f / 3), "0.33333");
}

TEST(format_global, doubleFormat) {
  static_assert(sizeof(double) == 8);

  EXPECT_EQ(fmt::format("{:.5}", 0.999'999), "1");
  EXPECT_EQ(fmt::format("{:.5}", 0.999'99), "0.99999");
  EXPECT_EQ(fmt::format("{:.3f}", 12.1236), "12.124"); // Round to 3 significant digits after the decimal point
  static_assert(is_same_v<decltype(1.0 / 3), double> == true);
  EXPECT_EQ(fmt::format("{:.5}", 1.0 / 3), "0.33333");
}

TEST(format_global, longDoubleFormat) {
  static_assert(sizeof(long double) == 16);

  EXPECT_EQ(fmt::format("{:.5}", 0.999'999L), "1");
  EXPECT_EQ(fmt::format("{:.5}", 0.999'99L), "0.99999");
  EXPECT_EQ(fmt::format("{:.3f}", 12.1236L), "12.124"); // Round to 3 significant digits after the decimal point
  static_assert(is_same_v<decltype(1.0L / 3), long double> == true);
  EXPECT_EQ(fmt::format("{:.5}", 1.0L / 3), "0.33333");
}

TEST(format_global, charPtrFormat) {
  EXPECT_EQ(fmt::format("{}", "hello"), "hello");
  EXPECT_EQ(fmt::format("{:?}", "hello"), "\"hello\"");
  EXPECT_EQ(fmt::format("{}", "a\bc"), "a\bc");
  EXPECT_EQ(fmt::format("{:?}", "a\bc"), "\"a\\x08c\"");
  EXPECT_EQ(fmt::format("{}", "⊕"), "⊕");
  EXPECT_EQ(fmt::format("{:?}", "⊕"), "\"⊕\"");
  // U+01F9D1 (Adult), U+200D (ZWJ), U+01F33E (Ear of rice)
  EXPECT_EQ(fmt::format("{}", "🧑‍🌾"), "🧑‍🌾");
  EXPECT_EQ(fmt::format("{:?}", "🧑‍🌾"), "\"🧑\\u200d🌾\"");
}

// EOF
