/**
 * @file bench.h
 *
 * Benchmarks with GoogleTest.
 */

#pragma once

#include "rocket/format/format.h"

#include <chrono>

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Executes the function @p f @p n times and measures the excution time.
 *
 * @param n the number of times to run the benchmark
 * @param f the benchmark function
 */
#define ROCKET_BENCH(n, f) { \
  using namespace std::chrono; \
  \
  auto t1 = steady_clock::now(); \
  for (size_t i__ = 0; i__ < n; ++i__) { \
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
