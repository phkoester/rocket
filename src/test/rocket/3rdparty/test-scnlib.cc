/*
 * test-scnlib.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include <scn/scan.h>

using namespace scn;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(scnlib, intScan) {
  auto result = scan<i32>("123", "{}");
  auto i = result->value();
  EXPECT_EQ(i, 123);
}

// EOF
