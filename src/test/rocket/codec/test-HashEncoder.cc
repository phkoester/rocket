/*
 * test-HashEncoder.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/HashEncoder.h"
#include "rocket/math/random/random.h"
#include "rocket/reflect/reflect-codec.h"

#include <fmt/ranges.h>

#include <numeric>

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

/// Tests that the order of insertion does not affect the hash value for ordered sets.
TEST(HashEncoder, SetOrdered) {
  HashEncoder<> encoder;

  const u64 N = 100;
  vector<u64> vec(N);
  ranges::iota(vec, 0);

  auto gen = math::random::gen();
  set<u64> hashes;

  for (u64 i = 0; i < N; ++i) {
    ranges::shuffle(vec, gen);
    set<u64> set(vec.begin(), vec.end());
    u64 hash = encoder.encode(set);
    hashes.insert(hash);
  }

  EXPECT_EQ(hashes.size(), 1);
}

/// Tests that the order of insertion does not affect the hash value for unordered sets.
TEST(HashEncoder, SetUnordered) {
  HashEncoder<> encoder;

  const u64 N = 100;
  vector<u64> vec(N);
  ranges::iota(vec, 0);

  auto gen = math::random::gen();
  set<u64> hashes;

  for (u64 i = 0; i < N; ++i) {
    ranges::shuffle(vec, gen);
    unordered_set<u64> set(vec.begin(), vec.end());
    u64 hash = encoder.encode(set);
    hashes.insert(hash);
  }

  EXPECT_EQ(hashes.size(), 1);
}

/// Tests that the order of insertion does not affect the hash value for ordered maps.
TEST(HashEncoder, MapOrdered) {
  HashEncoder<> encoder;

  const u64 N = 100;
  vector<u64> vec(N);
  ranges::iota(vec, 0);

  auto gen = math::random::gen();
  set<u64> hashes;

  for (u64 i = 0; i < N; ++i) {
    ranges::shuffle(vec, gen);
    map<int, string> map;
    for (const auto& elem : vec) {
      map.emplace(elem, fmt::format("{}", elem));
    }
    u64 hash = encoder.encode(map);
    hashes.insert(hash);
  }

  EXPECT_EQ(hashes.size(), 1);
}

/// Tests that the order of insertion does not affect the hash value for unordered maps.
TEST(HashEncoder, MapUnordered) {
  HashEncoder<> encoder;

  const u64 N = 100;
  vector<u64> vec(N);
  ranges::iota(vec, 0);

  auto gen = math::random::gen();
  set<u64> hashes;

  for (u64 i = 0; i < N; ++i) {
    ranges::shuffle(vec, gen);
    unordered_map<int, string> map;
    for (const auto& elem : vec) {
      map.emplace(elem, fmt::format("{}", elem));
    }
    u64 hash = encoder.encode(map);
    hashes.insert(hash);
  }

  EXPECT_EQ(hashes.size(), 1);
}

/// Tests that the order of insertion does not affect the hash value for ordered bimaps.
TEST(HashEncoder, BimapOrdered) {
  HashEncoder<> encoder;

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
    u64 hash = encoder.encode(map);
    hashes.insert(hash);
  }

  EXPECT_EQ(hashes.size(), 1);
}

/// Tests that the order of insertion does not affect the hash value for unordered bimaps.
TEST(HashEncoder, BimapUnordered) {
  HashEncoder<> encoder;

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
    u64 hash = encoder.encode(map);
    hashes.insert(hash);
  }

  EXPECT_EQ(hashes.size(), 1);
}

TEST(HashEncoder, DeclaredMyStruct) {
  const MyStruct val { 42, true, "hello", { 1, 2, 3 } };
  HashEncoder<> encoder;
  EXPECT_NE(encoder.encode(val), 0);
}

// EOF
