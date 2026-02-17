/*
 * test-EqualToEncoder.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/EqualToEncoder.h"
#include "rocket/reflect/reflect-codec.h"

using namespace rocket;
using namespace rocket::codec;

// #MyStruct ------------------------------------------------------------------------------------------------

struct MyStruct {
  int ärger;
  bool ökonom;
  string übermut;
  vector<int> vec;

  ROCKET_REFLECT_MEMBERS(MyStruct, Index, (ärger)(ökonom)(übermut)(vec));
  ROCKET_REFLECT_MEMBERS(MyStruct, Three, (ärger)(ökonom)(übermut));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyStruct, Index);
ROCKET_REFLECT_MEMBERS_DEFINE(, MyStruct, Index);

// #TEST ----------------------------------------------------------------------------------------------------

TEST(EqualToEncoder, Bool) {
  EqualToEncoder<> encoder;
  EXPECT_EQ(encoder.encode(false, false), true);
  EXPECT_EQ(encoder.encode(false, true), false);
  EXPECT_EQ(encoder.encode(true, false), false);
  EXPECT_EQ(encoder.encode(true, true), true);
}

TEST(EqualToEncoder, String) {
  EqualToEncoder<> encoder;
  EXPECT_EQ(encoder.encode("a"sv, "a"sv), true);
  EXPECT_EQ(encoder.encode("a"sv, "b"sv), false);
  EXPECT_EQ(encoder.encode("b"sv, "a"sv), false);
  EXPECT_EQ(encoder.encode("b"sv, "b"sv), true);
}

TEST(EqualToEncoder, Array) {
  using type = vector<i32>;
  type a = { 1, 2, 3 };
  type b = { 3, 2, 1 };

  EqualToEncoder<> encoder;
  EXPECT_EQ(encoder.encode(a, a), true);
  EXPECT_EQ(encoder.encode(a, b), false);
  EXPECT_EQ(encoder.encode(b, a), false);
  EXPECT_EQ(encoder.encode(b, b), true);
}

// EOF
