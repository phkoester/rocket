/*
 * test-numeric.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/numeric.h"

using namespace rocket;
using namespace std;
using namespace testing;

using rocket::internal::Control;

// `static_assert` ------------------------------------------------------------------------------------------

static_assert(is_same_v<Control<LargestType<char, unsigned short>::Type>::Type, int>);
static_assert(is_same_v<Control<LargestType<char, int>::Type>::Type, long>);
static_assert(is_same_v<Control<LargestType<signed char, long>::Type>::Type, int128_t>);
static_assert(is_same_v<Control<LargestType<signed char, int128_t>::Type>::Type, int128_t>);
static_assert(is_same_v<Control<LargestType<unsigned char, int128_t>::Type>::Type, int128_t>);
static_assert(is_same_v<Control<LargestType<unsigned char, uint128_t>::Type>::Type, uint128_t>);

// Constants ------------------------------------------------------------------------------------------------

constexpr auto longMin = numeric_limits<long>::min();
constexpr auto longMax = numeric_limits<long>::max();
constexpr auto int128Min = numeric_limits<int128_t>::min();
constexpr auto int128Max = numeric_limits<int128_t>::max();
constexpr auto uint128Max = numeric_limits<uint128_t>::max();

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(numeric, add) {
  // `char`

  EXPECT_THAT(([] { add<char>('\x00', 128); }), Throws<Overflow>());
  EXPECT_THAT(([] { add<char>(128, '\x00'); }), Throws<Overflow>());

  // `unsigned char`

  EXPECT_THAT(([] { add<unsigned char>('\x00', -1); }), Throws<Overflow>());
  EXPECT_THAT(([] { add<unsigned char>(255, 1); }), Throws<Overflow>());

  // `long`

  EXPECT_EQ((add<long>(longMin, 0)), longMin);
  EXPECT_THAT(([] { add<long>(longMin, -1); }), Throws<Overflow>());
  EXPECT_THAT(([] { add<long>(longMin, longMin); }), Throws<Overflow>());

  EXPECT_EQ((add<long>(longMax, 0)), longMax);
  EXPECT_THAT(([] { add<long>(longMax, 1); }), Throws<Overflow>());
  EXPECT_THAT(([] { add<long>(longMax, longMax); }), Throws<Overflow>());

  // `unsigned long`

  EXPECT_THAT(([] { add<unsigned long>(-1, 0); }), Throws<Overflow>());
  EXPECT_THAT(([] { add<unsigned long>(0, -1); }), Throws<Overflow>());

  // `int128_t`

  EXPECT_EQ((add<int128_t>(longMin, 0)), to<int128_t>(longMin));
  EXPECT_EQ((add<int128_t>(longMin, -1)), to<int128_t>(longMin) + to<int128_t>(-1));
  EXPECT_EQ((add<int128_t>(longMin, longMin)), to<int128_t>(longMin) + to<int128_t>(longMin));

  EXPECT_EQ((add<int128_t>(longMax, 0)), to<int128_t>(longMax));
  EXPECT_EQ((add<int128_t>(longMax, 1)), to<int128_t>(longMax) + to<int128_t>(1));
  EXPECT_EQ((add<int128_t>(longMax, longMax)), to<int128_t>(longMax) + to<int128_t>(longMax));

  EXPECT_EQ((add<int128_t>(int128Min, 0)), int128Min);
  EXPECT_THAT(([] { add<int128_t>(int128Min, -1); }), Throws<Overflow>());
  EXPECT_THAT(([] { add<int128_t>(int128Min, int128Min); }), Throws<Overflow>());

  EXPECT_EQ((add<int128_t>(int128Max, 0)), int128Max);
  EXPECT_THAT(([] { add<int128_t>(int128Max, 1); }), Throws<Overflow>());
  EXPECT_THAT(([] { add<int128_t>(int128Max, int128Max); }), Throws<Overflow>());

  // `uint128_t`

  EXPECT_EQ((add<uint128_t>(uint128Max, 0)), uint128Max);
  EXPECT_THAT(([] { add<uint128_t>(uint128Max, 1); }), Throws<Overflow>());
}

TEST(numeric, sub) {
  // `char`

  EXPECT_EQ((sub<char>(0, 128)), -128);
  EXPECT_THAT(([] { sub<char>('\x00', 129L); }), Throws<Overflow>());
  EXPECT_EQ((sub<char>(-128, 0)), -128);
  EXPECT_THAT(([] { sub<char>(-129, '\x00'); }), Throws<Overflow>());

  // `unsigned char`

  EXPECT_THAT(([] { sub<unsigned char>('\x00', 1); }), Throws<Overflow>());
  EXPECT_THAT(([] { sub<unsigned char>(255, -1); }), Throws<Overflow>());

  // `long`

  EXPECT_EQ((sub<long>(longMin, 0)), longMin);
  EXPECT_THAT(([] { sub<long>(longMin, 1); }), Throws<Overflow>());
  EXPECT_THAT(([] { sub<long>(longMin, LONG_NEG_MIN); }), Throws<Overflow>());

  EXPECT_EQ((sub<long>(longMax, 0)), longMax);
  EXPECT_THAT(([] { sub<long>(longMax, -1); }), Throws<Overflow>());
  EXPECT_THAT(([] { sub<long>(longMax, -longMax); }), Throws<Overflow>());

  // `unsigned long`

  EXPECT_THAT(([] { sub<unsigned long>(-1, 0); }), Throws<Overflow>());
  EXPECT_THAT(([] { sub<unsigned long>(0, 1); }), Throws<Overflow>());

  // `int128_t`

  // XXX Hier weiter
  EXPECT_EQ((add<int128_t>(longMin, 0)), to<int128_t>(longMin));
  EXPECT_EQ((add<int128_t>(longMin, -1)), to<int128_t>(longMin) + to<int128_t>(-1));
  EXPECT_EQ((add<int128_t>(longMin, longMin)), to<int128_t>(longMin) + to<int128_t>(longMin));

  EXPECT_EQ((add<int128_t>(longMax, 0)), to<int128_t>(longMax));
  EXPECT_EQ((add<int128_t>(longMax, 1)), to<int128_t>(longMax) + to<int128_t>(1));
  EXPECT_EQ((add<int128_t>(longMax, longMax)), to<int128_t>(longMax) + to<int128_t>(longMax));

  EXPECT_EQ((add<int128_t>(int128Min, 0)), int128Min);
  EXPECT_THAT(([] { add<int128_t>(int128Min, -1); }), Throws<Overflow>());
  EXPECT_THAT(([] { add<int128_t>(int128Min, int128Min); }), Throws<Overflow>());

  EXPECT_EQ((add<int128_t>(int128Max, 0)), int128Max);
  EXPECT_THAT(([] { add<int128_t>(int128Max, 1); }), Throws<Overflow>());
  EXPECT_THAT(([] { add<int128_t>(int128Max, int128Max); }), Throws<Overflow>());

  // `uint128_t`

  EXPECT_EQ((add<uint128_t>(uint128Max, 0)), uint128Max);
  EXPECT_THAT(([] { add<uint128_t>(uint128Max, 1); }), Throws<Overflow>());
}

// EOF
