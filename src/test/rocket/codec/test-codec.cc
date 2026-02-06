/*
 * test-codec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/codec.h"

using namespace rocket::codec;

// #TEST ----------------------------------------------------------------------------------------------------

TEST(codec, FormattedBool) {
  EXPECT_EQ(encode<Formatted>(true), "true");
  EXPECT_EQ((decode<Formatted, bool>("true"sv)), true);
}

// EOF
