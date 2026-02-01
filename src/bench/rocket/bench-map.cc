/*
 * bench-map.cc
 *
 * @todo add #std::flat_map when it's available
 */

#include "rocket-bench/rocket-bench.h"

#include "rocket/Bimap.h"
#include "rocket/math/random.h"

#include <array>
#include <unordered_map>

// Constants ------------------------------------------------------------------------------------------------

constexpr array<const char*, 7> KEYS = { "alpha", "bravo", "charlie", "delta", "echo", "foxtrot", "golf" };
constexpr array<i32, 7> VALS = { 0, 1, 2, 3, 4, 5, 6 };

// #BENCH ---------------------------------------------------------------------------------------------------

void
map_map(benchmark::State& state) {
  map<string_view, i32> map;
  for (u64 i = 0; i < KEYS.size(); ++i) {
    map.emplace(KEYS[i], VALS[i]);
  }

  auto gen = math::gen();

  for (auto _ : state) { // NOLINT
    const u64 index = math::random(gen, 0_u64, KEYS.size() - 1);
    const auto& key = KEYS[index];
    [[maybe_unused]] auto _val = map.find(key);
  }
}

BENCHMARK(map_map);

void
map_unorderedMap(benchmark::State& state) {
  unordered_map<string_view, i32> map;
  for (u64 i = 0; i < KEYS.size(); ++i) {
    map.emplace(KEYS[i], VALS[i]);
  }

  auto gen = math::gen();

  for (auto _ : state) { // NOLINT
    const u64 index = math::random(gen, 0_u64, KEYS.size() - 1);
    const auto& key = KEYS[index];
    [[maybe_unused]] auto _val = map.find(key);
  }
}

BENCHMARK(map_unorderedMap);

void
map_bimap(benchmark::State& state) {
  rocket::Bimap<string_view, i32> map;
  for (u64 i = 0; i < KEYS.size(); ++i) {
    map.insert({ KEYS[i], VALS[i] });
  }

  auto gen = math::gen();

  for (auto _ : state) { // NOLINT
    const u64 index = math::random(gen, 0_u64, KEYS.size() - 1);
    const auto& key = KEYS[index];
    [[maybe_unused]] auto _val = map.left.find(key);
  }
}

BENCHMARK(map_bimap);

void
map_unorderedBimap(benchmark::State& state) {
  rocket::UnorderedBimap<string_view, i32> map;
  for (u64 i = 0; i < KEYS.size(); ++i) {
    map.insert({ KEYS[i], VALS[i] });
  }

  auto gen = math::gen();

  for (auto _ : state) { // NOLINT
    const u64 index = math::random(gen, 0_u64, KEYS.size() - 1);
    const auto& key = KEYS[index];
    [[maybe_unused]] auto _val = map.left.find(key);
  }
}

BENCHMARK(map_unorderedBimap);

// EOF
