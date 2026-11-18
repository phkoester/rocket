// test-HashEncoder.cc

#include "rocket-test/rocket-test.h"

#include "rocket/codec/HashEncoder.h"
#include "rocket/chrono/chrono.h"
#include "rocket/math/random/random.h"
#include "rocket/reflect/reflect.h"
#include "rocket/unicode/Character.h"

#include <numeric>

using namespace rocket;
using namespace rocket::codec;
using namespace rocket::unicode;

#define MODULE rocketCodecHashEncoder

// `MyStruct` -----------------------------------------------------------------------------------------------

struct MyStruct {
  i32 ärger;
  bool ökonom;
  string übermut;
  vector<i32> vec;

  ROCKET_REFLECT_MEMBERS(MyStruct, Index, (ärger)(ökonom)(übermut)(vec));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyStruct, Index); // NOLINT(*-internal-linkage)
ROCKET_REFLECT_MEMBERS_DEFINE(, MyStruct, Index);

// Tests ====================================================================================================

TEST(MODULE, Enum) {
  const HashEncoder<> encoder;
  enum Color : u8 { red = 0, green, blue };
  EXPECT_EQ(encoder.encode(blue), 2);
}

TEST(MODULE, i128) {
  const HashEncoder<> encoder;
  EXPECT_NE(encoder.encode(1234_i128), 0);
}

TEST(MODULE, Pointer) {
  const HashEncoder<> encoder;
  void* ptr = nullptr; // NOLINT
  EXPECT_EQ(encoder.encode(ptr), 0);
}

/// Tests that the order of insertion does not affect the hash value for ordered sets.
TEST(MODULE, SetOrdered) {
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
TEST(MODULE, SetUnordered) {
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
TEST(MODULE, MapOrdered) {
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
TEST(MODULE, MapUnordered) {
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
TEST(MODULE, BimapOrdered) {
  const HashEncoder<> encoder;

  const u64 N = 100;
  vector<u64> vec(N);
  ranges::iota(vec, 0);

  auto gen = math::random::gen();
  set<u64> hashes;

  for (u64 i = 0; i < N; ++i) {
    ranges::shuffle(vec, gen);
    bimap<u64, string> map;
    for (const auto& elem : vec) {
      map.left.insert({ elem, fmt::format("{}", elem) });
    }
    const u64 hash = encoder.encode(map);
    hashes.insert(hash);
  }

  EXPECT_EQ(hashes.size(), 1);
}

/// Tests that the order of insertion does not affect the hash value for unordered bimaps.
TEST(MODULE, BimapUnordered) {
  const HashEncoder<> encoder;

  const u64 N = 100;
  vector<u64> vec(N);
  ranges::iota(vec, 0);

  auto gen = math::random::gen();
  set<u64> hashes;

  for (u64 i = 0; i < N; ++i) {
    ranges::shuffle(vec, gen);
    unordered_bimap<u64, string> map;
    for (const auto& elem : vec) {
      map.left.insert({ elem, fmt::format("{}", elem) });
    }
    const u64 hash = encoder.encode(map);
    hashes.insert(hash);
  }

  EXPECT_EQ(hashes.size(), 1);
}

TEST(MODULE, Duration) {
  using namespace std::chrono;

  const u64 count = 1;
  const HashEncoder<> encoder;
  const seconds s(count);
  const u64 sHash = encoder.encode(s);
  EXPECT_NE(sHash, 0);
  const milliseconds ms(count);
  const u64 msHash = encoder.encode(ms);
  EXPECT_NE(msHash, 0);
  // Test that the hash value is different for same counts but different durations
  EXPECT_NE(msHash, sHash);
}

TEST(MODULE, Date) {
  using namespace std::chrono;

  const HashEncoder<> encoder;
  const year_month_day val(year(1970), month(1), day(1));
  EXPECT_NE(encoder.encode(val), 0);
}

TEST(MODULE, HourMinuteSecond) {
  using namespace std::chrono;

  const HashEncoder<> encoder;
  const hh_mm_ss val(2h);
  EXPECT_EQ(val.hours(), hours(2));
  EXPECT_EQ(val.minutes(), minutes(0));
  EXPECT_EQ(val.seconds(), seconds(0));
  EXPECT_NE(encoder.encode(val), 0);
}

TEST(MODULE, TimeZone) {
  using namespace std::chrono;

  const auto* current = rocket::chrono::current_zone();
  const HashEncoder<> encoder;
  EXPECT_NE(encoder.encode(current), 0);
}

TEST(MODULE, Time) {
  using namespace std::chrono;

  const auto val = rocket::chrono::now<system_clock>();
  const HashEncoder<> encoder;
  EXPECT_NE(encoder.encode(val), 0);
}

TEST(MODULE, ZonedTime) {
  using namespace std::chrono;

  const auto val = rocket::chrono::zoned_time(rocket::chrono::current_zone(), rocket::chrono::now<system_clock>());
  const HashEncoder<> encoder;
  EXPECT_NE(encoder.encode(val), 0);
}

TEST(MODULE, DeclaredMyStruct) {
  const MyStruct val { 42, true, "hello", { 1, 2, 3 } };
  const HashEncoder<> encoder;
  EXPECT_NE(encoder.encode(val), 0);
}

TEST(MODULE, Character) {
  const auto val = "hello"_c;
  const HashEncoder<> encoder;
  EXPECT_NE(encoder.encode(val), 0);
}

TEST(MODULE, CharacterView) {
  const auto val = "hello"_cv;
  const HashEncoder<> encoder;
  EXPECT_NE(encoder.encode(val), 0);
}

// EOF
