/*
 * test-interval.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/math/interval.h"

using namespace rocket::math;

// #TEST ----------------------------------------------------------------------------------------------------

// Integer ..................................................................................................

TEST(interval, ClosedIntervalI32) {
  using type = ClosedInterval<i32>;

  type val = type();
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 0);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 1);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 2);
  EXPECT_EQ(val.size(), 1);

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 3);
  EXPECT_EQ(val.size(), 2);

  EXPECT_THAT([&] { val = type(1, 0); }, Throws<rocket::InvalidArgument>() );
}

TEST(interval, ClosedIntervalU32) {
  using type = ClosedInterval<u32>;

  type val = type();
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 0);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 1);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 2);
  EXPECT_EQ(val.size(), 1);

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 3);
  EXPECT_EQ(val.size(), 2);

  EXPECT_THAT([&] { val = type(1, 0); }, Throws<rocket::InvalidArgument>() );
}

TEST(interval, LeftOpenIntervalI32) {
  using type = LeftOpenInterval<i32>;

  type val = type();
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 0);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 1);
  EXPECT_EQ(val.size(), 1);

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 2);
  EXPECT_EQ(val.size(), 2);

  val = type(nullopt, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_THAT([&] { val = type(1, 0); }, Throws<rocket::InvalidArgument>() );
}

TEST(interval, LeftOpenIntervalU32) {
  using type = LeftOpenInterval<u32>;

  type val = type();
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 0);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 1);
  EXPECT_EQ(val.size(), 1);

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 2);
  EXPECT_EQ(val.size(), 2);

  val = type(nullopt, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_THAT([&] { val = type(1, 0); }, Throws<rocket::InvalidArgument>() );
}

TEST(interval, RightOpenIntervalI32) {
  using type = RightOpenInterval<i32>;

  type val = type();
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 0);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 1);
  EXPECT_EQ(val.size(), 1);

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 2);
  EXPECT_EQ(val.size(), 2);

  val = type(0, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_THAT([&] { val = type(1, 0); }, Throws<rocket::InvalidArgument>() );
}

TEST(interval, RightOpenIntervalU32) {
  using type = RightOpenInterval<u32>;

  type val = type();
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 0);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 1);
  EXPECT_EQ(val.size(), 1);

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 2);
  EXPECT_EQ(val.size(), 2);

  val = type(0, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_THAT([&] { val = type(1, 0); }, Throws<rocket::InvalidArgument>() );
}

TEST(interval, OpenIntervalI32) {
  using type = OpenInterval<i32>;

  type val = type();
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 0);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 1);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 1);
  EXPECT_EQ(val.size(), 2);

  val = type(0, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  val = type(nullopt, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  val = type(nullopt, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_THAT([&] { val = type(1, 0); }, Throws<rocket::InvalidArgument>() );
}

TEST(interval, OpenIntervalU32) {
  using type = OpenInterval<u32>;

  type val = type();
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 0);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 1);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 1);
  EXPECT_EQ(val.size(), 2);

  val = type(0, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  val = type(nullopt, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  val = type(nullopt, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_THAT([&] { val = type(1, 0); }, Throws<rocket::InvalidArgument>() );
}

// Floating-point ...........................................................................................

TEST(interval, ClosedIntervalF32) {
  using type = ClosedInterval<f32>;

  type val = type();
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 0);
  EXPECT_FALSE(val.empty());
  // This is the only case where a floating-point interval has a cardinality of 1
  EXPECT_EQ(val.cardinality(), 1);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), 1);

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), 2);

  EXPECT_THAT([&] { val = type(1, 0); }, Throws<rocket::InvalidArgument>() );
}

TEST(interval, LeftOpenIntervalF32) {
  using type = LeftOpenInterval<f32>;

  type val = type();
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 0);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), 1);

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), 2);

  val = type(nullopt, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_THAT([&] { val = type(1, 0); }, Throws<rocket::InvalidArgument>() );
}

TEST(interval, RightOpenIntervalF32) {
  using type = RightOpenInterval<f32>;

  type val = type();
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 0);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), 1);

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), 2);

  val = type(0, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_THAT([&] { val = type(1, 0); }, Throws<rocket::InvalidArgument>() );
}

TEST(interval, OpenIntervalF32) {
  using type = OpenInterval<f32>;

  type val = type();
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 0);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), 1);

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), 2);

  val = type(0, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  val = type(nullopt, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  val = type(nullopt, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_THAT([&] { val = type(1, 0); }, Throws<rocket::InvalidArgument>() );
}

// Format ...................................................................................................

TEST(interval, ClosedIntervalFormat) {
  using type = ClosedInterval<i32>;

  EXPECT_EQ(fmt::format("{}", type()), "∅");
  EXPECT_EQ(type().size(), 0);
  EXPECT_TRUE(type().empty());

  EXPECT_EQ(fmt::format("{}", type(1, 1)), "[1,1]");
  EXPECT_EQ(fmt::format("{}", type(1, 2)), "[1,2]");
}

TEST(interval, RightOpenIntervalFormat) {
  using type = RightOpenInterval<i32>;

  EXPECT_EQ(fmt::format("{}", type()), "∅");
  EXPECT_EQ(fmt::format("{}", type(1, 1)), "∅");
  EXPECT_EQ(fmt::format("{}", type(1, 2)), "[1,2)");
  EXPECT_EQ(fmt::format("{}", type(1'000, 2'000)), "[1000,2000)");
  EXPECT_EQ(fmt::format("{:~>6d}", type(1'000, 2'000)), "[~~1000,~~2000)");
  EXPECT_EQ(fmt::format("{}", type(5, nullopt)), "[5,+∞)");

  EXPECT_EQ(fmt::format(U"{}", type(1'000, 2'000)), U"[1000,2000)");
}

// EOF
