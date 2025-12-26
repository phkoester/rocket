/*
 * bench-codec-std.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-std-decl.h"
#include "rocket/codec-std.h"

#include "rocket/S.h"
#include "rocket/format-std.h"

#include "rocket-gtest/bench.h"

using namespace rocket;
using namespace std;

// Constants ------------------------------------------------------------------------------------------------

constexpr size_t N = 500'000;

// `TEST` ---------------------------------------------------------------------------------------------------

/**
 * Compare with `codec_std.S`.
 */
TEST(codec_std, fmt_print) {
  ostringstream oss;
  ROCKET_BENCH(N, [&] {
    fmt::print(oss, "{}\n", std::vector { 'h', 'e', 'l', 'l', 'o', '\n' } );
  });
}

/**
 * Compare with `codec_std.fmt_print`.
 */
TEST(codec_std, S) {
  ostringstream oss;
  ROCKET_BENCH(N, [&] {
    oss << (S << std::vector { 'h', 'e', 'l', 'l', 'o', '\n' }) << '\n';
  });
}

// EOF
