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
  EXPECT_TRUE(encoder.encode(false, false));
  EXPECT_FALSE(encoder.encode(false, true));
  EXPECT_FALSE(encoder.encode(true, false));
  EXPECT_TRUE(encoder.encode(true, true));
}

TEST(EqualToEncoder, String) {
  EqualToEncoder<> encoder;
  EXPECT_TRUE(encoder.encode("a"sv, "a"sv));
  EXPECT_FALSE(encoder.encode("a"sv, "b"sv));
  EXPECT_FALSE(encoder.encode("b"sv, "a"sv));
  EXPECT_TRUE(encoder.encode("b"sv, "b"sv));
}

TEST(EqualToEncoder, Array) {
  using type = vector<i32>;
  type a = { 1, 2, 3 };
  type b = { 3, 2, 1 };

  EqualToEncoder<> encoder;
  EXPECT_TRUE(encoder.encode(a, a));
  EXPECT_FALSE(encoder.encode(a, b));
  EXPECT_FALSE(encoder.encode(b, a));
  EXPECT_TRUE(encoder.encode(b, b));
}

TEST(EqualToEncoder, SetUnorderedString) {
  using type = unordered_set<string>;
  type a = { "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten" };
  type b = { "ten", "nine", "eight", "seven", "six", "five", "four", "three", "two", "one" };

  EqualToEncoder<> encoder;

  EXPECT_TRUE(encoder.encode(a, a));
  EXPECT_TRUE(encoder.encode(a, b));
  EXPECT_TRUE(encoder.encode(b, a));
  EXPECT_TRUE(encoder.encode(b, b));

  EXPECT_FALSE(encoder.encode(a, type {}));
  EXPECT_FALSE(encoder.encode(a, type { "one", "two", "three" }));

  type c = { "one", "two", "three", "one", "two", "three", "one", "two", "three" };
  type d = { "one", "two", "three" };
  EXPECT_TRUE(encoder.encode(c, d));
}

// XXX Hier testen mit span<const i32> als Key
TEST(EqualToEncoder, SetUnorderedVector) {
  using Vector = vector<i32>;

  struct VectorHash {
    u64
    operator()(const Vector& val) const {
      return HashEncoder<>().encode(val);
    }
  };

  using type = unordered_set<vector<i32>, VectorHash>;
  type a = { { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 } };
  type b = { { 7, 8, 9 }, { 4, 5, 6 }, { 1, 2, 3 } };

  EqualToEncoder<> encoder;

  EXPECT_TRUE(encoder.encode(a, a));
  EXPECT_TRUE(encoder.encode(a, b));
  EXPECT_TRUE(encoder.encode(b, a));
  EXPECT_TRUE(encoder.encode(b, b));
}

// EOF
