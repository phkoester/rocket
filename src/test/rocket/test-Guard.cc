/*
 * test-Guard.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/Guard.h"

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(Guard, Guard) {
  bool flag = true;
  {
    ROCKET_GUARD([&] { flag = false; });
    EXPECT_TRUE(flag);
  }
  EXPECT_FALSE(flag);
}

TEST(Guard, ValueGuard) {
  int n = 1;
  {
    ROCKET_VALUE_GUARD(n, 2);
    EXPECT_EQ(n, 2);
  }
  EXPECT_EQ(n, 1);
}

// EOF
