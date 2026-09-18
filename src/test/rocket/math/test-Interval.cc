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

// `TEST` ---------------------------------------------------------------------------------------------------

// Interval: integer ........................................................................................

TEST(Interval, ClosedIntervalI32) {
  using type = ClosedInterval<i32>;

  type val = type();
  EXPECT_TRUE(val.empty());
  EXPECT_EQ(val.cardinality(), 0);
  EXPECT_EQ(val.size(), 0);

  EXPECT_FALSE(val.contains(-2));
  EXPECT_FALSE(val.contains(-1));
  EXPECT_FALSE(val.contains(0));
  EXPECT_FALSE(val.contains(1));
  EXPECT_FALSE(val.contains(2));

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

// Interval: floating-point .................................................................................

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

// Interval: operators ......................................................................................

TEST(Interval, operatorBitWiseAnd) {
  {
    using type = ClosedInterval<i32>;
    EXPECT_EQ(type() & type(), type());
    EXPECT_EQ(type(0, 1) & type(), type());
    EXPECT_EQ(type() & type(2, 3), type());
    EXPECT_EQ(type(0, 1) & type(2, 3), type());
    EXPECT_EQ(type(2, 3) & type(0, 1), type());
    EXPECT_EQ(type(0, 2) & type(1, 3), type(1, 2));
    EXPECT_EQ(type(1, 3) & type(0, 2), type(1, 2));
  }

  {
    using type = OpenInterval<f64>;
    EXPECT_EQ(type() & type(), type());
    EXPECT_EQ(type(nullopt, nullopt) & type(), type());
    EXPECT_EQ(type(0, 1) & type(2, 3), type());
    EXPECT_EQ(type(0, 2) & type(1, 3), type(1, 2));
  }
}

TEST(Interval, operatorBitWiseOr) {
  {
    using type = ClosedInterval<i32>;
    EXPECT_EQ(type() | type(), vector<type>{ type() });
    EXPECT_EQ(type() | type(3, 4), vector<type>{ type(3, 4) });
    EXPECT_EQ(type(1, 2) | type(), vector<type>{ type(1, 2) });
    EXPECT_EQ(type() | type(3, 4), vector<type>{ type(3, 4) });
    EXPECT_EQ(type(1, 2) | type(4, 5), (vector<type>{ type(1, 2), type(4, 5) }));
    EXPECT_EQ(type(4, 5) | type(1, 2), (vector<type>{ type(1, 2), type(4, 5) }));
    EXPECT_EQ(type(1, 2) | type(3, 4), (vector<type>{ type(1, 4) }));
    EXPECT_EQ(type(3, 4) | type(1, 2), (vector<type>{ type(1, 4) }));
  }

  {
    using type = OpenInterval<f64>;
    EXPECT_EQ(type() | type(), vector<type>{ type() });
    EXPECT_EQ(type() | type(1, 2), (vector<type> { type(1, 2) }));
    EXPECT_EQ(type(1, 2) | type(), (vector<type> { type(1, 2) }));
    EXPECT_EQ(
      type(1, 2) | type(2, 3),
      (vector<type> { type(1, 2), type(2, 3) }));
  }

  {
    using type = LeftOpenInterval<f64>;
    EXPECT_EQ(type() | type(), vector<type>{ type() });
    EXPECT_EQ(type() | type(1, 2), (vector<type> { type(1, 2) }));
    EXPECT_EQ(type(1, 2) | type(), (vector<type> { type(1, 2) }));
    EXPECT_EQ(
      type(1, 2) | type(2, 3),
      (vector<type> { type(1, 3) }));
  }

  {
    using type = RightOpenInterval<f64>;
    EXPECT_EQ(type() | type(), vector<type>{ type() });
    EXPECT_EQ(type() | type(1, 2), (vector<type> { type(1, 2) }));
    EXPECT_EQ(type(1, 2) | type(), (vector<type> { type(1, 2) }));
    EXPECT_EQ(
      type(1, 2) | type(2, 3),
      (vector<type> { type(1, 3) }));
  }

  {
    using type = ClosedInterval<f64>;
    EXPECT_EQ(type() | type(), vector<type>{ type() });
    EXPECT_EQ(type() | type(1, 2), (vector<type> { type(1, 2) }));
    EXPECT_EQ(type(1, 2) | type(), (vector<type> { type(1, 2) }));
    EXPECT_EQ(
      type(1, 2) | type(3, 4),
      (vector<type> { type(1, 2), type(3, 4) }));
  }
}

// Interval: format .........................................................................................

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

// Intervals ................................................................................................

TEST(Interval, OpenIntervalsI32IntersectionOf) {
  using type = i32;
  using ival = OpenInterval<type>;
  using ivals = OpenIntervals<type>;

  // An empty list of intervals yields an empty intersection
  EXPECT_EQ(
    intersectionOf((ivals {})),
    ival());
  // A single nonempty interval yields itself
  EXPECT_EQ(
    intersectionOf((ivals { ival(1, 3) })),
    ival(1, 3));
  // A single empty interval yields an empty intersection
  EXPECT_EQ(
    intersectionOf((ivals { ival() })),
    ival());
  // An empty interval as the first operand yields an empty intersection
  EXPECT_EQ(
    intersectionOf((ivals { ival(), ival(1, 3) })),
    ival());
  // An empty interval as the second operand yields an empty intersection
  EXPECT_EQ(
    intersectionOf((ivals { ival(1, 3), ival() })),
    ival());
  // Overlapping intervals whose intersection (2,3) contains no integers yield an empty intersection
  EXPECT_EQ(
    intersectionOf((ivals { ival(1, 3), ival(2, 4) })),
    ival());
  // Overlapping intervals yield their common range
  EXPECT_EQ(
    intersectionOf((ivals { ival(1, 4), ival(2, 5) })),
    ival(2, 4));
  // An empty interval among the operands empties the whole intersection
  EXPECT_EQ(
    intersectionOf((ivals { ival(1, 4), ival(2, 5), ival() })),
    ival());
  // A disjoint interval among the operands empties the whole intersection
  EXPECT_EQ(
    intersectionOf((ivals { ival(1, 4), ival(2, 5), ival(7, 8) })),
    ival());
  // Nested intervals yield the innermost interval
  EXPECT_EQ(
    intersectionOf((ivals { ival(1, 10), ival(2, 9), ival(3, 8) })),
    ival(3, 8));
  // Unbounded intervals intersect with bounded ones
  EXPECT_EQ(
    intersectionOf((ivals { ival(nullopt, nullopt), ival(1, 4), ival(2, nullopt) })),
    ival(2, 4));
}

TEST(Interval, OpenIntervalsF64IntersectionOf) {
  using type = f64;
  using ival = OpenInterval<type>;
  using ivals = OpenIntervals<type>;

  // An empty list of intervals yields an empty intersection
  EXPECT_EQ(
    intersectionOf((ivals {})),
    ival());
  // A single nonempty interval yields itself
  EXPECT_EQ(
    intersectionOf((ivals { ival(1, 3) })),
    ival(1, 3));
  // A single empty interval yields an empty intersection
  EXPECT_EQ(
    intersectionOf((ivals { ival() })),
    ival());
  // An empty interval as the first operand yields an empty intersection
  EXPECT_EQ(
    intersectionOf((ivals { ival(), ival(1, 3) })),
    ival());
  // An empty interval as the second operand yields an empty intersection
  EXPECT_EQ(
    intersectionOf((ivals { ival(1, 3), ival() })),
    ival());
  // Unlike the integer case, the intersection (2,3) is nonempty for floating-point intervals
  EXPECT_EQ(
    intersectionOf((ivals { ival(1, 3), ival(2, 4) })),
    ival(2, 3));
  // Intervals sharing only a bound yield an empty intersection: (1,2) and (2,3) do not contain 2
  EXPECT_EQ(
    intersectionOf((ivals { ival(1, 2), ival(2, 3) })),
    ival());
  // An empty interval among the operands empties the whole intersection
  EXPECT_EQ(
    intersectionOf((ivals { ival(1, 4), ival(2, 5), ival() })),
    ival());
  // A disjoint interval among the operands empties the whole intersection
  EXPECT_EQ(
    intersectionOf((ivals { ival(1, 4), ival(2, 5), ival(7, 8) })),
    ival());
  // Nested intervals yield the innermost interval
  EXPECT_EQ(
    intersectionOf((ivals { ival(1, 10), ival(2, 9), ival(3, 8) })),
    ival(3, 8));
  // Unbounded intervals intersect with bounded ones
  EXPECT_EQ(
    intersectionOf((ivals { ival(nullopt, nullopt), ival(1, 4), ival(2, nullopt) })),
    ival(2, 4));
}

TEST(Interval, ClosedIntervalsI32UnionOf) {
  using type = i32;
  using ival = ClosedInterval<type>;
  using ivals = ClosedIntervals<type>;

  // An empty list of intervals yields an empty union
  EXPECT_EQ(
    unionOf((ivals {})),
    (ivals {}));
  // Empty intervals are dropped from the union
  EXPECT_EQ(
    unionOf((ivals { ival(), ival() })),
    (ivals {}));
  // An empty interval as the first operand does not affect the union
  EXPECT_EQ(
    unionOf((ivals { ival(), ival(1, 3) })),
    (ivals { ival(1, 3) }));
  // An empty interval as the second operand does not affect the union
  EXPECT_EQ(
    unionOf((ivals { ival(1, 3), ival() })),
    (ivals { ival(1, 3) }));
  // A single interval yields itself
  EXPECT_EQ(
    unionOf((ivals { ival(1, 3) })),
    (ivals { ival(1, 3) }));
  // Disjoint intervals are kept separate, sorted by their lower bounds
  EXPECT_EQ(
    unionOf((ivals { ival(4, 5), ival(1, 2) })),
    (ivals { ival(1, 2), ival(4, 5) }));
  // Overlapping intervals are merged into one
  EXPECT_EQ(
    unionOf((ivals { ival(1, 3), ival(2, 5) })),
    (ivals { ival(1, 5) }));
  // Adjacent intervals are merged: [1,2] and [3,4] cover all of [1,4] for integer
  EXPECT_EQ(
    unionOf((ivals { ival(1, 2), ival(3, 4) })),
    (ivals { ival(1, 4) }));
  // A nested interval is absorbed by the enclosing one
  EXPECT_EQ(
    unionOf((ivals { ival(1, 10), ival(2, 5) })),
    (ivals { ival(1, 10) }));
  // Mixed input: [1,2] and [2,4] merge to [1,4], [7,9] stays separate
  EXPECT_EQ(
    unionOf((ivals { ival(7, 9), ival(1, 2), ival(2, 4) })),
    (ivals { ival(1, 4), ival(7, 9) }));
  // Intervals with equal lower bounds are merged
  EXPECT_EQ(
    unionOf((ivals { ival(1, 5), ival(1, 2) })),
    (ivals { ival(1, 5) }));
}

TEST(Interval, OpenIntervalsF64UnionOf) {
  using type = f64;
  using ival = OpenInterval<type>;
  using ivals = OpenIntervals<type>;

  // An empty list of intervals yields an empty union
  EXPECT_EQ(
    unionOf((ivals {})),
    (ivals {}));
  // Empty intervals are dropped from the union
  EXPECT_EQ(
    unionOf((ivals { ival(), ival() })),
    (ivals {}));
  // An empty interval as the first operand does not affect the union
  EXPECT_EQ(
    unionOf((ivals { ival(), ival(1, 3) })),
    (ivals { ival(1, 3) }));
  // An empty interval as the second operand does not affect the union
  EXPECT_EQ(
    unionOf((ivals { ival(1, 3), ival() })),
    (ivals { ival(1, 3) }));
  // A single interval yields itself
  EXPECT_EQ(
    unionOf((ivals { ival(1, 3) })),
    (ivals { ival(1, 3) }));
  // Disjoint intervals are kept separate, sorted by their lower bounds
  EXPECT_EQ(
    unionOf((ivals { ival(4, 5), ival(1, 2) })),
    (ivals { ival(1, 2), ival(4, 5) }));
  // Overlapping intervals are merged into one
  EXPECT_EQ(
    unionOf((ivals { ival(1, 3), ival(2, 5) })),
    (ivals { ival(1, 5) }));
  // Intervals sharing only a bound stay separate: neither (1,2) nor (2,3) contains 2
  EXPECT_EQ(
    unionOf((ivals { ival(1, 2), ival(2, 3) })),
    (ivals { ival(1, 2), ival(2, 3) }));
  // A nested interval is absorbed by the enclosing one
  EXPECT_EQ(
    unionOf((ivals { ival(1, 10), ival(2, 5) })),
    (ivals { ival(1, 10) }));
  // Mixed input: (1,3) and (2,4) merge to (1,4), (7,9) stays separate
  EXPECT_EQ(
    unionOf((ivals { ival(7, 9), ival(1, 3), ival(2, 4) })),
    (ivals { ival(1, 4), ival(7, 9) }));
  // Intervals with equal lower bounds are merged
  EXPECT_EQ(
    unionOf((ivals { ival(1, 5), ival(1, 2) })),
    (ivals { ival(1, 5) }));
  // Overlapping unbounded intervals merge into the unbounded interval
  EXPECT_EQ(
    unionOf((ivals { ival(1, nullopt), ival(nullopt, 2) })),
    (ivals { ival(nullopt, nullopt) }));
}

// EOF
