/*
 * test-number.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/number.h"

using namespace rocket;
using namespace std;
using namespace testing;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(number, addLong) {
  EXPECT_EQ((add<long, int128_t>(-3L, 0)), -3);
  EXPECT_THAT(
    ([] { add<unsigned long, int128_t>(-3L, 0); }),
    ThrowsMessage<overflow_error>(HasSubstr("`unsigned long` overflow: -3 + 0")));
}

TEST(number, addInt128) {
  size_t pos = 7;
  long offset = -10;
  EXPECT_EQ((add<int128_t, int128_t>(pos, offset)), -3);
}

TEST(number, subLong) {
  EXPECT_EQ((sub<long, int128_t>(0, 3L)), -3);
  EXPECT_THAT(
    ([] { sub<unsigned long, int128_t>(0, 3L); }),
    ThrowsMessage<overflow_error>(HasSubstr("`unsigned long` overflow: 0 - 3")));
}

TEST(number, tryAddLong) {
  EXPECT_EQ((tryAdd<long, int128_t>(-3L, 0)), -3);
  EXPECT_FALSE((tryAdd<unsigned long, int128_t>(-3L, 0)));
}

TEST(number, trySubLong) {
  EXPECT_EQ((trySub<long, int128_t>(0, 3L)), -3);
  EXPECT_FALSE((trySub<unsigned long, int128_t>(0, 3L)));
}

// EOF
