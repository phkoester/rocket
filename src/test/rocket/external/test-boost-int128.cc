/*
 * test-boost-int128.cc
 */

#include "rocket-test/rocket-test.h"

#include <boost/int128/int128.hpp>

// Types ----------------------------------------------------------------------------------------------------

using int128 = boost::int128::int128;
using uint128 = boost::int128::uint128;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(boostInt128, uint128OpLeInt128) {
  uint128 lhs = 1;
  int128 rhs = 2;
  EXPECT_LE(lhs, rhs);
}

TEST(boostInt128, numericLimits) {
  EXPECT_EQ(numeric_limits<uint128>::min(), 0);
}

#ifdef ROCKET_HAS_NATIVE_INT128
TEST(boostInt128, nativeNumericLimits) {
  EXPECT_EQ(numeric_limits<unsigned __int128>::min(), 0);
}
#endif

// EOF
