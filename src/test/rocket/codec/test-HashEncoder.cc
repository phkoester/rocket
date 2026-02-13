/*
 * test-HashEncoder.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Bimap.h"
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

  ROCKET_REFLECT_MEMBERS(MyStruct, index, (ärger)(ökonom)(übermut)(vec));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyStruct, index);
ROCKET_REFLECT_MEMBERS_DEFINE(, MyStruct, index);

// #TEST ----------------------------------------------------------------------------------------------------

TEST(HashEncoder, Enum) {
  HashEncoder<> encoder;
  enum Color : u8 { red = 0, green, blue };
  EXPECT_EQ(encoder.encode(blue), 2);
}

TEST(HashEncoder, Pointer) {
  HashEncoder<> encoder;
  EXPECT_EQ(encoder.encode(&encoder), reinterpret_cast<u64>(&encoder));
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

TEST(HashEncoder, MemberRefProviderMyStruct) {
  const MyStruct val { 42, true, "hello", { 1, 2, 3 } };
  HashEncoder<> encoder;
  EXPECT_NE(encoder.encode(val), 0);
}

// EOF
