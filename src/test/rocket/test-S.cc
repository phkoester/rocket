/*
 * test-S.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-std-decl.h"
#include "rocket/codec-std.h"

#include "rocket/S.h"

using namespace rocket;
using namespace std;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(S, stream) {
  EXPECT_EQ(S << false, "false");
  EXPECT_EQ(S << raw(false), "0");
  EXPECT_EQ(S << true, "true");
  EXPECT_EQ(S << raw(true), "1");
  
  EXPECT_EQ(S << 1'000'000, "1'000'000");
  EXPECT_EQ(S << raw(1'000'000), "1000000");

  EXPECT_EQ(S << "ä\nö", "ä\nö");
  EXPECT_EQ(S << "ä\nö"sv, "\"ä\\nö\"");
  EXPECT_EQ(S << "ä\nö"s, "\"ä\\nö\"");
  EXPECT_EQ(S << "a" << static_cast<const char*>(nullptr) << "b", "anullb");

  EXPECT_EQ(S << U"ä\nö", "ä\nö");
  EXPECT_EQ(S << U"ä\nö"sv, "\"ä\\nö\"");
  EXPECT_EQ(S << U"ä\nö"s, "\"ä\\nö\"");
  EXPECT_EQ(S << U"a" << static_cast<const char32_t*>(nullptr) << U"b", "anullb");
}

// EOF
