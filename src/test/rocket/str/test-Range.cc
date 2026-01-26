/*
 * test-Range.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/str/Range.h"

using namespace rocket::str;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(Range, initializerList) {
  str::Range val {};
  EXPECT_EQ(val.lower, 1);
  EXPECT_EQ(val.upper, 0);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.size(), 0);
}

// EOF
