/*
 * test-codec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/codec.h"
#include "rocket/codec/Formatted.h"

using namespace rocket::codec;

// #TEST ----------------------------------------------------------------------------------------------------

TEST(codec, FormattedBool) {
  EXPECT_EQ(encode<Formatted>(true), "true");
  EXPECT_EQ((decode<Formatted, bool>("true"sv)), true);
}

TEST(codec, FormattedOptionalI32) {
  using type = std::optional<i32>;

  EXPECT_EQ(encode<Formatted>(type()), "<none>");
  EXPECT_EQ(encode<Formatted>(type(42_i32)), "42");

  EXPECT_EQ((decode<Formatted, type>("<none>"sv)), nullopt);
  EXPECT_EQ((decode<Formatted, type>("42"sv)), 42_i32);
}

#if 0 // XXX
TEST(std, optionalAndVectorInTypeLoopFormat) {
  using type = optional<vector<optional<i32>>>;
  type val1 = nullopt;
  EXPECT_EQ(fmt::format("{:}", val1), "<none>");
  type val2 = vector<optional<i32>> { optional<i32>(1), nullopt, optional<i32>(3) };
  EXPECT_EQ(fmt::format("{}", val2), "[1, <none>, 3]");
}
#endif

#if 0 // XXX
TEST(std, vectorAndOptionalInTypeLoopFormat) {
  using type = vector<optional<vector<i32>>>;
  type val1 = {};
  EXPECT_EQ(fmt::format("{}", val1), "[]");
  type val2 = type { vector<i32> { vector<i32> { 1, 2 } } };
  EXPECT_EQ(fmt::format("{}", val2), "[[1, 2]]");
}
#endif

// EOF
