/*
 * test-InputFailure.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Exception.h"
#include "rocket/InputFailure.h"

using namespace std;

// Tests ====================================================================================================

TEST(InputFailure, mapped) {
  const InputFailure ex(3, { { 1, 4 }, { 5, 8 } }, "oops");

  const auto mapped = ex.mapped([](u64 position) { return 2 * position; });
  EXPECT_EQ(mapped.position(), 6U);
  EXPECT_EQ(mapped.ranges(), (str::Ranges { { 2, 8 }, { 10, 16 } }));
  EXPECT_EQ(mapped.message(), "oops");

  // The original stays as it is
  EXPECT_EQ(ex.position(), 3U);
  EXPECT_EQ(ex.ranges(), (str::Ranges { { 1, 4 }, { 5, 8 } }));
}

TEST(InputFailure, mappedUnknown) {
  const InputFailure ex(3, { { 1, 4 } }, "oops");

  // If any position cannot be translated, the copy has neither a position nor ranges
  const auto mapped = ex.mapped([](u64 position) -> optional<u64> {
    if (position == 4) {
      return nullopt;
    }
    return position;
  });
  EXPECT_EQ(mapped.position(), nullopt);
  EXPECT_TRUE(mapped.ranges().empty());
  EXPECT_EQ(mapped.message(), "oops");
}

TEST(InputFailure, noPosition) {
  const InputFailure ex(nullopt, "oops");
  EXPECT_EQ(ex.position(), nullopt);
  EXPECT_TRUE(ex.ranges().empty());

  // Without a position, there is nothing to translate
  EXPECT_EQ(ex.mapped([](u64 position) { return position + 1; }).position(), nullopt);
  EXPECT_EQ(ex.shifted(10).position(), nullopt);

  // Ranges require a position
  EXPECT_THROW(InputFailure(nullopt, { { 1, 4 } }, "oops"), InvalidArgument);
}

TEST(InputFailure, shifted) {
  const auto shifted = InputFailure(3, { { 1, 4 } }, "oops").shifted(10);
  EXPECT_EQ(shifted.position(), 13U);
  EXPECT_EQ(shifted.ranges(), (str::Ranges { { 11, 14 } }));
  EXPECT_EQ(shifted.message(), "oops");

  const auto noRanges = InputFailure(3, "oops").shifted(10);
  EXPECT_EQ(noRanges.position(), 13U);
  EXPECT_TRUE(noRanges.ranges().empty());

  // An unknown offset makes the position unknown and drops the ranges
  const auto unknown = InputFailure(3, { { 1, 4 } }, "oops").shifted(nullopt);
  EXPECT_EQ(unknown.position(), nullopt);
  EXPECT_TRUE(unknown.ranges().empty());
}

TEST(InputFailure, withRange) {
  const auto ex = InputFailure::withRange(4, 1, 4, "oops");
  EXPECT_EQ(ex.position(), 4U);
  EXPECT_EQ(ex.ranges(), (str::Ranges { { 1, 4 } }));
  EXPECT_EQ(ex.message(), "oops");

  // A range is only created if the position and both bounds are known
  for (const auto& [position, a, b] : vector<tuple<optional<u64>, optional<u64>, optional<u64>>> {
         { nullopt, 1, 4 }, { 4, nullopt, 4 }, { 4, 1, nullopt }, { nullopt, nullopt, nullopt } }) {
    const auto noRange = InputFailure::withRange(position, a, b, "oops");
    EXPECT_EQ(noRange.position(), position);
    EXPECT_TRUE(noRange.ranges().empty());
  }
}

// EOF
