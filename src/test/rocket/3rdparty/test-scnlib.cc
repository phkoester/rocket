/*
 * test-scnlib.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/format/std.h"
#include "rocket/system/system.h"

#include <scn/chrono.h>
#include <scn/ranges.h>
#include <scn/regex.h>

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(scnlib, scanI32) {
  {
    auto input = "123"sv;
    auto result = scn::scan<i32>(input, "{}");
    ASSERT_TRUE(result);
    auto val = result->value();
    static_assert(is_same_v<decltype(val), i32>);
    EXPECT_EQ(val, 123);
    EXPECT_EQ(result->begin() - input.begin(), 3);
  }

  {
    auto input = "123, 456"sv;
    auto result = scn::scan<i32, i32>(input, "{}, {}");
    ASSERT_TRUE(result);
    auto [val1, val2] = result->values();
    static_assert(is_same_v<decltype(val1), i32>);
    EXPECT_EQ(val1, 123);
    static_assert(is_same_v<decltype(val2), i32>);
    EXPECT_EQ(val2, 456);
    EXPECT_EQ(result->begin() - input.begin(), 8);
  }
}

TEST(scnlib, scanU32Hex) {
  {
    auto result = scn::scan<u32>("1234abCD", "{:x}");
    ASSERT_TRUE(result);
    auto [val] = result->values();
    EXPECT_EQ(val, 0x1234ABCD);
    // `result.error()` may not be called here!
  }

  {
    auto input = "abCDXXXX"sv;
    auto result = scn::scan<u32>(input, "{:x}");
    ASSERT_TRUE(result);
    auto [val] = result->values();
    static_assert(is_same_v<decltype(val), u32>);
    EXPECT_EQ(val, 0xABCD);
    EXPECT_EQ(result->begin() - input.begin(), 4);
  }

  {
    auto input = "abCDeXXX"sv;
    string fmt = "{:x}";
    auto result = scn::scan<u32>(input, scn::runtime_format(fmt));
    ASSERT_TRUE(result);
    auto [val] = result->values();
    static_assert(is_same_v<decltype(val), u32>);
    EXPECT_EQ(val, 0xABCDE);
    EXPECT_EQ(result->begin() - input.begin(), 5);
  }

  {
    auto input = "abCDXXXX"sv;
    auto result = scn::scan<u32>(input, "{:8x}");
    ASSERT_FALSE(result);
    // `result->begin()` may not be called here!
  }

  {
    auto input = "12345678"sv;
    auto result = scn::scan<u32>(input, "{:8x}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 0x12345678);
  }
}

TEST(scnlib, scanI128) {
  auto result = scn::scan<i128>("12345678901234567890", "{}");
  auto val = result->value();
  EXPECT_EQ(fmt::format("{}", val), "12345678901234567890");
}

TEST(scnlib, scanString) {
  {
    auto result = scn::scan<string, string>("[a    ][bbb  ]", "[{: <5}][{: <5}]");
    ASSERT_TRUE(result);
    auto [val1, val2] = result->values();
    EXPECT_EQ(val1, "a");
    EXPECT_EQ(val2, "bbb");
  }

  {
    auto result = scn::scan<string_view>("\"hi\"", "{}");
    ASSERT_TRUE(result);
    auto val = result->value();
    EXPECT_EQ(val, "\"hi\"");
  }
}

TEST(scnlib, scanMap) {
  using type = map<int, double>;

  auto val1 = type { { 1, 1.11 }, { 2, 2.22 }, { 3, 3.33 } };
  string input = fmt::format("{}", val1); // "{1: 1.11, 2: 2.22, 3: 3.33}", 27 chars
  auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  auto val2 = result->value();
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(result->begin() - input.begin(), 27);
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

TEST(scnlib, scanPair) {
  using type = pair<bool, int>;

  auto val1 = type { true, 12 };
  string input = fmt::format("{}", val1); // "(true, 12)", 10 chars
  auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  auto val2 = result->value();
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(result->begin() - input.begin(), 10);
}

TEST(scnlib, scanSet) {
  using type = set<int>;

  auto val1 = type { 1, 2, 3 };
  string input = fmt::format("{}", val1); // "{1, 2, 3}", 9 chars
  auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  auto val2 = result->value();
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(result->begin() - input.begin(), 9);
}

// XXX
TEST(scnlib, scanTimePoint) {
  // This interferes somehow ...
  // system::env::set("TZ", "America/Godthab");

  using TimePoint = chrono::system_clock::time_point;
  TimePoint val1 = chrono::system_clock::now();
  string input = std::format("{:%Y-%m-%d %H:%M:%S}", val1); // "2026-01-19 15:46:10.049025520", 29 chars
  auto result = scn::scan<TimePoint>(input, "{:%Y-%m-%d %H:%M:%.S}");
  ASSERT_TRUE(result);
  auto val2 = result->value();
  EXPECT_EQ(result->begin() - input.begin(), 29);

  // The time zone of the scanned time point is unclear. The following adjustment seems to convert the
  // time point to UTC
  const auto* tz = std::chrono::current_zone();
  auto info = tz->get_info(val1);
  val2 += info.offset;

  EXPECT_EQ(val2, val1);
}

TEST(scnlib, scanTuple) {
  using type = tuple<bool, int, double>;

  auto val1 = type { true, 1, 2.22 };
  string input = fmt::format("{}", val1); // "(true, 1, 2.22)", 15 chars
  auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  auto val2 = result->value();
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(result->begin() - input.begin(), 15);
}

TEST(scnlib, scanVector) {
  using type = vector<int>;

  auto val1 = type { 1, 2, 3 };
  string input = fmt::format("{}", val1); // "[1, 2, 3]", 9 chars
  auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  auto val2 = result->value();
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(result->begin() - input.begin(), 9);
}

// EOF
