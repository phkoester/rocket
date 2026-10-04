/*
 * test-boost-int128.cc
 */

#include "rocket-test/rocket-test.h"

#include <boost/int128/int128.hpp>

// Types ----------------------------------------------------------------------------------------------------

using int128 = boost::int128::int128;
using uint128 = boost::int128::uint128;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(boostInt128, opCompare) {
  EXPECT_LT(int128(-1), int128(0));
  EXPECT_GT(uint128(-1), uint128(0));
}

// EOF
