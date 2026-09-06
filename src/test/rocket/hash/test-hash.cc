/*
 * test-hash.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/hash/hash.h"

using namespace rocket::hash;

// `TEST` ---------------------------------------------------------------------------------------------------

/// Tests that the order affects the hash value.
TEST(hash, combine) {
  u64 seed = 1;
  combine(seed, 2);
  combine(seed, 3);
  const u64 hash1 = seed;

  seed = 3;
  combine(seed, 2);
  combine(seed, 1);
  const u64 hash2 = seed;

  EXPECT_NE(hash1, hash2);
}

// EOF
