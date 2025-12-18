/**
 * @file bench.h
 *
 * Benchmarks with GoogleTest.
 */

#pragma once

#include <chrono>
#include <fmt/format.h>

// Macros ---------------------------------------------------------------------------------------------------

#define ROCKET_BENCH(n, f) { \
  using namespace std::chrono; \
  \
  auto t1 = steady_clock::now(); \
  for (size_t i = 0; i < n; ++i) { \
    f(); \
  } \
  auto t2 = steady_clock::now(); \
  auto ms = duration_cast<milliseconds>(t2 - t1); \
  auto ns = duration_cast<nanoseconds>(t2 - t1); \
  auto info = ::testing::UnitTest::GetInstance()->current_test_info(); \
  fmt::println("Bench \"{}.{}\": {:L} executions in {:L} ms ({:L} ns each)", \
      info->test_suite_name(), \
      info->name(), \
      n, \
      ms.count(), \
      ns.count() / n); \
}

// EOF
