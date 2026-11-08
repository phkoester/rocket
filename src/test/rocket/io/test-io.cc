/*
 * test-io.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/io/io.h"

namespace {

// Local constants ------------------------------------------------------------------------------------------

// These have 39 digits, the same as the greatest `u128`, but are greater. Multiplying them out wraps around
// to a value that can look perfectly valid
const string FOUR_E38 = "4" + string(38, '0');
const string NINE_E38 = "9" + string(38, '0');

const string TWO_TO_128 = "340282366920938463463374607431768211456";

// Local functions ------------------------------------------------------------------------------------------

/// Checks that reading @p input fails, consumes all of it, and leaves the destination unchanged.
template<typename T>
void
expectOverflow(const string& input) {
  SCOPED_TRACE(input);

  auto val = static_cast<T>(42);
  auto is = io::is(input);
  is >> val;
  EXPECT_ISTREAM(is, true, true, static_cast<i32>(input.size()));
  EXPECT_EQ(val, static_cast<T>(42));
}

/// Checks that reading @p input succeeds, consumes all of it, and yields @p expected.
template<typename T>
void
expectValue(const string& input, T expected) {
  SCOPED_TRACE(input);

  auto val = static_cast<T>(42);
  auto is = io::is(input);
  is >> val;
  EXPECT_ISTREAM(is, false, true, static_cast<i32>(input.size()));
  EXPECT_EQ(val, expected);
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(io, i128OpInput) {
  using compareType = i32;
  compareType compare = 0;
  auto compareLimits = numeric_limits<compareType>();

  using type = i128;
  type val = 0;
  auto limits = numeric_limits<type>();

  // Empty input
  {
    const string input;

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 0);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 0);
  }

  // Invalid character
  {
    const string input = "x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 0);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, false, 0);
  }

  // Valid character, invalid input
  {
    const string input = "-";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 1);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 1);
  }

  // Valid character, invalid input
  {
    const string input = "-x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 1);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, false, 1);
  }

  // Valid input, EOF
  {
    const string input = "-1";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 2);
    EXPECT_EQ(compare, -1);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 2);
    EXPECT_EQ(val, -1);
  }

  // Valid input, no EOF
  {
    const string input = "-999999x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, false, 7);
    EXPECT_EQ(compare, -999999);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, false, 7);
    EXPECT_EQ(val, -999999);
  }

  // MIN
  {
    const string compareInput = "-2147483648";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 11);
    EXPECT_EQ(compare, compareLimits.min());

    const string input = "-170141183460469231731687303715884105728";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 40);
    EXPECT_EQ(val, limits.min());
  }

  // MIN - 1, overflow
  {
    const string compareInput = "-2147483649";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 11);

    const string input = "-170141183460469231731687303715884105729";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 40);
  }

  // MAX
  {
    const string compareInput = "2147483647";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 10);
    EXPECT_EQ(compare, compareLimits.max());

    const string input = "170141183460469231731687303715884105727";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 39);
    EXPECT_EQ(val, limits.max());
  }

  // MAX + 1, overflow
  {
    const string compareInput = "2147483648";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 10);

    const string input = "170141183460469231731687303715884105728";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 39);
  }

  // MAX * 10, overflow
  {
    const string compareInput = "21474836470";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 11);

    const string input = "1701411834604692317316873037158841057280";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 40);
  }

  // MAX * 100, overflow
  {
    const string compareInput = "214748364700";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 12);

    const string input = "17014118346046923173168730371588410572800";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 41);
  }
}

TEST(io, i128OpInputOverflow) {
  expectOverflow<i128>(FOUR_E38);
  expectOverflow<i128>("+" + FOUR_E38);
  expectOverflow<i128>("-" + FOUR_E38);
  expectOverflow<i128>(NINE_E38);
  expectOverflow<i128>("-" + NINE_E38);
  expectOverflow<i128>(TWO_TO_128);
  expectOverflow<i128>("-" + TWO_TO_128);
  expectOverflow<i128>("1" + string(39, '0'));
}

TEST(io, i128OpInputOverflowStopsAtNonDigit) {
  auto val = static_cast<i128>(42);
  const string input = FOUR_E38 + "x"; // `io::is` only keeps a view, so this must outlive the stream
  auto is = io::is(input);
  is >> val;
  EXPECT_ISTREAM(is, true, false, 39);
  EXPECT_EQ(val, static_cast<i128>(42));
}

TEST(io, i128OpInputLimits) {
  using limits = numeric_limits<i128>;

  expectValue<i128>("170141183460469231731687303715884105727", limits::max());
  expectValue<i128>("+170141183460469231731687303715884105727", limits::max());
  expectValue<i128>("-170141183460469231731687303715884105728", limits::min());
  expectValue<i128>("-170141183460469231731687303715884105727", limits::min() + 1);
}

TEST(io, i128OpInputLeadingZeros) {
  using limits = numeric_limits<i128>;

  expectValue<i128>(string(50, '0') + "1", static_cast<i128>(1));
  expectValue<i128>("-" + string(50, '0') + "1", static_cast<i128>(-1));
  expectValue<i128>(string(10, '0') + "170141183460469231731687303715884105727", limits::max());
  expectValue<i128>("-" + string(10, '0') + "170141183460469231731687303715884105728", limits::min());
  expectValue<i128>(string(50, '0'), static_cast<i128>(0));
  expectValue<i128>("-0", static_cast<i128>(0));
}

TEST(io, i128OpOutput) {
  using type = i128;

  using limits = numeric_limits<type>;

  {
    ostringstream os;
    os << limits::min();
    EXPECT_EQ(os.str(), "-170141183460469231731687303715884105728");
  }

  {
    ostringstream os;
    os << limits::max();
    EXPECT_EQ(os.str(), "170141183460469231731687303715884105727");
  }
}

TEST(io, u128OpInput) {
  using compareType = u32;
  compareType compare = 0;
  auto compareLimits = numeric_limits<compareType>();

  using type = u128;
  type val = 0;
  using limits = numeric_limits<type>;

  // Empty input
  {
    const string input;

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 0);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 0);
  }

  // Invalid character
  {
    const string input = "x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 0);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, false, 0);
  }

  // Valid character, invalid input
  {
    const string input = "-";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 1);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 1);
  }

  // Valid character, invalid input
  {
    const string input = "-x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 1);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, false, 1);
  }

  // Valid input, EOF
  {
    const string input = "1";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 1);
    EXPECT_EQ(compare, 1);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 1);
    EXPECT_EQ(val, 1U);
  }

  // Valid input, no EOF
  {
    const string input = "999999x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, false, 6);
    EXPECT_EQ(compare, 999999);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, false, 6);
    EXPECT_EQ(val, 999999U);
  }

  // MIN
  {
    const string input = "0";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 1);
    EXPECT_EQ(compare, compareLimits.min());

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 1);
    EXPECT_EQ(val, limits::min());
  }

  // MIN - 1, e.g "-1", which is accepted
  {
    const string input = "-1";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 2);
    EXPECT_EQ(compare, compareLimits.max());

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 2);
    EXPECT_EQ(val, limits::max());
  }

  // MAX
  {
    const string compareInput = "4294967295";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 10);
    EXPECT_EQ(compare, compareLimits.max());

    const string input = "340282366920938463463374607431768211455";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 39);
    EXPECT_EQ(val, limits::max());
  }

  // MAX + 1, overflow
  {
    const string compareInput = "4294967296";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 10);

    const string input = "340282366920938463463374607431768211456";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 39);
  }

  // MAX * 10, overflow
  {
    const string compareInput = "42949672960";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 11);

    const string input = "3402823669209384634633746074317682114560";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 40);
  }

  // MAX * 100, overflow
  {
    const string compareInput = "429496729600";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 12);

    const string input = "34028236692093846346337460743176821145600";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 41);
  }
}

TEST(io, u128OpInputOverflow) {
  expectOverflow<u128>(FOUR_E38); // Used to yield 59717633079061536536625392568231788544
  expectOverflow<u128>("+" + FOUR_E38);
  expectOverflow<u128>(NINE_E38);
  expectOverflow<u128>(TWO_TO_128); // Wraps around to 0
  expectOverflow<u128>("680564733841876926926749214863536422912"); // 2^129, also wraps around to 0
  expectOverflow<u128>("1" + string(39, '0'));

  expectOverflow<u128>("-" + FOUR_E38); // Handled by the `i128` overload
}

TEST(io, u128OpInputOverflowStopsAtNonDigit) {
  auto val = static_cast<u128>(42);
  const string input = FOUR_E38 + "x"; // `io::is` only keeps a view, so this must outlive the stream
  auto is = io::is(input);
  is >> val;
  EXPECT_ISTREAM(is, true, false, 39);
  EXPECT_EQ(val, static_cast<u128>(42));
}

TEST(io, u128OpInputLimits) {
  using limits = numeric_limits<u128>;

  expectValue<u128>("340282366920938463463374607431768211455", limits::max());
  expectValue<u128>("+340282366920938463463374607431768211455", limits::max());
  expectValue<u128>("340282366920938463463374607431768211454", limits::max() - 1);
}

TEST(io, u128OpInputLeadingZeros) {
  using limits = numeric_limits<u128>;

  expectValue<u128>(string(50, '0') + "1", static_cast<u128>(1));
  expectValue<u128>(string(10, '0') + "340282366920938463463374607431768211455", limits::max());
  expectValue<u128>(string(50, '0'), static_cast<u128>(0));
}

TEST(io, u128OpOutput) {
  using type = u128;

  using limits = numeric_limits<type>;

  {
    ostringstream os;
    os << limits::min();
    EXPECT_EQ(os.str(), "0");
  }

  {
    ostringstream os;
    os << limits::max();
    EXPECT_EQ(os.str(), "340282366920938463463374607431768211455");
  }
}

// EOF
