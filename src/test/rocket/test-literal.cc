/*
 * test-literal.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Exception.h"
#include "rocket/literal.h"

#include <cmath>

namespace {

namespace lit = rocket::internal::literal;

// Local functions ------------------------------------------------------------------------------------------

// An integer literal that is out of range fails a `static_assert` in its literal operator, which cannot be
// tested without breaking the build. These operators expose the underlying check instead

template<typename T, typename U, char... C>
constexpr bool VALID_SIGNED = lit::SignedInteger<T, U, C...>::valid;

template<typename T, char... C>
constexpr bool VALID_UNSIGNED = lit::UnsignedInteger<T, C...>::valid;

template<char... C> consteval bool operator""_validI8() { return VALID_SIGNED<i8, u8, C...>; }
template<char... C> consteval bool operator""_validU8() { return VALID_UNSIGNED<u8, C...>; }
template<char... C> consteval bool operator""_validI16() { return VALID_SIGNED<i16, u16, C...>; }
template<char... C> consteval bool operator""_validU16() { return VALID_UNSIGNED<u16, C...>; }
template<char... C> consteval bool operator""_validI32() { return VALID_SIGNED<i32, u32, C...>; }
template<char... C> consteval bool operator""_validU32() { return VALID_UNSIGNED<u32, C...>; }
template<char... C> consteval bool operator""_validI64() { return VALID_SIGNED<i64, u64, C...>; }
template<char... C> consteval bool operator""_validU64() { return VALID_UNSIGNED<u64, C...>; }
template<char... C> consteval bool operator""_validI128() { return VALID_SIGNED<i128, u128, C...>; }
template<char... C> consteval bool operator""_validU128() { return VALID_UNSIGNED<u128, C...>; }

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(literal, integerTypes) {
  static_assert(same_as<decltype(1_i8), i8>);
  static_assert(same_as<decltype(1_u8), u8>);
  static_assert(same_as<decltype(1_i16), i16>);
  static_assert(same_as<decltype(1_u16), u16>);
  static_assert(same_as<decltype(1_i32), i32>);
  static_assert(same_as<decltype(1_u32), u32>);
  static_assert(same_as<decltype(1_i64), i64>);
  static_assert(same_as<decltype(1_u64), u64>);
  static_assert(same_as<decltype(1_i128), i128>);
  static_assert(same_as<decltype(1_u128), u128>);
}

TEST(literal, integerIsConstexpr) {
  static_assert(42_i32 == 42);
  static_assert(0xFF_u8 == 255);
  static_assert(0b101_i32 == 5);
  static_assert(017_u16 == 15);
}

TEST(literal, integerZero) {
  EXPECT_EQ(0_i8, 0);
  EXPECT_EQ(0_u8, 0);
  EXPECT_EQ(0_i32, 0);
  EXPECT_EQ(0_u64, 0U);
  EXPECT_EQ(0_i128, static_cast<i128>(0));
  EXPECT_EQ(0_u128, static_cast<u128>(0));

  EXPECT_EQ(00_i32, 0);
  EXPECT_EQ(0x0_i32, 0);
  EXPECT_EQ(0b0_i32, 0);
}

TEST(literal, integerLimits) {
  EXPECT_EQ(127_i8, numeric_limits<i8>::max());
  EXPECT_EQ(255_u8, numeric_limits<u8>::max());
  EXPECT_EQ(32767_i16, numeric_limits<i16>::max());
  EXPECT_EQ(65535_u16, numeric_limits<u16>::max());
  EXPECT_EQ(2147483647_i32, numeric_limits<i32>::max());
  EXPECT_EQ(4294967295_u32, numeric_limits<u32>::max());
  EXPECT_EQ(9223372036854775807_i64, numeric_limits<i64>::max());
  EXPECT_EQ(18446744073709551615_u64, numeric_limits<u64>::max());
  EXPECT_EQ(170141183460469231731687303715884105727_i128, numeric_limits<i128>::max());
  EXPECT_EQ(340282366920938463463374607431768211455_u128, numeric_limits<u128>::max());
}

TEST(literal, integerHexadecimal) {
  EXPECT_EQ(0x7F_i8, 127);
  EXPECT_EQ(0X7F_i8, 127);
  EXPECT_EQ(0xFF_u8, 255);
  EXPECT_EQ(0XFF_u8, 255);

  // Digits in both cases
  EXPECT_EQ(0xaB_u16, 171);
  EXPECT_EQ(0xAb_i16, 171);

  EXPECT_EQ(0x7FFF'FFFF'FFFF'FFFF'FFFF'FFFF'FFFF'FFFF_i128, numeric_limits<i128>::max());
  EXPECT_EQ(0XFFFF'FFFF'FFFF'FFFF'FFFF'FFFF'FFFF'FFFF_u128, numeric_limits<u128>::max());
}

TEST(literal, integerBinary) {
  EXPECT_EQ(0b1111111_i8, 127);
  EXPECT_EQ(0B1111111_i8, 127);
  EXPECT_EQ(0b11111111_u8, 255);
  EXPECT_EQ(0B11111111_u8, 255);

  EXPECT_EQ(0b101_i32, 5);
  EXPECT_EQ(0B101_i32, 5);
  EXPECT_EQ(0b1000'0000'0000'0000'0000'0000'0000'0000_u32, 2147483648U);
}

TEST(literal, integerOctal) {
  EXPECT_EQ(0177_i8, 127);
  EXPECT_EQ(0377_u8, 255);
  EXPECT_EQ(017_i32, 15);
  EXPECT_EQ(017_u32, 15U);
}

TEST(literal, integerDigitSeparators) {
  EXPECT_EQ(1'000_i32, 1000);
  EXPECT_EQ(1'000'000_u64, 1000000U);
  EXPECT_EQ(0xFF'FF_u16, 65535);
  EXPECT_EQ(0b1111'0000_u8, 240);
  EXPECT_EQ(0'17_u8, 15);
}

TEST(literal, integerValidityDecimal) {
  EXPECT_TRUE(127_validI8);
  EXPECT_FALSE(128_validI8);
  EXPECT_TRUE(255_validU8);
  EXPECT_FALSE(256_validU8);

  EXPECT_TRUE(32767_validI16);
  EXPECT_FALSE(32768_validI16);
  EXPECT_TRUE(65535_validU16);
  EXPECT_FALSE(65536_validU16);

  EXPECT_TRUE(2147483647_validI32);
  EXPECT_FALSE(2147483648_validI32);
  EXPECT_TRUE(4294967295_validU32);
  EXPECT_FALSE(4294967296_validU32);

  EXPECT_TRUE(9223372036854775807_validI64);
  EXPECT_FALSE(9223372036854775808_validI64);
  EXPECT_TRUE(18446744073709551615_validU64);
  EXPECT_FALSE(18446744073709551616_validU64);

  EXPECT_TRUE(170141183460469231731687303715884105727_validI128);
  EXPECT_FALSE(170141183460469231731687303715884105728_validI128);
  EXPECT_TRUE(340282366920938463463374607431768211455_validU128);
  EXPECT_FALSE(340282366920938463463374607431768211456_validU128);
}

TEST(literal, integerValidityFarOutOfRange) {
  // The value must not wrap around and appear valid
  EXPECT_FALSE(1000_validU8);
  EXPECT_FALSE(65792_validU8); // 0x10100
  EXPECT_FALSE(99999999999999999999999_validU64);
  EXPECT_FALSE(3402823669209384634633746074317682114560_validU128);
}

TEST(literal, integerValidityOtherBases) {
  EXPECT_TRUE(0x7F_validI8);
  EXPECT_FALSE(0x80_validI8);
  EXPECT_TRUE(0X7F_validI8);
  EXPECT_FALSE(0X80_validI8);
  EXPECT_TRUE(0xFF_validU8);
  EXPECT_FALSE(0x100_validU8);
  EXPECT_TRUE(0XFF_validU8);
  EXPECT_FALSE(0X100_validU8);

  EXPECT_TRUE(0b1111111_validI8);
  EXPECT_FALSE(0b10000000_validI8);
  EXPECT_TRUE(0B1111111_validI8);
  EXPECT_FALSE(0B10000000_validI8);
  EXPECT_TRUE(0b11111111_validU8);
  EXPECT_FALSE(0b100000000_validU8);
  EXPECT_TRUE(0B11111111_validU8);
  EXPECT_FALSE(0B100000000_validU8);

  EXPECT_TRUE(0177_validI8);
  EXPECT_FALSE(0200_validI8);
  EXPECT_TRUE(0377_validU8);
  EXPECT_FALSE(0400_validU8);
}

TEST(literal, floatTypes) {
  static_assert(same_as<decltype(1_f32), f32>);
  static_assert(same_as<decltype(1.0_f32), f32>);
  static_assert(same_as<decltype(1_f64), f64>);
  static_assert(same_as<decltype(1.0_f64), f64>);
}

TEST(literal, floatZero) {
  EXPECT_EQ(0.0_f32, 0.0F);
  EXPECT_EQ(.0_f32, 0.0F);
  EXPECT_EQ(0e0_f32, 0.0F);
  EXPECT_EQ(0.0e10_f32, 0.0F);

  EXPECT_EQ(0.0_f64, 0.0);
  EXPECT_EQ(.0_f64, 0.0);
  EXPECT_EQ(0e0_f64, 0.0);
  EXPECT_EQ(0.0e10_f64, 0.0);
}

TEST(literal, floatNegativeZero) {
  // A literal is never negative, but the operators can be called directly
  EXPECT_EQ(operator""_f32(-0.0L), 0.0F);
  EXPECT_TRUE(signbit(operator""_f32(-0.0L)));
  EXPECT_EQ(operator""_f64(-0.0L), 0.0);
  EXPECT_TRUE(signbit(operator""_f64(-0.0L)));
}

TEST(literal, floatValues) {
  EXPECT_EQ(1.5_f32, 1.5F);
  EXPECT_EQ(-1.5_f32, -1.5F);
  EXPECT_EQ(1'000.5_f32, 1000.5F);
  EXPECT_EQ(0x1p3_f32, 8.0F);
  EXPECT_FLOAT_EQ(0.1_f32, 0.1F);
  EXPECT_FLOAT_EQ(1e10_f32, 1e10F);

  EXPECT_EQ(1.5_f64, 1.5);
  EXPECT_EQ(-1.5_f64, -1.5);
  EXPECT_EQ(1'000.5_f64, 1000.5);
  EXPECT_EQ(0x1p3_f64, 8.0);
  EXPECT_DOUBLE_EQ(0.1_f64, 0.1);
  EXPECT_DOUBLE_EQ(1e100_f64, 1e100);
}

TEST(literal, floatFromInteger) {
  EXPECT_EQ(0_f32, 0.0F);
  EXPECT_EQ(42_f32, 42.0F);
  EXPECT_EQ(16777216_f32, 16777216.0F);
  EXPECT_FLOAT_EQ(18446744073709551615_f32, 18446744073709551616.0F);

  EXPECT_EQ(0_f64, 0.0);
  EXPECT_EQ(42_f64, 42.0);
  EXPECT_EQ(9007199254740992_f64, 9007199254740992.0);
  EXPECT_DOUBLE_EQ(18446744073709551615_f64, 18446744073709551616.0);
}

TEST(literal, floatLimits) {
  using limitsF32 = numeric_limits<f32>;
  using limitsF64 = numeric_limits<f64>;

  EXPECT_EQ(operator""_f32(static_cast<std_long_double>(limitsF32::max())), limitsF32::max());
  EXPECT_EQ(operator""_f32(static_cast<std_long_double>(limitsF32::lowest())), limitsF32::lowest());
  EXPECT_EQ(operator""_f32(static_cast<std_long_double>(limitsF32::min())), limitsF32::min());

  EXPECT_EQ(operator""_f64(static_cast<std_long_double>(limitsF64::max())), limitsF64::max());
  EXPECT_EQ(operator""_f64(static_cast<std_long_double>(limitsF64::lowest())), limitsF64::lowest());
  EXPECT_EQ(operator""_f64(static_cast<std_long_double>(limitsF64::min())), limitsF64::min());
}

TEST(literal, floatSubnormal) {
  using limitsF32 = numeric_limits<f32>;
  using limitsF64 = numeric_limits<f64>;

  const f32 a = 1e-40_f32;
  EXPECT_EQ(fpclassify(a), FP_SUBNORMAL);
  EXPECT_GT(a, 0.0F);
  EXPECT_LT(a, limitsF32::min());

  const f64 b = 1e-310_f64;
  EXPECT_EQ(fpclassify(b), FP_SUBNORMAL);
  EXPECT_GT(b, 0.0);
  EXPECT_LT(b, limitsF64::min());

  EXPECT_EQ(1.4e-45_f32, limitsF32::denorm_min());
  EXPECT_EQ(4.9e-324_f64, limitsF64::denorm_min());

  EXPECT_EQ(operator""_f32(static_cast<std_long_double>(limitsF32::denorm_min())), limitsF32::denorm_min());
  EXPECT_EQ(operator""_f64(static_cast<std_long_double>(limitsF64::denorm_min())), limitsF64::denorm_min());
}

TEST(literal, floatRoundsUpToSmallestSubnormal) {
  // Three quarters of the smallest subnormal value is closer to that value than to zero
  const auto val = static_cast<std_long_double>(numeric_limits<f32>::denorm_min()) * 3 / 4;
  EXPECT_EQ(operator""_f32(val), numeric_limits<f32>::denorm_min());
}

TEST(literal, floatUnderflowF32) {
  EXPECT_THAT([] { static_cast<void>(1e-50_f32); }, Throws<Underflow>());

  // A quarter of the smallest subnormal value rounds to zero
  const auto val = static_cast<std_long_double>(numeric_limits<f32>::denorm_min()) / 4;
  EXPECT_THAT([&] { static_cast<void>(operator""_f32(val)); }, Throws<Underflow>());
  EXPECT_THAT([&] { static_cast<void>(operator""_f32(-val)); }, Throws<Underflow>());
}

TEST(literal, floatUnderflowF64) {
  // This is only possible if `long double` has more precision than `f64`, which isn’t the case everywhere
  if (numeric_limits<std_long_double>::digits <= numeric_limits<f64>::digits) {
    GTEST_SKIP() << "`long double` is no wider than `f64`";
  }

  const auto val = static_cast<std_long_double>(numeric_limits<f64>::denorm_min()) / 4;
  EXPECT_THAT([&] { static_cast<void>(operator""_f64(val)); }, Throws<Underflow>());
  EXPECT_THAT([&] { static_cast<void>(operator""_f64(-val)); }, Throws<Underflow>());
}

TEST(literal, floatOverflowF32) {
  EXPECT_THAT([] { static_cast<void>(1e39_f32); }, Throws<Overflow>());

  // The next representable value above the greatest `f32`
  const auto val = nextafter(
    static_cast<std_long_double>(numeric_limits<f32>::max()),
    numeric_limits<std_long_double>::infinity());
  EXPECT_THAT([&] { static_cast<void>(operator""_f32(val)); }, Throws<Overflow>());
  EXPECT_THAT([&] { static_cast<void>(operator""_f32(-val)); }, Throws<Overflow>());
}

TEST(literal, floatOverflowF64) {
  // Where `long double` is no wider than `f64`, this is infinity, which must be rejected as well
  const auto val = nextafter(
    static_cast<std_long_double>(numeric_limits<f64>::max()),
    numeric_limits<std_long_double>::infinity());
  EXPECT_THAT([&] { static_cast<void>(operator""_f64(val)); }, Throws<Overflow>());
  EXPECT_THAT([&] { static_cast<void>(operator""_f64(-val)); }, Throws<Overflow>());
}

TEST(literal, literalStringChar) {
  using type = LiteralString<char, 'a', 'b', 'c'>;

  static_assert(type().size() == 3);
  EXPECT_EQ(type::value[0], 'a');
  EXPECT_EQ(type::value[2], 'c');

  const string_view view = type();
  EXPECT_EQ(view, "abc"sv);
}

TEST(literal, literalStringChar32) {
  using type = LiteralString<char32, U'a', U'€'>;

  static_assert(type().size() == 2);

  const u32string_view view = type();
  EXPECT_EQ(view, U"a€"sv);
}

TEST(literal, literalStringEmpty) {
  using type = LiteralString<char>;

  static_assert(type().size() == 0); // NOLINT(*-container-size-empty)

  const string_view view = type();
  EXPECT_TRUE(view.empty());
}

TEST(literal, literalStringIsConstexpr) {
  static constexpr string_view VIEW = LiteralString<char, 'x', 'y'>();
  static_assert(VIEW == "xy"sv);
}

// EOF
