/*
 * test-codec-rocket.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-rocket-decl.h"
#include "rocket/codec-std-decl.h"
#include "rocket/codec-rocket.h"
#include "rocket/codec-std.h"

#include "rocket/S.h"

using namespace rocket;
using namespace std;
using namespace testing;

// 'TEST' ---------------------------------------------------------------------------------------------------

TEST(codec_rocket, printRon_Type) {
  EXPECT_EQ(S << Type::of<int>(), "'int'");
  EXPECT_EQ(S << Type::of<Type>(), "'rocket::Type'");
}

TEST(codec_rocket, printRon_math_RightOpenInterval_int) {
  using type = math::RightOpenInterval<int>;

  EXPECT_EQ((S << type {}), "∅");
  EXPECT_EQ((S << type { 1'000, 2'000 }), "[1'000,2'000)");
}

TEST(codec_rocket, parseRon_math_OpenInterval_int) {
  using type = math::OpenInterval<int>;

  type v;

  {
    auto is = io::is("∅");
    parseRon(is, v);
    EXPECT_EQ(v, {});
  }

  {
    auto is = io::is("{}");
    parseRon(is, v);
    EXPECT_EQ(v, {});
  }

  {
    auto is = io::is("(-inf,inf)");
    parseRon(is, v);
    EXPECT_EQ(v, (type { nullopt, nullopt }));
  }

  {
    auto is = io::is("(-∞,∞)");
    parseRon(is, v);
    EXPECT_EQ(v, (type { nullopt, nullopt }));
  }

  {
    auto is = io::is("(-∞,+∞)");
    parseRon(is, v);
    EXPECT_EQ(v, (type { nullopt, nullopt }));
  }
}

TEST(codec_rocket, parseRon_math_RightOpenInterval_int) {
  using type = math::RightOpenInterval<int>;

  type v;

  {
    auto is = io::is("∅");
    parseRon(is, v);
    EXPECT_EQ(v, {});
  }

  {
    auto is = io::is("[1'000, 2'000)");
    parseRon(is, v);
    EXPECT_EQ(v, (type { 1'000, 2'000 }));
  }
}

// EOF
