/*
 * test-interval.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/literal.h"
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

  EXPECT_FALSE(val.contains(-1));
  EXPECT_TRUE(val.contains(0));
  EXPECT_FALSE(val.contains(1));

  val = type(0, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 2);
  EXPECT_EQ(val.size(), 1);

  EXPECT_FALSE(val.contains(-1));
  EXPECT_TRUE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_FALSE(val.contains(2));

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 3);
  EXPECT_EQ(val.size(), 2);

  EXPECT_FALSE(val.contains(-1));
  EXPECT_TRUE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_TRUE(val.contains(2));
  EXPECT_FALSE(val.contains(3));

  EXPECT_EQ(type(1, 0), type());
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

  EXPECT_TRUE(val.contains(0));
  EXPECT_FALSE(val.contains(1));

  val = type(0, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 2);
  EXPECT_EQ(val.size(), 1);

  EXPECT_TRUE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_FALSE(val.contains(2));

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 3);
  EXPECT_EQ(val.size(), 2);

  EXPECT_TRUE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_TRUE(val.contains(2));
  EXPECT_FALSE(val.contains(3));

  EXPECT_EQ(type(1, 0), type());
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

  EXPECT_FALSE(val.contains(-1));
  EXPECT_FALSE(val.contains(0));
  EXPECT_FALSE(val.contains(1));

  val = type(0, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 1);
  EXPECT_EQ(val.size(), 1);

  EXPECT_FALSE(val.contains(-1));
  EXPECT_FALSE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_FALSE(val.contains(2));

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 2);
  EXPECT_EQ(val.size(), 2);

  EXPECT_FALSE(val.contains(-1));
  EXPECT_FALSE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_TRUE(val.contains(2));
  EXPECT_FALSE(val.contains(3));

  val = type(nullopt, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_TRUE(val.contains(-1));
  EXPECT_TRUE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_FALSE(val.contains(2));

  EXPECT_EQ(type(1, 0), type());
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

  EXPECT_FALSE(val.contains(0));
  EXPECT_FALSE(val.contains(1));

  val = type(0, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 1);
  EXPECT_EQ(val.size(), 1);

  EXPECT_FALSE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_FALSE(val.contains(2));

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 2);
  EXPECT_EQ(val.size(), 2);

  EXPECT_FALSE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_TRUE(val.contains(2));
  EXPECT_FALSE(val.contains(3));

  val = type(nullopt, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_TRUE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_FALSE(val.contains(2));

  EXPECT_EQ(type(1, 0), type());
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

  EXPECT_FALSE(val.contains(-1));
  EXPECT_FALSE(val.contains(0));
  EXPECT_FALSE(val.contains(1));

  val = type(0, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 1);
  EXPECT_EQ(val.size(), 1);

  EXPECT_FALSE(val.contains(-1));
  EXPECT_TRUE(val.contains(0));
  EXPECT_FALSE(val.contains(1));

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 2);
  EXPECT_EQ(val.size(), 2);

  EXPECT_FALSE(val.contains(-1));
  EXPECT_TRUE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_FALSE(val.contains(2));

  val = type(0, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_FALSE(val.contains(-1));
  EXPECT_TRUE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_TRUE(val.contains(2));

  EXPECT_EQ(type(1, 0), type());
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

  EXPECT_FALSE(val.contains(0));
  EXPECT_FALSE(val.contains(1));

  val = type(0, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 1);
  EXPECT_EQ(val.size(), 1);

  EXPECT_TRUE(val.contains(0));
  EXPECT_FALSE(val.contains(1));

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 2);
  EXPECT_EQ(val.size(), 2);

  EXPECT_TRUE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_FALSE(val.contains(2));

  val = type(0, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_TRUE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_TRUE(val.contains(2));

  EXPECT_EQ(type(1, 0), type());
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

  EXPECT_FALSE(val.contains(-1));
  EXPECT_FALSE(val.contains(0));
  EXPECT_FALSE(val.contains(1));

  val = type(0, 1);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  EXPECT_FALSE(val.contains(-1));
  EXPECT_FALSE(val.contains(0));
  EXPECT_FALSE(val.contains(1));
  EXPECT_FALSE(val.contains(2));

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 1);
  EXPECT_EQ(val.size(), 2);

  EXPECT_FALSE(val.contains(-1));
  EXPECT_FALSE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_FALSE(val.contains(2));

  val = type(0, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_FALSE(val.contains(-1));
  EXPECT_FALSE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_TRUE(val.contains(2));

  val = type(nullopt, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_TRUE(val.contains(-1));
  EXPECT_TRUE(val.contains(0));
  EXPECT_FALSE(val.contains(1));
  EXPECT_FALSE(val.contains(2));

  val = type(nullopt, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_TRUE(val.contains(-1));
  EXPECT_TRUE(val.contains(0));
  EXPECT_TRUE(val.contains(1));

  EXPECT_EQ(type(1, 0), type());
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

  EXPECT_FALSE(val.contains(0));
  EXPECT_FALSE(val.contains(1));

  val = type(0, 1);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  EXPECT_FALSE(val.contains(0));
  EXPECT_FALSE(val.contains(1));
  EXPECT_FALSE(val.contains(2));

  val = type(0, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), 1);
  EXPECT_EQ(val.size(), 2);

  EXPECT_FALSE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_FALSE(val.contains(2));

  val = type(0, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_FALSE(val.contains(0));
  EXPECT_TRUE(val.contains(1));
  EXPECT_TRUE(val.contains(2));

  val = type(nullopt, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_TRUE(val.contains(0));
  EXPECT_FALSE(val.contains(1));
  EXPECT_FALSE(val.contains(2));

  val = type(nullopt, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_TRUE(val.contains(0));
  EXPECT_TRUE(val.contains(1));

  EXPECT_EQ(type(1, 0), type());
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

  EXPECT_EQ(type(1, 0), type());
}

TEST(interval, LeftOpenIntervalF32) {
  using type = LeftOpenInterval<f32>;

  type val = type();
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0_f32, 0);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0_f32, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), 1);

  val = type(0_f32, 2);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), 2);

  val = type(nullopt, 1);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_EQ(type(1_f32, 0), type());
}

TEST(interval, RightOpenIntervalF32) {
  using type = RightOpenInterval<f32>;

  type val = type();
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 0_f32);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  val = type(0, 1_f32);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), 1);

  val = type(0, 2_f32);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), 2);

  val = type(0, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_EQ(type(1, 0_f32), type());
}

TEST(interval, OpenIntervalF32) {
  using type = OpenInterval<f32>;

#if 0
  auto val = type(1_f32, 0_f32);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0_f32);
#endif

  auto val = type(0_f32, 0_f32);
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0_f32);

  val = type(0_f32, 1_f32);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), 1);

  val = type(0_f32, 2_f32);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), 2);

  val = type(0_f32, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  val = type(nullopt, 1_f32);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  val = type(nullopt, nullopt);
  EXPECT_FALSE(val.empty());
  EXPECT_EQ(val.cardinality(), nullopt);
  EXPECT_EQ(val.size(), nullopt);

  EXPECT_EQ(type(1_f32, 0_f32), type());
}

// Operators ................................................................................................

TEST(interval, intersection) {
  {
    using type = ClosedInterval<i32>;
    EXPECT_EQ(type() & type(), type());
    EXPECT_EQ(type(0, 1) & type(2, 3), type());
    EXPECT_EQ(type(2, 3) & type(0, 1), type());
    EXPECT_EQ(type(0, 2) & type(1, 3), type(1, 2));
    EXPECT_EQ(type(1, 3) & type(0, 2), type(1, 2));
  }

  {
    using type = OpenInterval<f64>;
    EXPECT_EQ(type() & type(), type());
    EXPECT_EQ(type(nullopt, nullopt) & type(), type());
    EXPECT_EQ(type(0_f64, 1_f64) & type(2_f64, 3_f64), type());
    EXPECT_EQ(type(0_f64, 2_f64) & type(1_f64, 3_f64), type(1_f64, 2_f64));
  }
}

TEST(interval, union) {
  {
    using type = ClosedInterval<i32>;
    EXPECT_EQ(type() | type(), type());
    EXPECT_EQ(type(0, 1) | type(2, 3), type(0, 3));
    EXPECT_EQ(type(2, 3) | type(0, 1), type(0, 3));
    EXPECT_EQ(type(0, 2) | type(1, 3), type(0, 3));
    EXPECT_EQ(type(1, 3) | type(0, 2), type(0, 3));
  }

  {
    using type = OpenInterval<f64>;
    EXPECT_EQ(type() | type(), type());
    EXPECT_EQ(type(nullopt, nullopt) | type(), type(nullopt, nullopt));
    EXPECT_EQ(type(0_f64, 1_f64) | type(2_f64, 3_f64), type(0_f64, 3_f64));
    EXPECT_EQ(type(0_f64, 2_f64) | type(1_f64, 3_f64), type(0_f64, 3_f64));
  }
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
