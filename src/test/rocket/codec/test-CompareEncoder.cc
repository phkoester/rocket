/*
 * test-CompareEncoder.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/CompareEncoder.h"

using namespace rocket::codec;

// #TEST ----------------------------------------------------------------------------------------------------

TEST(CompareEncoder, TupeCmpOrdering) {
  using codec::internal::CmpOrdering;

  using Cmp = StdCompare;

  static_assert(std::is_same_v<CmpOrdering<Cmp, std::tuple<i32, i32, string>>::Type, std::strong_ordering>);
  static_assert(std::is_same_v<CmpOrdering<Cmp, std::tuple<i32, f32>>::Type, std::partial_ordering>);
}

TEST(CompareEncoder, Bool) {
  CompareEncoder<> encoder;

  static_assert(std::is_same_v<decltype(encoder.encode(false, false)), std::strong_ordering>);

  EXPECT_TRUE(std::is_eq(encoder.encode(false, false)));
  EXPECT_TRUE(std::is_lt(encoder.encode(false, true)));
  EXPECT_TRUE(std::is_gt(encoder.encode(true, false)));
  EXPECT_TRUE(std::is_eq(encoder.encode(true, true)));
}

TEST(CompareEncoder, String) {
  CompareEncoder<> encoder;

  static_assert(std::is_same_v<decltype(encoder.encode("a"sv, "a"sv)), std::strong_ordering>);

  EXPECT_TRUE(std::is_eq(encoder.encode("a"sv, "a"sv)));
  EXPECT_TRUE(std::is_lt(encoder.encode("a"sv, "b"sv)));
  EXPECT_TRUE(std::is_gt(encoder.encode("b"sv, "a"sv)));
  EXPECT_TRUE(std::is_eq(encoder.encode("b"sv, "b"sv)));
}

TEST(CompareEncoder, Array) {
  using type = vector<i32>;
  CompareEncoder<> encoder;

  static_assert(std::is_same_v<decltype(encoder.encode(type(), type())), std::strong_ordering>);

  EXPECT_TRUE(std::is_eq(encoder.encode((type { 1, 2, 3 }), type( { 1, 2, 3 }))));
  EXPECT_TRUE(std::is_lt(encoder.encode((type { 1, 2, 3 }), type( { 1, 2, 4 }))));
  EXPECT_TRUE(std::is_lt(encoder.encode((type { 1, 2, 3 }), type( { 1, 2, 3, 4 }))));
  EXPECT_TRUE(std::is_gt(encoder.encode((type { 1, 2, 3, 4 }), type( { 1, 2, 3 }))));
}

// EOF
