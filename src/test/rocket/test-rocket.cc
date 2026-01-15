/*
 * test-rocket.cc
 */

#include "rocket-gtest/rocket-gtest.h"

// `static_assert` ------------------------------------------------------------------------------------------

static_assert(sizeof(char32_t) == 4);
static_assert(is_unsigned_v<char32_t>);
static_assert(sizeof(char32_t) == sizeof(U' '));
static_assert(is_same_v<decltype(U' '), char32_t>);
static_assert(is_same_v<decltype(0.0L), long double>);

// `TEST` ---------------------------------------------------------------------------------------------------

/**
 * Tests that the introductory table in `rocket.h` is correct.
 */
TEST(rocket, sizeof) {
  EXPECT_EQ(sizeof(char), 1);
  EXPECT_EQ(sizeof(std::byte), 1);
  EXPECT_EQ(sizeof(short), 2);
  EXPECT_EQ(sizeof(wchar_t), 4);
  EXPECT_EQ(sizeof(char32_t), 4);
  EXPECT_EQ(sizeof(int), 4);
  EXPECT_EQ(sizeof(float), 4);
  EXPECT_EQ(sizeof(long), 8);
  EXPECT_EQ(sizeof(long long), 8);
  EXPECT_EQ(sizeof(double), 8);
  EXPECT_EQ(sizeof(void*), 8);
  EXPECT_EQ(sizeof(int128_t), 16);
  EXPECT_EQ(sizeof(long double), 16);
}

TEST(rocket, int128OpInput) {
  using compareType = int;
  compareType compare;
  auto compareLimits = numeric_limits<compareType>();

  using type = int128_t;
  type v;
  auto limits = numeric_limits<type>();

  // Empty input
  {
    string input = "";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 0);

    auto is = io::is(input);
    is >> v;
    EXPECT_ISTREAM(is, true, true, 0);
  }

  // Invalid character
  {
    string input = "x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 0);

    auto is = io::is(input);
    is >> v;
    EXPECT_ISTREAM(is, true, false, 0);
  }

  // Valid character, invalid input
  {
    string input = "-";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 1);

    auto is = io::is(input);
    is >> v;
    EXPECT_ISTREAM(is, true, true, 1);
  }

  // Valid character, invalid input
  {
    string input = "-x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 1);

    auto is = io::is(input);
    is >> v;
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
    is >> v;
    EXPECT_ISTREAM(is, false, true, 2);
    EXPECT_EQ(v, -1);
  }

  // Valid input, no EOF
  {
    string input = "-999999x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, false, 7);
    EXPECT_EQ(compare, -999999);

    auto is = io::is(input);
    is >> v;
    EXPECT_ISTREAM(is, false, false, 7);
    EXPECT_EQ(v, -999999);
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
    is >> v;
    EXPECT_ISTREAM(is, false, true, 40);
    EXPECT_EQ(v, limits.min());
  }

  // MIN - 1, overflow
  {
    string compareInput = "-2147483649";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 11);

    string input = "-170141183460469231731687303715884105729";
    auto is = io::is(input);
    is >> v;
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
    is >> v;
    EXPECT_ISTREAM(is, false, true, 39);
    EXPECT_EQ(v, limits.max());
  }

  // MAX + 1, overflow
  {
    string compareInput = "2147483648";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 10);

    string input = "170141183460469231731687303715884105728";
    auto is = io::is(input);
    is >> v;
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
    is >> v;
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
    is >> v;
    EXPECT_ISTREAM(is, true, true, 41);
  }
}

TEST(base, int128OpOutput) {
  using type = int128_t;

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

TEST(base, uint128OpInput) {
  using compareType = uint;
  compareType compare;
  auto compareLimits = numeric_limits<compareType>();

  using type = uint128_t;
  type v;
  auto limits = numeric_limits<type>();

  // Empty input
  {
    string input = "";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 0);

    auto is = io::is(input);
    is >> v;
    EXPECT_ISTREAM(is, true, true, 0);
  }

  // Invalid character
  {
    string input = "x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 0);

    auto is = io::is(input);
    is >> v;
    EXPECT_ISTREAM(is, true, false, 0);
  }

  // Valid character, invalid input
  {
    string input = "-";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 1);

    auto is = io::is(input);
    is >> v;
    EXPECT_ISTREAM(is, true, true, 1);
  }

  // Valid character, invalid input
  {
    string input = "-x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 1);

    auto is = io::is(input);
    is >> v;
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
    is >> v;
    EXPECT_ISTREAM(is, false, true, 1);
    EXPECT_EQ(v, 1);
  }

  // Valid input, no EOF
  {
    string input = "999999x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, false, 6);
    EXPECT_EQ(compare, 999999);

    auto is = io::is(input);
    is >> v;
    EXPECT_ISTREAM(is, false, false, 6);
    EXPECT_EQ(v, 999999);
  }

  // MIN
  {
    string input = "0";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 1);
    EXPECT_EQ(compare, compareLimits.min());

    auto is = io::is(input);
    is >> v;
    EXPECT_ISTREAM(is, false, true, 1);
    EXPECT_EQ(v, limits.min());
  }

  // MIN - 1, e.g "-1", which is accepted
  {
    string input = "-1";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 2);
    EXPECT_EQ(compare, compareLimits.max());

    auto is = io::is(input);
    is >> v;
    EXPECT_ISTREAM(is, false, true, 2);
    EXPECT_EQ(v, limits.max());
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
    is >> v;
    EXPECT_ISTREAM(is, false, true, 39);
    EXPECT_EQ(v, limits.max());
  }

  // MAX + 1, overflow
  {
    string compareInput = "4294967296";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 10);

    string input = "340282366920938463463374607431768211456";
    auto is = io::is(input);
    is >> v;
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
    is >> v;
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
    is >> v;
    EXPECT_ISTREAM(is, true, true, 41);
  }
}

TEST(base, uint128OpOutput) {
  using type = uint128_t;

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

// EOF
