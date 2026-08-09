/*
 * test-Interval.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/literal.h"
#include "rocket/math/Interval-codec.h"

#include <fmt/xchar.h>

using namespace rocket::math;

#ifdef ROCKET_CXX_COMPILER_MSVC
#pragma warning(disable:4244)
#endif

// #TEST ----------------------------------------------------------------------------------------------------

// Integer ..................................................................................................

TEST(Interval, ClosedIntervalI32) {
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

TEST(Interval, ClosedIntervalU32) {
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

TEST(Interval, LeftOpenIntervalI32) {
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

TEST(Interval, LeftOpenIntervalU32) {
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

TEST(Interval, RightOpenIntervalI32) {
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

TEST(Interval, RightOpenIntervalU32) {
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

TEST(Interval, OpenIntervalI32) {
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

TEST(Interval, OpenIntervalU32) {
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

TEST(Interval, ClosedIntervalF32) {
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

TEST(Interval, LeftOpenIntervalF32) {
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

TEST(Interval, RightOpenIntervalF32) {
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

TEST(Interval, OpenIntervalF32) {
  using type = OpenInterval<f32>;

  auto val = type();
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0_f32);

  val = type();
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

TEST(Interval, intersection) {
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

// XXX
TEST(Interval, union) {
  {
    using type = ClosedInterval<i32>;
    EXPECT_EQ(type() | type(), vector<type>{ type() });
    EXPECT_EQ(type() | type(3, 4), vector<type>{ type(3, 4) });
    EXPECT_EQ(type(1, 2) | type(), vector<type>{ type(1, 2) });
    EXPECT_EQ(type() | type(3, 4), vector<type>{ type(3, 4) });
  }

  {
    using type = OpenInterval<f64>;
    EXPECT_EQ(type() | type(), vector<type>{ type() });
    // XXX EXPECT_EQ(type(0_f64, 1_f64) | type(2_f64, 3_f64), type(0_f64, 3_f64));
    // XXX EXPECT_EQ(type(0_f64, 2_f64) | type(1_f64, 3_f64), type(0_f64, 3_f64));
  }
}

// Format ...................................................................................................

TEST(Interval, ClosedIntervalFormat) {
  using type = ClosedInterval<i32>;

  EXPECT_EQ(fmt::format("{}", type()), "∅");
  EXPECT_EQ(type().size(), 0);
  EXPECT_TRUE(type().empty());

  EXPECT_EQ(fmt::format("{}", type(1, 1)), "[1,1]");
  EXPECT_EQ(fmt::format("{}", type(1, 2)), "[1,2]");
}

TEST(Interval, RightOpenIntervalI32Format) {
  using type = RightOpenInterval<i32>;

  EXPECT_EQ(fmt::format("{}", type()), "∅");
  EXPECT_EQ(fmt::format("{}", type(1, 1)), "∅");
  EXPECT_EQ(fmt::format("{}", type(1, 2)), "[1,2)");
  EXPECT_EQ(fmt::format("{}", type(1'000, 2'000)), "[1000,2000)");
  EXPECT_EQ(fmt::format("{}", type(5, nullopt)), "[5,∞)");

  EXPECT_EQ(fmt::format(U"{}", type(1'000, 2'000)), U"[1000,2000)");
}

TEST(Interval, RightOpenIntervalF64Format) {
  using type = RightOpenInterval<f64>;

  EXPECT_EQ(fmt::format("{}", type()), "∅");
  EXPECT_EQ(fmt::format("{}", type(1, 1)), "∅");
}

// EOF
