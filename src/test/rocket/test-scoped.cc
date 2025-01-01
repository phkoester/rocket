/*
 * test-scoped.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/scoped.h"

// 'TEST' ---------------------------------------------------------------------------------------------------

TEST(scope, ROCKET_SCOPED) {
  bool flag = true;
  {
    ROCKET_SCOPED([&] { flag = false; });
    EXPECT_TRUE(flag);
  }
  EXPECT_FALSE(flag);
}

TEST(scope, ROCKET_SCOPED_VALUE) {
  int n = 1;
  {
    ROCKET_SCOPED_VALUE(n, 2);
    EXPECT_EQ(n, 2);
  }
  EXPECT_EQ(n, 1);
}

// EOF
