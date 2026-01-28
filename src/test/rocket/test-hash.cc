/*
 * test-hash.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/hash.h"

// #TEST ----------------------------------------------------------------------------------------------------

TEST(hash, unhash) {
  EXPECT_EQ(unhash32(hash32(1U)), 1U);
  EXPECT_EQ(unhash32(hash32(12345678U)), 12345678U);

  EXPECT_EQ(unhash64(hash64(1UL)), 1UL);
  EXPECT_EQ(unhash64(hash64(123456789012UL)), 123456789012UL);
}

// EOF
