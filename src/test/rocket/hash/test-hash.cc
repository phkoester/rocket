/*
 * test-hash.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/hash/hash.h"

using namespace rocket::hash;

// #TEST ----------------------------------------------------------------------------------------------------

TEST(hash, combineOrder) {
  u64 seed = 1;
  combine(seed, 2);
  combine(seed, 3);
  u64 hash1 = seed;

  seed = 3;
  combine(seed, 2);
  combine(seed, 1);
  u64 hash2 = seed;

  EXPECT_NE(hash1, hash2);
}

// EOF
