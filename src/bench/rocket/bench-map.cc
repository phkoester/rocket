/*
 * bench-map.cc
 *
 * @todo add #std::flat_map when it's available
 */

#include "rocket-bench/rocket-bench.h"

#include "rocket/Bimap.h"
#include "rocket/math/random/random.h"

#include <array>
#include <unordered_map>

// Constants ------------------------------------------------------------------------------------------------

constexpr array<const char*, 7> SMALL_CONST_MAP_KEYS = {
  "alpha", "bravo", "charlie", "delta", "echo", "foxtrot", "golf"
};

// Variables ------------------------------------------------------------------------------------------------

auto gen = math::random::gen();

// #BENCH ---------------------------------------------------------------------------------------------------

void
map_smallConstMap(benchmark::State& state) {
  map<string_view, u64> map;
  for (u64 i = 0; i < SMALL_CONST_MAP_KEYS.size(); ++i) {
    map.emplace(SMALL_CONST_MAP_KEYS[i], i);
  }

  for (auto _ : state) { // NOLINT
    const u64 index = math::random::uniformInteger<u64>(gen, 0, SMALL_CONST_MAP_KEYS.size() - 1);
    const auto& key = SMALL_CONST_MAP_KEYS[index];
    [[maybe_unused]] auto _val = map.find(key);
  }
}

BENCHMARK(map_smallConstMap);

void
map_smallConstUnorderedMap(benchmark::State& state) {
  unordered_map<string_view, u64> map;
  for (u64 i = 0; i < SMALL_CONST_MAP_KEYS.size(); ++i) {
    map.emplace(SMALL_CONST_MAP_KEYS[i], i);
  }

  for (auto _ : state) { // NOLINT
    const u64 index = math::random::uniformInteger<u64>(gen, 0, SMALL_CONST_MAP_KEYS.size() - 1);
    const auto& key = SMALL_CONST_MAP_KEYS[index];
    [[maybe_unused]] auto _val = map.find(key);
  }
}

BENCHMARK(map_smallConstUnorderedMap);

void
map_smallConstBimap(benchmark::State& state) {
  rocket::Bimap<string_view, u64> map;
  for (u64 i = 0; i < SMALL_CONST_MAP_KEYS.size(); ++i) {
    map.insert({ SMALL_CONST_MAP_KEYS[i], i });
  }

  for (auto _ : state) { // NOLINT
    const u64 index = math::random::uniformInteger<u64>(gen, 0, SMALL_CONST_MAP_KEYS.size() - 1);
    const auto& key = SMALL_CONST_MAP_KEYS[index];
    [[maybe_unused]] auto _val = map.left.find(key);
  }
}

BENCHMARK(map_smallConstBimap);

void
map_smallConstUnorderedBimap(benchmark::State& state) {
  rocket::UnorderedBimap<string_view, u64> map;
  for (u64 i = 0; i < SMALL_CONST_MAP_KEYS.size(); ++i) {
    map.insert({ SMALL_CONST_MAP_KEYS[i], i });
  }

  for (auto _ : state) { // NOLINT
    const u64 index = math::random::uniformInteger<u64>(gen, 0, SMALL_CONST_MAP_KEYS.size() - 1);
    const auto& key = SMALL_CONST_MAP_KEYS[index];
    [[maybe_unused]] auto _val = map.left.find(key);
  }
}

BENCHMARK(map_smallConstUnorderedBimap);

// EOF
