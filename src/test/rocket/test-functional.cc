/*
 * test-functional.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/functional.h"

using namespace rocket;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(hash, BoostHashI132) {
  EXPECT_EQ(BoostHash()(42_i32), 42_u64);
}

#ifdef ROCKET_HAS_128

TEST(hash, BoostHash128)
{
  EXPECT_NE(BoostHash()(numeric_limits<i128>::max()), 0);
  EXPECT_NE(BoostHash()(numeric_limits<f128>::max()), 0);
}

TEST(hash, StdHash128)
{
  EXPECT_EQ(StdHash()(1_i128), StdHash()(1_i128));
  EXPECT_NE(StdHash()(1_i128), StdHash()(2_i128));

  EXPECT_EQ(StdHash()(1_f128), StdHash()(1_f128));
  EXPECT_NE(StdHash()(1_f128), StdHash()(2_f128));
}

#endif // ROCKET_HAS_128

// EOF
