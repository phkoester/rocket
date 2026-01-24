/*
 * test-rocket.cc
 */

#include "rocket-test/rocket-test.h"

// `static_assert` ------------------------------------------------------------------------------------------

static_assert(is_signed_v<char>);
static_assert(is_unsigned_v<char32>);
static_assert(is_same_v<u64, std_size_t>);
#ifdef ROCKET_HAVE_128
static_assert(is_signed_v<i128>);
static_assert(is_unsigned_v<u128>);
#endif
static_assert(is_same_v<decltype(1.0F), f32>);
static_assert(is_same_v<decltype(1.0), f64>);
#ifdef ROCKET_HAVE_128
static_assert(is_signed_v<f128>);
#endif

// `TEST` ---------------------------------------------------------------------------------------------------

#ifdef ROCKET_HAVE_128

TEST(rocket, i128OpInput) {
  using compareType = i32;
  compareType compare;
  auto compareLimits = numeric_limits<compareType>();

  using type = i128;
  type val;
  auto limits = numeric_limits<type>();

  // Empty input
  {
    string input = "";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 0);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 0);
  }

  // Invalid character
  {
    string input = "x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 0);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, false, 0);
  }

  // Valid character, invalid input
  {
    string input = "-";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 1);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 1);
  }

  // Valid character, invalid input
  {
    string input = "-x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 1);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, false, 1);
  }

  // Valid input, EOF
  {
    string input = "-1";

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
    string input = "-999999x";

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
    string compareInput = "-2147483648";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 11);
    EXPECT_EQ(compare, compareLimits.min());

    string input = "-170141183460469231731687303715884105728";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 40);
    EXPECT_EQ(val, limits.min());
  }

  // MIN - 1, overflow
  {
    string compareInput = "-2147483649";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 11);

    string input = "-170141183460469231731687303715884105729";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 40);
  }

  // MAX
  {
    string compareInput = "2147483647";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 10);
    EXPECT_EQ(compare, compareLimits.max());

    string input = "170141183460469231731687303715884105727";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 39);
    EXPECT_EQ(val, limits.max());
  }

  // MAX + 1, overflow
  {
    string compareInput = "2147483648";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 10);

    string input = "170141183460469231731687303715884105728";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 39);
  }

  // MAX * 10, overflow
  {
    string compareInput = "21474836470";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 11);

    string input = "1701411834604692317316873037158841057280";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 40);
  }

  // MAX * 100, overflow
  {
    string compareInput = "214748364700";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 12);

    string input = "17014118346046923173168730371588410572800";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 41);
  }
}

TEST(base, i128OpOutput) {
  using type = i128;

  auto limits = numeric_limits<type>();

  {
    ostringstream os;
    os << limits.min();
    EXPECT_EQ(os.str(), "-170141183460469231731687303715884105728");
  }

  {
    ostringstream os;
    os << limits.max();
    EXPECT_EQ(os.str(), "170141183460469231731687303715884105727");
  }
}

TEST(base, u128OpInput) {
  using compareType = u32;
  compareType compare;
  auto compareLimits = numeric_limits<compareType>();

  using type = u128;
  type val;
  auto limits = numeric_limits<type>();

  // Empty input
  {
    string input = "";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 0);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 0);
  }

  // Invalid character
  {
    string input = "x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 0);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, false, 0);
  }

  // Valid character, invalid input
  {
    string input = "-";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 1);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 1);
  }

  // Valid character, invalid input
  {
    string input = "-x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 1);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, false, 1);
  }

  // Valid input, EOF
  {
    string input = "1";

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
    string input = "999999x";

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
    string input = "0";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 1);
    EXPECT_EQ(compare, compareLimits.min());

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 1);
    EXPECT_EQ(val, limits.min());
  }

  // MIN - 1, e.g "-1", which is accepted
  {
    string input = "-1";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 2);
    EXPECT_EQ(compare, compareLimits.max());

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 2);
    EXPECT_EQ(val, limits.max());
  }

  // MAX
  {
    string compareInput = "4294967295";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 10);
    EXPECT_EQ(compare, compareLimits.max());

    string input = "340282366920938463463374607431768211455";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 39);
    EXPECT_EQ(val, limits.max());
  }

  // MAX + 1, overflow
  {
    string compareInput = "4294967296";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 10);

    string input = "340282366920938463463374607431768211456";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 39);
  }

  // MAX * 10, overflow
  {
    string compareInput = "42949672960";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 11);

    string input = "3402823669209384634633746074317682114560";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 40);
  }

  // MAX * 100, overflow
  {
    string compareInput = "429496729600";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 12);

    string input = "34028236692093846346337460743176821145600";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 41);
  }
}

TEST(base, u128OpOutput) {
  using type = u128;

  auto limits = numeric_limits<type>();

  {
    ostringstream os;
    os << limits.min();
    EXPECT_EQ(os.str(), "0");
  }

  {
    ostringstream os;
    os << limits.max();
    EXPECT_EQ(os.str(), "340282366920938463463374607431768211455");
  }
}

#endif // ROCKET_HAVE_128

// EOF
