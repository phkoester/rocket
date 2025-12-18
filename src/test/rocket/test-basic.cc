/*
 * test-basic.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/S.h"
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
 * Tests that the introductory table in `basic.h` is correct.
 */
TEST(basic, sizeof) {
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

TEST(basic, opInput_int128_t) {
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

TEST(basic, opOutput_int128_t) {
  using type = int128_t;

  type v;

  v = numeric_limits<int128_t>::min(); // -2^127
  EXPECT_EQ(S << raw(v), "-170141183460469231731687303715884105728");
  v = numeric_limits<int128_t>::max(); // 2^127 - 1
  EXPECT_EQ(S << raw(v), "170141183460469231731687303715884105727");
}

TEST(basic, opInput_uint128_t) {
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

TEST(basic, opOutput_uint128) {
  using type = uint128_t;

  type v;

  v = numeric_limits<type>::min(); // 0
  EXPECT_EQ(S << raw(v), "0");

  v = numeric_limits<type>::max(); // 2^128 - 1
  EXPECT_EQ(S << rocket::raw(v), "340282366920938463463374607431768211455");
}

// EOF
