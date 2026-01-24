/*
 * test-fmt.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/format/format.h"

#include <limits>

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(fmt, boolFormat) {
  EXPECT_EQ(fmt::format("{}", false), "false");
  EXPECT_EQ(fmt::format("{}", true), "true");
}

TEST(fmt, charFormat) {
  EXPECT_EQ(fmt::format("{}", 'a'), "a");
  EXPECT_EQ(fmt::format("{:?}", 'a'), "'a'");
  EXPECT_EQ(fmt::format("{}", '\x01'), "\x01");
  EXPECT_EQ(fmt::format("{:?}", '\x01'), "'\\x01'");
  EXPECT_EQ(fmt::format("{:?}", '\xff'), "'\\xff'");
}

TEST(fmt, unsignedCharFormat) {
  EXPECT_EQ(fmt::format("{}", static_cast<unsigned char>('a')), "97");
  EXPECT_EQ(fmt::format("{:X}", static_cast<unsigned char>(-1)), "FF");
  EXPECT_EQ(fmt::format("{:#x}", static_cast<unsigned char>(-1)), "0xff");
}

TEST(fmt, char32Format) {
  EXPECT_EQ(fmt::format(U"{}", U'a'), U"a");
  EXPECT_EQ(fmt::format(U"{:?}", U'€'), U"'€'");
}

TEST(fmt, i8Format) {
  EXPECT_EQ(fmt::format("{:+}", static_cast<i8>(42)), "+42");
}

TEST(fmt, u8Format) {
  EXPECT_EQ(fmt::format("{}", static_cast<u8>(-1)), "255");
}

TEST(fmt, i16Format) {
  EXPECT_EQ(fmt::format("{:+}", static_cast<i16>(42)), "+42");
}

TEST(fmt, u16Format) {
  EXPECT_EQ(fmt::format("{}", static_cast<u16>(-1)), "65535");
}

TEST(fmt, i32Format) {
  EXPECT_EQ(fmt::format("{:+}", static_cast<i32>(42)), "+42");
}

TEST(fmt, u32Format) {
  EXPECT_EQ(fmt::format("{}", static_cast<u32>(-1)), "4294967295");
}

TEST(fmt, i64Format) {
  EXPECT_EQ(fmt::format("{:+}", static_cast<i64>(42)), "+42");
}

TEST(fmt, u64Format) {
  EXPECT_EQ(fmt::format("{}", static_cast<u64>(-1)), "18446744073709551615");
}

TEST(fmt, i28Format) {
  EXPECT_EQ(fmt::format("{}", static_cast<i128>(42)), "42"); // XXX
}

TEST(fmt, u128Format) {
  EXPECT_EQ(fmt::format("{}", static_cast<u128>(-1)), "340282366920938463463374607431768211455");
}

TEST(fmt, f32Format) {
  EXPECT_EQ(fmt::format("{}", numeric_limits<f32>::quiet_NaN()), "nan");
  EXPECT_EQ(fmt::format("{}", numeric_limits<f32>::signaling_NaN()), "nan");
  EXPECT_EQ(fmt::format("{}", numeric_limits<f32>::infinity()), "inf");
  EXPECT_EQ(fmt::format("{}", -numeric_limits<f32>::infinity()), "-inf");

  EXPECT_EQ(fmt::format("{:.5}", 0.999'999f), "1");
  EXPECT_EQ(fmt::format("{:.5}", 0.999'99f), "0.99999");
  EXPECT_EQ(fmt::format("{:.3f}", 12.1236f), "12.124"); // Round to 3 significant digits after the decimal point
  EXPECT_EQ(fmt::format("{:.5}", 1.0f / 3), "0.33333");
}

TEST(fmt, f64Format) {
  EXPECT_EQ(fmt::format("{:.5}", 0.999'999), "1");
  EXPECT_EQ(fmt::format("{:.5}", 0.999'99), "0.99999");
  EXPECT_EQ(fmt::format("{:.3f}", 12.1236), "12.124"); // Round to 3 significant digits after the decimal point
  EXPECT_EQ(fmt::format("{:.5}", 1.0 / 3), "0.33333");
}

TEST(fmt, f128Format) {
  EXPECT_EQ(fmt::format("{:.5}", 0.999'999L), "1");
  EXPECT_EQ(fmt::format("{:.5}", 0.999'99L), "0.99999");
  EXPECT_EQ(fmt::format("{:.3f}", 12.1236L), "12.124"); // Round to 3 significant digits after the decimal point
  EXPECT_EQ(fmt::format("{:.5}", 1.0L / 3), "0.33333");
}

TEST(fmt, charPtrFormat) {
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

TEST(fmt, char32PtrFormat) {
  EXPECT_EQ(fmt::format(U"{}", U"hello"), U"hello");
}

// EOF
