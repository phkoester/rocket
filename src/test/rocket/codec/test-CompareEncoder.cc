/*
 * test-CompareEncoder.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/type-traits.h"

#include "rocket/codec/CompareEncoder.h"

using namespace rocket::codec;

// #MyStruct ------------------------------------------------------------------------------------------------

struct MyStruct {
  int a;
  string b;
};

// #MyDervivedStruct ----------------------------------------------------------------------------------------

struct MyDerivedStruct : MyStruct {
  float c;
};

// #TEST ----------------------------------------------------------------------------------------------------

TEST(CompareEncoder, CmpOrdering) {
  using codec::internal::CmpOrdering;
  using reflect::MemberRef;

  using Cmp = StdCompare;

  static_assert(std::is_same_v<CmpOrdering<Cmp, MemberRef<MyStruct, i32>>, CmpOrdering<Cmp, i32>>);
}

TEST(CompareEncoder, CmpCommonOrdering) {
  using codec::internal::CmpCommonOrdering;
  using reflect::MemberRef;

  using Cmp = StdCompare;

  static_assert(std::is_same_v<CmpCommonOrdering<Cmp, i32, i32, string>, std::strong_ordering>);
  static_assert(std::is_same_v<CmpCommonOrdering<Cmp, i32, f32>, std::partial_ordering>);
  static_assert(std::is_same_v<CmpCommonOrdering<Cmp, std::tuple<i32, i32, string>>, std::strong_ordering>);
  static_assert(std::is_same_v<CmpCommonOrdering<Cmp, std::tuple<i32, f32>>, std::partial_ordering>);
  static_assert(std::is_same_v<
    CmpCommonOrdering<Cmp, MemberRef<MyStruct, i32>, MemberRef<MyDerivedStruct, float>>,
    std::partial_ordering>);
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

TEST(CompareEncoder, ArraySpan) {
  vector<i32> v = { 1, 2, 3 };
  span<i32> span = v;

  CompareEncoder<> encoder;
  EXPECT_TRUE(std::is_eq(encoder.encode(span, span)));
}

TEST(CompareEncoder, ArrayVector) {
  using type = vector<i32>;
  CompareEncoder<> encoder;

  static_assert(std::is_same_v<decltype(encoder.encode(type(), type())), std::strong_ordering>);

  EXPECT_TRUE(std::is_eq(encoder.encode((type { 1, 2, 3 }), type( { 1, 2, 3 }))));
  EXPECT_TRUE(std::is_lt(encoder.encode((type { 1, 2, 3 }), type( { 1, 2, 4 }))));
  EXPECT_TRUE(std::is_lt(encoder.encode((type { 1, 2, 3 }), type( { 1, 2, 3, 4 }))));
  EXPECT_TRUE(std::is_gt(encoder.encode((type { 1, 2, 3, 4 }), type( { 1, 2, 3 }))));
}

// EOF
