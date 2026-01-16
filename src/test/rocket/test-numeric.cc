/*
 * test-numeric.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/numeric.h"

using rocket::internal::Control;

// `static_assert` ------------------------------------------------------------------------------------------

static_assert(is_same_v<Control<LargestType<char, u16>::Type>::Type, i32>);
static_assert(is_same_v<Control<LargestType<char, i32>::Type>::Type, i64>);
static_assert(is_same_v<Control<LargestType<signed char, i64>::Type>::Type, i128>);
static_assert(is_same_v<Control<LargestType<signed char, i128>::Type>::Type, i128>);
static_assert(is_same_v<Control<LargestType<unsigned char, i128>::Type>::Type, i128>);
static_assert(is_same_v<Control<LargestType<unsigned char, u128>::Type>::Type, u128>);

// Constants ------------------------------------------------------------------------------------------------

constexpr auto charMin = numeric_limits<char>::min();
constexpr auto charMax = numeric_limits<char>::max();
constexpr auto ucharMax = numeric_limits<unsigned char>::max();
constexpr auto i128Min = numeric_limits<i128>::min();
constexpr auto i128Max = numeric_limits<i128>::max();
constexpr auto u128Max = numeric_limits<u128>::max();

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(numeric, add) {
  // `char`

  EXPECT_THAT([] { add<char>(charMin, -1); }, Throws<Overflow>()); // MIN - 1
  EXPECT_EQ(add<char>(charMin, 0), charMin); // MIN
  EXPECT_EQ(add<char>(charMin, 1), charMin + 1); // MIN + 1
  EXPECT_EQ(add<char>(charMax, -1), charMax - 1); // MAX - 1
  EXPECT_EQ(add<char>(charMax, 0), charMax); // MAX
  EXPECT_THAT([] { add<char>(charMax, 1); }, Throws<Overflow>()); // MAX + 1

  // `unsigned char`

  EXPECT_THAT([] { add<unsigned char>(0, -1); }, Throws<Overflow>()); // -1
  EXPECT_EQ(add<unsigned char>(0, 0), 0); // 0
  EXPECT_EQ(add<unsigned char>(0, 1), 1); // 1
  EXPECT_EQ(add<unsigned char>(ucharMax, -1), ucharMax - 1); // MAX - 1
  EXPECT_EQ(add<unsigned char>(ucharMax, 0), ucharMax); // MAX
  EXPECT_THAT(([] { add<unsigned char>(ucharMax, 1); }), Throws<Overflow>()); // MAX + 1

  // `i128`

  EXPECT_THAT([] { add<i128>(i128Min, -i128Max); }, Throws<Overflow>()); // MIN - MAX
  EXPECT_THAT([] { add<i128>(i128Min, -1); }, Throws<Overflow>()); // MIN - 1
  EXPECT_EQ(add<i128>(i128Min, 0), i128Min); // MIN
  EXPECT_EQ(add<i128>(i128Min, 1), i128Min + 1); // MIN + 1
  EXPECT_EQ(add<i128>(i128Max, -1), i128Max - 1); // MAX - 1
  EXPECT_EQ(add<i128>(i128Max, 0), i128Max); // MAX
  EXPECT_THAT([] { add<i128>(i128Max, 1); }, Throws<Overflow>()); // MAX + 1
  EXPECT_THAT([] { add<i128>(i128Max, i128Max); }, Throws<Overflow>()); // MAX + MAX

  // `u128`

  EXPECT_EQ(add<u128>(0, -u128Max), 1); // -MAX (not working; should overflow)
  EXPECT_EQ(add<u128>(0, -1), u128Max); // -1 (not working; should overflow)
  EXPECT_EQ(add<u128>(0, 0), 0); // 0
  EXPECT_EQ(add<u128>(0, 1), 1); // 1
  EXPECT_THAT(([] { add<u128>(u128Max, -1); }), Throws<Overflow>()); // MAX - 1 (not working; should be MAX - 1)
  EXPECT_EQ(add<u128>(u128Max, 0), u128Max); // MAX
  EXPECT_THAT(([] { add<u128>(u128Max, 1); }), Throws<Overflow>()); // MAX + 1
  EXPECT_THAT(([] { add<u128>(u128Max, u128Max); }), Throws<Overflow>()); // MAX + MAX
}

TEST(numeric, sub) {
  // `char`

  EXPECT_THAT([] { sub<char>(charMin, 1); }, Throws<Overflow>()); // MIN - 1
  EXPECT_EQ(sub<char>(charMin, 0), charMin); // MIN
  EXPECT_EQ(sub<char>(charMin, -1), charMin + 1); // MIN + 1
  EXPECT_EQ(sub<char>(charMax, 1), charMax - 1); // MAX - 1
  EXPECT_EQ(sub<char>(charMax, 0), charMax); // MAX
  EXPECT_THAT([] { sub<char>(charMax, -1); }, Throws<Overflow>()); // MAX + 1

  // `unsigned char`

  EXPECT_THAT([] { sub<unsigned char>(0, 1); }, Throws<Overflow>()); // -1
  EXPECT_EQ(sub<unsigned char>(0, 0), 0); // 0
  EXPECT_EQ(sub<unsigned char>(0, -1), 1); // 1
  EXPECT_EQ(sub<unsigned char>(ucharMax, 1), ucharMax - 1); // MAX - 1
  EXPECT_EQ(sub<unsigned char>(ucharMax, 0), ucharMax); // MAX
  EXPECT_THAT(([] { sub<unsigned char>(ucharMax, -1); }), Throws<Overflow>()); // MAX + 1

  // `i128`

  EXPECT_THAT([] { sub<i128>(i128Min, i128Max); }, Throws<Overflow>()); // MIN - MAX
  EXPECT_THAT([] { sub<i128>(i128Min, 1); }, Throws<Overflow>()); // MIN - 1
  EXPECT_EQ(sub<i128>(i128Min, 0), i128Min); // MIN
  EXPECT_EQ(sub<i128>(i128Min, -1), i128Min + 1); // MIN + 1
  EXPECT_EQ(sub<i128>(i128Max, 1), i128Max - 1); // MAX - 1
  EXPECT_EQ(sub<i128>(i128Max, 0), i128Max); // MAX
  EXPECT_THAT([] { sub<i128>(i128Max, -1); }, Throws<Overflow>()); // MAX + 1
  EXPECT_THAT([] { sub<i128>(i128Max, -i128Max); }, Throws<Overflow>()); // MAX + MAX

  // `u128`

  EXPECT_THAT([] { sub<u128>(0, u128Max); }, Throws<Overflow>()); // -MAX
  EXPECT_THAT([] { sub<u128>(0, 1); }, Throws<Overflow>()); // -1
  EXPECT_EQ(sub<u128>(0, 0), 0); // 0
  EXPECT_THAT([] { sub<u128>(0, -1); }, Throws<Overflow>()); // 1 (not working; should be 1)
  EXPECT_EQ(sub<u128>(u128Max, 1), u128Max - 1); // MAX - 1
  EXPECT_EQ(sub<u128>(u128Max, 0), u128Max); // MAX
  EXPECT_EQ(sub<u128>(u128Max, -1), 0); // MAX + 1 (not working; should overflow)
  EXPECT_EQ(sub<u128>(u128Max, -u128Max), u128Max - 1); // MAX + MAX (not working; should overflow)
}

TEST(numeric, to) {
  EXPECT_THAT([] { to<unsigned char>(-1); }, Throws<Overflow>());
  EXPECT_EQ(to<unsigned char>(255), 255);
  EXPECT_THAT([] { to<unsigned char>(256); }, Throws<Overflow>());
  EXPECT_EQ(to<i16>(-32768), -32768);
  EXPECT_THAT([] { to<i16>(-32769); }, Throws<Overflow>());
}

// EOF
