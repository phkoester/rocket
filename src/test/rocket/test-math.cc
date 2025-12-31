/*
 * test-math.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/math.h"

using namespace rocket;
using namespace rocket::math;
using namespace std;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(math, RightOpenIntervalFormat) {
  using type = RightOpenInterval<int>;

  EXPECT_EQ(fmt::format("{}", type {}), "∅");
  EXPECT_EQ(fmt::format("{}", type { 1'000, 2'000 }), "[1000,2000)");
  EXPECT_EQ(fmt::format("{: >6d}", type { 1'000, 2'000 }), "[  1000,  2000)");
  EXPECT_EQ(fmt::format("{}", type { 5, nullopt }), "[5,+∞)");
}

// EOF
