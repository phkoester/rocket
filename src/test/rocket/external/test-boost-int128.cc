/*
 * test-boost-int128.cc
 */

#include "rocket-test/rocket-test.h"

#include <boost/int128/int128.hpp>

using namespace std;

using int128 = boost::int128::int128;
using uint128 = boost::int128::uint128;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(boostInt128, uint128OpLeInt128) {
  uint128 lhs = 1;
  int128 rhs = 2;
  EXPECT_LE(lhs, rhs);
}

// EOF
