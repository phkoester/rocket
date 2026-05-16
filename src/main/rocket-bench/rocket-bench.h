/**
 * @file rocket-bench.h
 *
 * This header must always be the first included file in a benchmark.
 *
 * Parameters:
 *
 * - `ROCKET_BENCH_NO_USING_NAMESPACE`: If defined, the `using namespace` directives are not included.
 */

#pragma once

// Early macros ---------------------------------------------------------------------------------------------

#undef ROCKET_TEST_PROTECTED
/// Use this macro instead of `protected` to allow access to protected members of a class when testing.
#define ROCKET_TEST_PROTECTED public
#undef ROCKET_TEST_PRIVATE
/// Use this macro instead of `private` to allow access to private members of a class when testing.
#define ROCKET_TEST_PRIVATE public

// Includes -------------------------------------------------------------------------------------------------

#include <benchmark/benchmark.h>

#include "rocket/assert.h"
#include "rocket/literal.h"
#include "rocket/rocket.h"

#ifndef ROCKET_BENCH_NO_USING_NAMESPACE

// NOLINTBEGIN
using namespace rocket;
using namespace std;
// NOLINTEND

#endif // ROCKET_BENCH_NO_USING_NAMESPACE

// Macros ---------------------------------------------------------------------------------------------------

/// Defines and registers a benchmark function.
#define BENCH(group, name, fn) \
  namespace { \
    void \
    group##_##name(benchmark::State& state) { \
      fn(state); \
    } \
    BENCHMARK(group##_##name); \
  }

// EOF
