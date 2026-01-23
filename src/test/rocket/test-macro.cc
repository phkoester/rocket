/*
 * test-macro.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/macro.h"

// Variables ------------------------------------------------------------------------------------------------

void foo(i32, i32) {}

i32 x;
ROCKET_INIT(([] { x = 12; if (true) { foo(1, 2); } }));
i32 y;
ROCKET_INIT(([] { y = 13; }));

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(macro, RocketInit) {
  EXPECT_EQ(x, 12);
  EXPECT_EQ(y, 13);
}

// EOF
