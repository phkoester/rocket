/*
 * test-base.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/io.h"

using namespace rocket;
using namespace std;
using namespace testing;

// `static_assert` ------------------------------------------------------------------------------------------

static_assert(sizeof(char32_t) == 4);
static_assert(is_unsigned_v<char32_t>);
static_assert(sizeof(char32_t) == sizeof(U' '));
static_assert(is_same_v<decltype(U' '), char32_t>);
static_assert(is_same_v<decltype(0.0L), long double>);

// `TEST` ---------------------------------------------------------------------------------------------------

/**
 * Tests that the introductory table in `base.h` is correct.
 */
TEST(base, sizeof) {
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

TEST(base, int128OpInput) {
  // using type = int128_t;
  using compareType = int;
  auto compareLimits = numeric_limits<compareType>();

  // type v;
  compareType compare;

  // Empty input
  {
    string input = "";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 0);
  }

  // Invalid character
  {
    string input = "x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 0);
  }

  // Valid character, invalid input
  {
    string input = "-";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 1);
  }

  // Valid character, invalid input
  {
    string input = "-x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 1);
  }

  // Valid input, EOF
  {
    string input = "-1";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 2);
    EXPECT_EQ(compare, -1);
  }

  // Valid input, no EOF
  {
    string input = "-999999x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, false, 7);
    EXPECT_EQ(compare, -999999);
  }

  // MIN
  {
    string compareInput = "-2147483648";

    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 11);
    EXPECT_EQ(compare, compareLimits.min());
  }

  // MIN - 1, overflow
  {
    string compareInput = "-2147483649";

    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 11);
  }

  // MAX
  {
    string compareInput = "2147483647";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 10);
    EXPECT_EQ(compare, compareLimits.max());
  }

  // MAX + 1, overflow
  {
    string compareInput = "2147483648";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 10);
  }
}

#if 0 // XXX
TEST(base, opInput_int128_t) {
  using type = int128_t;

  type v;

  {
    auto is = io::is("-170141183460469231731687303715884105729"); // min - 1
    is >> v;
    EXPECT_ISTREAM(is, true, false, 40);
  }

  {
    auto is = io::is("-170141183460469231731687303715884105728"); // min
    is >> v;
    EXPECT_EQ(v, numeric_limits<type>::min());
    EXPECT_ISTREAM(is, false, false, 40);
  }

  {
    auto is = io::is("170141183460469231731687303715884105727"); // max
    is >> v;
    EXPECT_EQ(v, numeric_limits<type>::max());
    EXPECT_ISTREAM(is, false, false, 39);
  }

  {
    auto is = io::is("170141183460469231731687303715884105728"); // max + 1
    is >> v;
    EXPECT_ISTREAM(is, true, false, 39);
  }
}

TEST(base, opOutput_int128_t) {
  using type = int128_t;

  type v;

  v = numeric_limits<int128_t>::min(); // -2^127
  EXPECT_EQ(fmt::format("{}", v), "-170141183460469231731687303715884105728");
  v = numeric_limits<int128_t>::max(); // 2^127 - 1
  EXPECT_EQ(fmt::format("{}", v), "170141183460469231731687303715884105727");
}

TEST(base, opInput_uint128_t) {
  using type = uint128_t;

  type v;

  {
    auto is = io::is();
    is >> v;
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("+");
    is >> v;
    EXPECT_ISTREAM(is, true, true, 1);
  }

  {
    auto is = io::is("++");
    is >> v;
    EXPECT_ISTREAM(is, true, false, 2);
  }

  {
    auto is = io::is("+1");
    is >> v;
    EXPECT_EQ(v, 1);
  }

  {
    auto is = io::is("99");
    is >> v;
    EXPECT_EQ(v, 99);
  }

  {
    auto is = io::is("0000099");
    is >> v;
    EXPECT_EQ(v, 99);
  }

  {
    auto is = io::is("+0000099");
    is >> v;
    EXPECT_EQ(v, 99);
  }

  {
    auto is = io::is("-1"); // min - 1
    is >> v;
    EXPECT_ISTREAM(is, true, false, 1);
  }

  {
    auto is = io::is("0"); // min
    is >> v;
    EXPECT_EQ(v, numeric_limits<type>::min());
    EXPECT_ISTREAM(is, false, false, 1);
  }

  {
    auto is = io::is("340282366920938463463374607431768211455"); // max
    is >> v;
    EXPECT_EQ(v, numeric_limits<type>::max());
    EXPECT_ISTREAM(is, false, false, 39);
  }

  {
    auto is = io::is("340282366920938463463374607431768211456"); // max + 1
    is >> v;
    EXPECT_ISTREAM(is, true, false, 39);
  }
}

TEST(base, opOutput_uint128) {
  using type = uint128_t;

  type v;

  v = numeric_limits<type>::min(); // 0
  EXPECT_EQ(fmt::format("{}", v), "0");

  v = numeric_limits<type>::max(); // 2^128 - 1
  EXPECT_EQ(fmt::format("{}", v), "340282366920938463463374607431768211455");
}
#endif

// EOF
