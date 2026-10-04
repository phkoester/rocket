/*
 * test-int128.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/nio/nio.h"

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(int128, int128OpInput) {
  using compareType = i32;
  compareType compare = 0;
  auto compareLimits = numeric_limits<compareType>();

  using type = int128;
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

TEST(int128, int128OpOutput) {
  using type = int128;

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

TEST(int128, uint128OpInput) {
  using compareType = u32;
  compareType compare = 0;
  auto compareLimits = numeric_limits<compareType>();

  using type = uint128;
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

TEST(int128, uint128OpOutput) {
  using type = uint128;

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
