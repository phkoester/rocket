/*
 * test-Guard.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Guard.h"

// #TEST ----------------------------------------------------------------------------------------------------

TEST(Guard, Guard) {
  bool flag = true;
  {
    ROCKET_GUARD([&] { flag = false; });
    EXPECT_TRUE(flag);
  }
  EXPECT_FALSE(flag);
}

TEST(Guard, ValueGuard) {
  i32 n = 1;
  {
    ROCKET_VALUE_GUARD(n, 2);
    EXPECT_EQ(n, 2);
  }
  EXPECT_EQ(n, 1);
}

// EOF
