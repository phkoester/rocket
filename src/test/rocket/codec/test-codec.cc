/*
 * test-codec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/codec.h"
#include "rocket/codec/Formatted.h"

using namespace rocket::codec;

// #TEST ----------------------------------------------------------------------------------------------------

TEST(codec, FormattedBool) {
  EXPECT_EQ(encode<Formatted>(true), "true");
  EXPECT_EQ((decode<Formatted, bool>("true"sv)), true);
}

TEST(codec, FormattedOptionalI32) {
  using type = std::optional<i32>;

  EXPECT_EQ(encode<Formatted>(type()), "<none>");
  EXPECT_EQ(encode<Formatted>(type(42_i32)), "42");

  EXPECT_EQ((decode<Formatted, type>("<none>"sv)), nullopt);
  EXPECT_EQ((decode<Formatted, type>("42"sv)), 42_i32);
}

// EOF
