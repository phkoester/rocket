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

// EOF
