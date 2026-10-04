/*
 * test-boost-int128.cc
 */

#include "rocket-test/rocket-test.h"

#include <boost/int128/int128.hpp>

// Types ----------------------------------------------------------------------------------------------------

using int128 = boost::int128::int128;
using uint128 = boost::int128::uint128;

// `TEST` ---------------------------------------------------------------------------------------------------

// template<>
// struct std::is_unsigned<uint128> : true_type {};

TEST(boostInt128, isSigned) {
  EXPECT_TRUE(is_signed_v<int128>);
  EXPECT_TRUE(is_unsigned_v<uint128>);
}

// EOF
