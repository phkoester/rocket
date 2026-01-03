/*
 * test-scnlib.cc
 */

#include "rocket-gtest/rocket-gtest.h"

using namespace std;

#include <scn/scan.h>

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(scnlib, intScan) {
  auto result = scn::scan<int>("123", "{}");
  auto i = result->value();
  EXPECT_EQ(i, 123);
}

// EOF
