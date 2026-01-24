/*
 * test-boost-safe-numerics.cc
 */

#include "rocket-test/rocket-test.h"

#include <boost/safe_numerics/safe_integer.hpp>

using namespace boost::safe_numerics;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(boost_safe_numerics, safe) {
  EXPECT_THAT([] { safe<u8> val = 256; }, Throws<system_error>());
  EXPECT_THAT([] { safe<u8> val = -1; }, Throws<system_error>());
  EXPECT_THAT([] { safe<u8> val = 100; val *= 3; }, Throws<system_error>());

  // Mixing signed and unsigned is not allowed!
  EXPECT_THAT([] { safe<u64>(10) + -1; }, Throws<system_error>());
  EXPECT_THAT([] { auto val = safe<u64>(10); val += -1; }, Throws<system_error>());
}

// EOF
