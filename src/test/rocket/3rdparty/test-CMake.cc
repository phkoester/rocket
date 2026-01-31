/*
 * test-CMake.cc
 */

#include "rocket-test/rocket-test.h"

extern const char* generated();

// #TEST ----------------------------------------------------------------------------------------------------

TEST(CMake, generated) {
  EXPECT_EQ(generated(), "Hello from `generated.cc`!");
}

// EOF
