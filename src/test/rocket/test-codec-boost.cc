/*
 * test-codec-boost.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-boost-decl.h"
#include "rocket/codec-std-decl.h"
#include "rocket/codec-boost.h"
#include "rocket/codec-std.h"

#include "rocket/S.h"
#include "rocket/log.h"

using namespace rocket;
using namespace std;

namespace x3 = ::boost::spirit::x3;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(codec_boost, printRon_mix) {
  using Tuple = tuple<optional<log::LogLevel>, x3::variant<log::LogLevel, int, vector<int128_t>>>;
  Tuple t1(nullopt, log::LogLevel::info);
  Tuple t2(log::LogLevel::info, 3);
  using Vector = vector<Tuple>;
  Vector v{ t1, t2 };
  EXPECT_EQ(S << v, "[(null, 0:\"info\"), (\"info\", 1:3)]");

  ROCKET_CODEC_RON_PRINT_PARAMS({ .indent = true });
  EXPECT_EQ(S << v, "[\n  (null, 0:\"info\"),\n  (\"info\", 1:3)\n]");
}

// EOF
