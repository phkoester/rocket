/*
 * test-scnlib.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include <scn/ranges.h>
#include <scn/regex.h>

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(scnlib, scanI32) {
  {
    auto input = "123"sv;
    auto result = scn::scan<i32>(input, "{}");
    EXPECT_TRUE(result);
    auto i = result->value();
    static_assert(is_same_v<decltype(i), i32>);
    EXPECT_EQ(i, 123);
    EXPECT_EQ(result->begin() - input.begin(), 3);
  }

  {
    auto input = "123, 456"sv;
    auto result = scn::scan<i32, i32>(input, "{}, {}");
    EXPECT_TRUE(result);
    auto[i1, i2] = result->values();
    static_assert(is_same_v<decltype(i1), i32>);
    EXPECT_EQ(i1, 123);
    static_assert(is_same_v<decltype(i2), i32>);
    EXPECT_EQ(i2, 456);
    EXPECT_EQ(result->begin() - input.begin(), 8);
  }
}

TEST(scnlib, scanU32Hex) {
  {
    auto result = scn::scan<u32>("1234abCD", "{:x}");
    EXPECT_TRUE(result);
    auto [v] = result->values();
    EXPECT_EQ(v, 0x1234ABCD);
    // `result.error()` may not be called here!
  }

  {
    auto input = "abCDXXXX"sv;
    auto result = scn::scan<u32>(input, "{:x}");
    EXPECT_TRUE(result);
    auto [v] = result->values();
    static_assert(is_same_v<decltype(v), u32>);
    EXPECT_EQ(v, 0xABCD);
    EXPECT_EQ(result->begin() - input.begin(), 4);
  }

  {
    auto input = "abCDeXXX"sv;
    string fmt = "{:x}";
    auto result = scn::scan<u32>(input, scn::runtime_format(fmt));
    EXPECT_TRUE(result);
    auto [v] = result->values();
    static_assert(is_same_v<decltype(v), u32>);
    EXPECT_EQ(v, 0xABCDE);
    EXPECT_EQ(result->begin() - input.begin(), 5);
  }

  {
    auto input = "abCDXXXX"sv;
    auto result = scn::scan<u32>(input, "{:8x}");
    EXPECT_FALSE(result);
    // `result->begin()` may not be called here!
  }

  {
    auto input = "12345678"sv;
    auto result = scn::scan<u32>(input, "{:8x}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result->value(), 0x12345678);
  }
}

TEST(scnlib, scanI128) {
  auto result = scn::scan<i128>("12345678901234567890", "{}");
  auto i = result->value();
  EXPECT_EQ(fmt::format("{}", i), "12345678901234567890");
}

#if 0
TEST(scnlib, scanRegex) {
  auto result = scn::scan<scn::regex_matches>("abc123", "{:/([a-z]+)([0-9]+)/}");
  const scn::regex_matches& matches = result->value();
  EXPECT_EQ(matches.size(), 3);
  for (size_t i = 0; i < matches.size(); ++i) {
    optional<scn::regex_matches::match_type> match = matches.at(i);
    if (i == 0) { EXPECT_EQ(match->get(), "abc123"); }
    if (i == 1) { EXPECT_EQ(match->get(), "abc"); }
    if (i == 2) { EXPECT_EQ(match->get(), "123"); }
  }
}
#endif

TEST(scnlib, scanVectorInt) {
  auto v1 = vector<int> { 1, 2, 3 };
  string input = fmt::format("{}", v1); // "[1, 2, 3]", 9 chars
  fmt::println("input: {}", input);
  auto result = scn::scan<vector<int>>(input, "{}");
  EXPECT_TRUE(result);
  auto v2 = result->value();
  EXPECT_EQ(v2, v1);
  EXPECT_EQ(result->begin() - input.begin(), 9);
}

// EOF
