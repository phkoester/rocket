/*
 * test-boost-safe-numerics.cc
 */

#include "rocket-test/rocket-test.h"

#include <boost/safe_numerics/safe_integer.hpp>

using namespace boost::safe_numerics;
using namespace std;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(boostSafeNumerics, safe) {
  EXPECT_THAT([] { safe<u8>(256); }, Throws<system_error>());
  EXPECT_THAT([] { safe<u8>(-1); }, Throws<system_error>());
  EXPECT_THAT([] { safe<u8> val = 100; val *= 3; }, Throws<system_error>());

  EXPECT_THAT([] { safe<i16>(32'768_u16); }, Throws<system_error>());
  EXPECT_THAT([] { safe<i64>(18'446'744'073'709'551'615_u64); }, Throws<system_error>()); // u64 max
  EXPECT_THAT([] { safe<u64>(-1'000_i64); }, Throws<system_error>());

  // Mixing signed and unsigned is not allowed!
  EXPECT_THAT([] { safe<u64>(10) + -1; }, Throws<system_error>());
  EXPECT_THAT([] { auto val = safe<u64>(10); val += -1; }, Throws<system_error>());
}

// EOF
