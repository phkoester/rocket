/*
 * test-macro.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/macro.h"

// Variables ------------------------------------------------------------------------------------------------

void foo(int, int) {}

int x;
ROCKET_INIT(([] { x = 12; if (true) { foo(1, 2); } }));
int y;
ROCKET_INIT(([] { y = 13; }));

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(macro, ROCKET_INIT) {
  EXPECT_EQ(x, 12);
  EXPECT_EQ(y, 13);
}

// EOF
