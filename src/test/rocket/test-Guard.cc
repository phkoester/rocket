/*
 * test-Guard.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/Guard.h"

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(Guard, ROCKET_GUARD) {
  bool flag = true;
  {
    ROCKET_GUARD([&] { flag = false; });
    EXPECT_TRUE(flag);
  }
  EXPECT_FALSE(flag);
}

TEST(Guard, ROCKET_VALUE_GUARD) {
  int n = 1;
  {
    ROCKET_VALUE_GUARD(n, 2);
    EXPECT_EQ(n, 2);
  }
  EXPECT_EQ(n, 1);
}

// EOF
