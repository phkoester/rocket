/*
 * test-HashEncoder.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/HashEncoder.h"
#include "rocket/math/random/random.h"
#include "rocket/reflect/reflect.h"

#include <fmt/ranges.h>

#include <numeric>

using namespace rocket;
using namespace rocket::codec;

// #MyStruct ------------------------------------------------------------------------------------------------

struct MyStruct {
  i32 ärger;
  bool ökonom;
  string übermut;
  vector<i32> vec;

  ROCKET_REFLECT_MEMBERS(MyStruct, Index, (ärger)(ökonom)(übermut)(vec));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyStruct, Index); // NOLINT(*-internal-linkage)
ROCKET_REFLECT_MEMBERS_DEFINE(, MyStruct, Index);

// #TEST ----------------------------------------------------------------------------------------------------

TEST(HashEncoder, Enum) {
  const HashEncoder<> encoder;
  enum Color : u8 { red = 0, green, blue };
  EXPECT_EQ(encoder.encode(blue), 2);
}

#ifdef ROCKET_HAS_128
TEST(HashEncoder, i128) {
  const HashEncoder<> encoder;
  EXPECT_EQ(encoder.encode(1234_i128), 1234);
}
#endif

TEST(HashEncoder, Pointer) {
  const HashEncoder<> encoder;
  void* const ptr = nullptr; // XXX
  EXPECT_EQ(encoder.encode(ptr), 0);
}

/// Tests that the order of insertion does not affect the hash value for ordered sets.
TEST(HashEncoder, SetOrdered) {
  const HashEncoder<> encoder;

  const u64 N = 100;
  vector<u64> vec(N);
  ranges::iota(vec, 0);

  auto gen = math::random::gen();
  set<u64> hashes;

  for (u64 i = 0; i < N; ++i) {
    ranges::shuffle(vec, gen);
    const set<u64> set(vec.begin(), vec.end());
    const u64 hash = encoder.encode(set);
    hashes.insert(hash);
  }

  EXPECT_EQ(hashes.size(), 1);
}

/// Tests that the order of insertion does not affect the hash value for unordered sets.
TEST(HashEncoder, SetUnordered) {
  const HashEncoder<> encoder;

  const u64 N = 100;
  vector<u64> vec(N);
  ranges::iota(vec, 0);

  auto gen = math::random::gen();
  set<u64> hashes;

  for (u64 i = 0; i < N; ++i) {
    ranges::shuffle(vec, gen);
    const unordered_set<u64> set(vec.begin(), vec.end());
    const u64 hash = encoder.encode(set);
    hashes.insert(hash);
  }

  EXPECT_EQ(hashes.size(), 1);
}

/// Tests that the order of insertion does not affect the hash value for ordered maps.
TEST(HashEncoder, MapOrdered) {
  const HashEncoder<> encoder;

  const u64 N = 100;
  vector<u64> vec(N);
  ranges::iota(vec, 0);

  auto gen = math::random::gen();
  set<u64> hashes;

  for (u64 i = 0; i < N; ++i) {
    ranges::shuffle(vec, gen);
    map<u64, string> map;
    for (const auto& elem : vec) {
      map.emplace(elem, fmt::format("{}", elem));
    }
    const u64 hash = encoder.encode(map);
    hashes.insert(hash);
  }

  EXPECT_EQ(hashes.size(), 1);
}

/// Tests that the order of insertion does not affect the hash value for unordered maps.
TEST(HashEncoder, MapUnordered) {
  const HashEncoder<> encoder;

  const u64 N = 100;
  vector<u64> vec(N);
  ranges::iota(vec, 0);

  auto gen = math::random::gen();
  set<u64> hashes;

  for (u64 i = 0; i < N; ++i) {
    ranges::shuffle(vec, gen);
    unordered_map<u64, string> map;
    for (const auto& elem : vec) {
      map.emplace(elem, fmt::format("{}", elem));
    }
    const u64 hash = encoder.encode(map);
    hashes.insert(hash);
  }

  EXPECT_EQ(hashes.size(), 1);
}

/// Tests that the order of insertion does not affect the hash value for ordered bimaps.
TEST(HashEncoder, BimapOrdered) {
  const HashEncoder<> encoder;

  const u64 N = 100;
  vector<u64> vec(N);
  ranges::iota(vec, 0);

  auto gen = math::random::gen();
  set<u64> hashes;

  for (u64 i = 0; i < N; ++i) {
    ranges::shuffle(vec, gen);
    Bimap<u64, string> map;
    for (const auto& elem : vec) {
      map.left.insert({ elem, fmt::format("{}", elem) });
    }
    const u64 hash = encoder.encode(map);
    hashes.insert(hash);
  }

  EXPECT_EQ(hashes.size(), 1);
}

/// Tests that the order of insertion does not affect the hash value for unordered bimaps.
TEST(HashEncoder, BimapUnordered) {
  const HashEncoder<> encoder;

  const u64 N = 100;
  vector<u64> vec(N);
  ranges::iota(vec, 0);

  auto gen = math::random::gen();
  set<u64> hashes;

  for (u64 i = 0; i < N; ++i) {
    ranges::shuffle(vec, gen);
    UnorderedBimap<u64, string> map;
    for (const auto& elem : vec) {
      map.left.insert({ elem, fmt::format("{}", elem) });
    }
    const u64 hash = encoder.encode(map);
    hashes.insert(hash);
  }

  EXPECT_EQ(hashes.size(), 1);
}

TEST(HashEncoder, DeclaredMyStruct) {
  const MyStruct val { 42, true, "hello", { 1, 2, 3 } };
  const HashEncoder<> encoder;
  EXPECT_NE(encoder.encode(val), 0);
}

// EOF
