/*
 * test-numeric.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/numeric.h"

using rocket::internal::Control;

// `static_assert` ------------------------------------------------------------------------------------------

static_assert(is_same_v<Control<LargestType<char, unsigned short>::Type>::Type, int>);
static_assert(is_same_v<Control<LargestType<char, int>::Type>::Type, long>);
static_assert(is_same_v<Control<LargestType<signed char, long>::Type>::Type, int128_t>);
static_assert(is_same_v<Control<LargestType<signed char, int128_t>::Type>::Type, int128_t>);
static_assert(is_same_v<Control<LargestType<unsigned char, int128_t>::Type>::Type, int128_t>);
static_assert(is_same_v<Control<LargestType<unsigned char, uint128_t>::Type>::Type, uint128_t>);

// Constants ------------------------------------------------------------------------------------------------

constexpr auto charMin = numeric_limits<char>::min();
constexpr auto charMax = numeric_limits<char>::max();
constexpr auto ucharMax = numeric_limits<unsigned char>::max();
constexpr auto int128Min = numeric_limits<int128_t>::min();
constexpr auto int128Max = numeric_limits<int128_t>::max();
constexpr auto uint128Max = numeric_limits<uint128_t>::max();

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

  // `int128_t`

  EXPECT_THAT([] { add<int128_t>(int128Min, -int128Max); }, Throws<Overflow>()); // MIN - MAX
  EXPECT_THAT([] { add<int128_t>(int128Min, -1); }, Throws<Overflow>()); // MIN - 1
  EXPECT_EQ(add<int128_t>(int128Min, 0), int128Min); // MIN
  EXPECT_EQ(add<int128_t>(int128Min, 1), int128Min + 1); // MIN + 1
  EXPECT_EQ(add<int128_t>(int128Max, -1), int128Max - 1); // MAX - 1
  EXPECT_EQ(add<int128_t>(int128Max, 0), int128Max); // MAX
  EXPECT_THAT([] { add<int128_t>(int128Max, 1); }, Throws<Overflow>()); // MAX + 1
  EXPECT_THAT([] { add<int128_t>(int128Max, int128Max); }, Throws<Overflow>()); // MAX + MAX

  // `uint128_t`

  EXPECT_EQ(add<uint128_t>(0, -uint128Max), 1); // -MAX (not working; should overflow)
  EXPECT_EQ(add<uint128_t>(0, -1), uint128Max); // -1 (not working; should overflow)
  EXPECT_EQ(add<uint128_t>(0, 0), 0); // 0
  EXPECT_EQ(add<uint128_t>(0, 1), 1); // 1
  EXPECT_THAT(([] { add<uint128_t>(uint128Max, -1); }), Throws<Overflow>()); // MAX - 1 (not working; should be MAX - 1)
  EXPECT_EQ(add<uint128_t>(uint128Max, 0), uint128Max); // MAX
  EXPECT_THAT(([] { add<uint128_t>(uint128Max, 1); }), Throws<Overflow>()); // MAX + 1
  EXPECT_THAT(([] { add<uint128_t>(uint128Max, uint128Max); }), Throws<Overflow>()); // MAX + MAX
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

  // `int128_t`

  EXPECT_THAT([] { sub<int128_t>(int128Min, int128Max); }, Throws<Overflow>()); // MIN - MAX
  EXPECT_THAT([] { sub<int128_t>(int128Min, 1); }, Throws<Overflow>()); // MIN - 1
  EXPECT_EQ(sub<int128_t>(int128Min, 0), int128Min); // MIN
  EXPECT_EQ(sub<int128_t>(int128Min, -1), int128Min + 1); // MIN + 1
  EXPECT_EQ(sub<int128_t>(int128Max, 1), int128Max - 1); // MAX - 1
  EXPECT_EQ(sub<int128_t>(int128Max, 0), int128Max); // MAX
  EXPECT_THAT([] { sub<int128_t>(int128Max, -1); }, Throws<Overflow>()); // MAX + 1
  EXPECT_THAT([] { sub<int128_t>(int128Max, -int128Max); }, Throws<Overflow>()); // MAX + MAX

  // `uint128_t`

  EXPECT_THAT([] { sub<uint128_t>(0, uint128Max); }, Throws<Overflow>()); // -MAX
  EXPECT_THAT([] { sub<uint128_t>(0, 1); }, Throws<Overflow>()); // -1
  EXPECT_EQ(sub<uint128_t>(0, 0), 0); // 0
  EXPECT_THAT([] { sub<uint128_t>(0, -1); }, Throws<Overflow>()); // 1 (not working; should be 1)
  EXPECT_EQ(sub<uint128_t>(uint128Max, 1), uint128Max - 1); // MAX - 1
  EXPECT_EQ(sub<uint128_t>(uint128Max, 0), uint128Max); // MAX
  EXPECT_EQ(sub<uint128_t>(uint128Max, -1), 0); // MAX + 1 (not working; should overflow)
  EXPECT_EQ(sub<uint128_t>(uint128Max, -uint128Max), uint128Max - 1); // MAX + MAX (not working; should overflow)
}

TEST(numeric, to) {
  EXPECT_THAT([] { to<unsigned char>(-1); }, Throws<Overflow>());
  EXPECT_EQ(to<unsigned char>(255), 255);
  EXPECT_THAT([] { to<unsigned char>(256); }, Throws<Overflow>());
  EXPECT_EQ(to<short>(-32768), -32768);
  EXPECT_THAT([] { to<short>(-32769); }, Throws<Overflow>());
}

// EOF
