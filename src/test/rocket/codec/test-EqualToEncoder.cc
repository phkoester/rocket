/*
 * test-EqualToEncoder.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/EqualToEncoder.h"
#include "rocket/reflect/reflect.h"

using namespace rocket;
using namespace rocket::codec;

// `MyStruct` -----------------------------------------------------------------------------------------------

struct MyStruct {
  int ärger;
  bool ökonom;
  string übermut;
  vector<int> vec;

  ROCKET_REFLECT_MEMBERS(MyStruct, Index, (ärger)(ökonom)(übermut)(vec));
  ROCKET_REFLECT_MEMBERS(MyStruct, Three, (ärger)(ökonom)(übermut));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyStruct, Index); // NOLINT(*-internal-linkage)
ROCKET_REFLECT_MEMBERS_DEFINE(, MyStruct, Index);

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(EqualToEncoder, Bool) {
  const EqualToEncoder<> encoder;
  EXPECT_TRUE(encoder.encode(false, false));
  EXPECT_FALSE(encoder.encode(false, true));
  EXPECT_FALSE(encoder.encode(true, false));
  EXPECT_TRUE(encoder.encode(true, true));
}

TEST(EqualToEncoder, String) {
  const EqualToEncoder<> encoder;
  EXPECT_TRUE(encoder.encode("a"sv, "a"sv));
  EXPECT_FALSE(encoder.encode("a"sv, "b"sv));
  EXPECT_FALSE(encoder.encode("b"sv, "a"sv));
  EXPECT_TRUE(encoder.encode("b"sv, "b"sv));
}

TEST(EqualToEncoder, ListVector) {
  using type = vector<i32>;
  const type a { 1, 2, 3 };
  const type b { 3, 2, 1 };

  const EqualToEncoder<> encoder;
  EXPECT_TRUE(encoder.encode(a, a));
  EXPECT_FALSE(encoder.encode(a, b));
  EXPECT_FALSE(encoder.encode(b, a));
  EXPECT_TRUE(encoder.encode(b, b));
}

TEST(EqualToEncoder, SetUnordered) {
  using type = unordered_set<string>;
  const type a { "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten" };
  const type b { "ten", "nine", "eight", "seven", "six", "five", "four", "three", "two", "one" };

  const EqualToEncoder<> encoder;

  EXPECT_TRUE(encoder.encode(a, a));
  EXPECT_TRUE(encoder.encode(a, b));
  EXPECT_TRUE(encoder.encode(b, a));
  EXPECT_TRUE(encoder.encode(b, b));

  EXPECT_FALSE(encoder.encode(a, type {}));
  EXPECT_FALSE(encoder.encode(a, type { "one", "two", "three" }));

  const type c { "one", "two", "three", "one", "two", "three", "one", "two", "three" };
  const type d { "one", "two", "three" };
  EXPECT_TRUE(encoder.encode(c, d));
}

TEST(EqualToEncoder, MapUnordered) {
  using type = unordered_map<i32, string>;
  const type a { { 1, "one" }, { 2, "two" }, { 3, "three" } };
  const type b { { 3, "three" }, { 2, "two" }, { 1, "one" } };
  const type c { { 1, "one" }, { 2, "two" }, { 3, "free" } };

  const EqualToEncoder<> encoder;

  EXPECT_TRUE(encoder.encode(a, a));
  EXPECT_TRUE(encoder.encode(a, b));
  EXPECT_TRUE(encoder.encode(b, a));
  EXPECT_TRUE(encoder.encode(b, b));

  EXPECT_FALSE(encoder.encode(a, c));
  EXPECT_FALSE(encoder.encode(c, a));
}

TEST(EqualToEncoder, BimapUnordered) {
  using type = UnorderedBimap<i32, string>;
  const type a = makeUnorderedBimap<i32, string>({ { 1, "one" }, { 2, "two" }, { 3, "three" } });
  const type b = makeUnorderedBimap<i32, string>({ { 3, "three" }, { 2, "two" }, { 1, "one" } });
  const type c = makeUnorderedBimap<i32, string>({ { 1, "one" }, { 2, "two" }, { 3, "free" } });

  const EqualToEncoder<> encoder;

  EXPECT_TRUE(encoder.encode(a, a));
  EXPECT_TRUE(encoder.encode(a, b));
  EXPECT_TRUE(encoder.encode(b, a));
  EXPECT_TRUE(encoder.encode(b, b));

  EXPECT_FALSE(encoder.encode(a, c));
  EXPECT_FALSE(encoder.encode(c, a));
}

TEST(EqualToEncoder, MyStruct) {
  const MyStruct a { 1, true, "one", { 1, 2, 3 } };
  const MyStruct b { 1, true, "one", { 1, 2, 3 } };
  const MyStruct c { 1, true, "two", { 1, 2, 3 } };

  const EqualToEncoder<> encoder;

  EXPECT_TRUE(encoder.encode(a, b));
  EXPECT_TRUE(encoder.encode(b, a));

  EXPECT_FALSE(encoder.encode(a, c));
  EXPECT_FALSE(encoder.encode(c, a));
}

// EOF
