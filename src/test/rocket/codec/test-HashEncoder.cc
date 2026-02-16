/*
 * test-HashEncoder.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/HashEncoder.h"
#include "rocket/reflect/reflect-codec.h"

#include <fmt/ranges.h>

using namespace rocket;
using namespace rocket::codec;

// #MyStruct ------------------------------------------------------------------------------------------------

struct MyStruct {
  int ärger;
  bool ökonom;
  string übermut;
  vector<int> vec;

  ROCKET_REFLECT_MEMBERS(MyStruct, Index, (ärger)(ökonom)(übermut)(vec));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyStruct, Index);
ROCKET_REFLECT_MEMBERS_DEFINE(, MyStruct, Index);

// #TEST ----------------------------------------------------------------------------------------------------

TEST(HashEncoder, Enum) {
  HashEncoder<> encoder;
  enum Color : u8 { red = 0, green, blue };
  EXPECT_EQ(encoder.encode(blue), 2);
}

#ifdef ROCKET_HAS_128
TEST(HashEncoder, i128) {
  HashEncoder<> encoder;
  EXPECT_EQ(encoder.encode(1234_i128), 1234);
}
#endif

TEST(HashEncoder, Pointer) {
  HashEncoder<> encoder;
  void* ptr = nullptr;
  EXPECT_EQ(encoder.encode(ptr), 0);
}

TEST(HashEncoder, Set) {
  const set<string> val = { "one", "two", "three" };
  HashEncoder<> encoder;
  EXPECT_NE(encoder.encode(val), 0);
}

TEST(HashEncoder, Map) {
  const map<string, int> val = { { "a", 1 }, { "b", 2 }, { "c", 3 } };
  HashEncoder<> encoder;
  EXPECT_NE(encoder.encode(val), 0);
}

TEST(HashEncoder, Bimap) {
  const auto val = makeBimap<string, i32>({ { "a", 1 }, { "b", 2 }, { "c", 3 } });
  HashEncoder<> encoder;
  EXPECT_NE(encoder.encode(val), 0);
}

TEST(HashEncoder, BimapUnordered) {
  const auto val = makeUnorderedBimap<string, i32>({ { "a", 1 }, { "b", 2 }, { "c", 3 } });
  HashEncoder<> encoder;
  EXPECT_NE(encoder.encode(val), 0);
}

TEST(HashEncoder, DeclaredMyStruct) {
  const MyStruct val { 42, true, "hello", { 1, 2, 3 } };
  HashEncoder<> encoder;
  EXPECT_NE(encoder.encode(val), 0);
}

// EOF
