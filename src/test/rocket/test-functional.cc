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

TEST(hash, BoostHash128) {
  EXPECT_NE(BoostHash()(numeric_limits<i128>::max()), 0);
}

TEST(hash, StdHash128) {
  EXPECT_EQ(StdHash()(1_i128), StdHash()(1_i128));
  EXPECT_NE(StdHash()(1_i128), StdHash()(2_i128));
}

// EOF
