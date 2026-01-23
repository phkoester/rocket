/*
 * test-Interval.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/math/Interval.h"

using namespace rocket::math;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(math, RightOpenIntervalFormat) {
  using type = RightOpenInterval<i32>;

  EXPECT_EQ(fmt::format("{}", type {}), "∅");
  EXPECT_EQ(fmt::format("{}", type { 1'000, 2'000 }), "[1000,2000)");
  EXPECT_EQ(fmt::format("{:~>6d}", type { 1'000, 2'000 }), "[~~1000,~~2000)");
  EXPECT_EQ(fmt::format("{}", type { 5, nullopt }), "[5,+∞)");

  EXPECT_EQ(fmt::format(U"{}", type { 1'000, 2'000 }), U"[1000,2000)");
}

// EOF
