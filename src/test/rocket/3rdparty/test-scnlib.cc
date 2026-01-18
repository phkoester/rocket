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
    auto v = result->value();
    static_assert(is_same_v<decltype(v), i32>);
    EXPECT_EQ(v, 123);
    EXPECT_EQ(result->begin() - input.begin(), 3);
  }

  {
    auto input = "123, 456"sv;
    auto result = scn::scan<i32, i32>(input, "{}, {}");
    ASSERT_TRUE(result);
    auto[v1, v2] = result->values();
    static_assert(is_same_v<decltype(v1), i32>);
    EXPECT_EQ(v1, 123);
    static_assert(is_same_v<decltype(v2), i32>);
    EXPECT_EQ(v2, 456);
    EXPECT_EQ(result->begin() - input.begin(), 8);
  }
}

TEST(scnlib, scanU32Hex) {
  {
    auto result = scn::scan<u32>("1234abCD", "{:x}");
    ASSERT_TRUE(result);
    auto [v] = result->values();
    EXPECT_EQ(v, 0x1234ABCD);
    // `result.error()` may not be called here!
  }

  {
    auto input = "abCDXXXX"sv;
    auto result = scn::scan<u32>(input, "{:x}");
    ASSERT_TRUE(result);
    auto [v] = result->values();
    static_assert(is_same_v<decltype(v), u32>);
    EXPECT_EQ(v, 0xABCD);
    EXPECT_EQ(result->begin() - input.begin(), 4);
  }

  {
    auto input = "abCDeXXX"sv;
    string fmt = "{:x}";
    auto result = scn::scan<u32>(input, scn::runtime_format(fmt));
    ASSERT_TRUE(result);
    auto [v] = result->values();
    static_assert(is_same_v<decltype(v), u32>);
    EXPECT_EQ(v, 0xABCDE);
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
  auto v = result->value();
  EXPECT_EQ(fmt::format("{}", v), "12345678901234567890");
}

TEST(scnlib, scanString) {
  {
  auto result = scn::scan<string, string>("[a    ][bbb  ]", "[{: <5}][{: <5}]");
  ASSERT_TRUE(result);
  auto[v1, v2] = result->values();
  EXPECT_EQ(v1, "a");
  EXPECT_EQ(v2, "bbb");
  }

  {
    auto result = scn::scan<string_view>("\"hi\"", "{}");
    ASSERT_TRUE(result);
    auto v = result->value();
    EXPECT_EQ(v, "\"hi\"");
  }
}

TEST(scnlib, scanMap) {
  using type = map<int, double>;

  auto v1 = type { { 1, 1.11 }, { 2, 2.22 }, { 3, 3.33 } };
  string input = fmt::format("{}", v1); // "{1: 1.11, 2: 2.22, 3: 3.33}", 27 chars
  auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  auto v2 = result->value();
  EXPECT_EQ(v2, v1);
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

// There is no scanner for `optional`!

TEST(scnlib, scanPair) {
  using type = pair<bool, int>;

  auto v1 = type { true, 12 };
  string input = fmt::format("{}", v1); // "(true, 12)", 10 chars
  auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  auto v2 = result->value();
  EXPECT_EQ(v2, v1);
  EXPECT_EQ(result->begin() - input.begin(), 10);
}

TEST(scnlib, scanSet) {
  using type = set<int>;

  auto v1 = type { 1, 2, 3 };
  string input = fmt::format("{}", v1); // "{1, 2, 3}", 9 chars
  auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  auto v2 = result->value();
  EXPECT_EQ(v2, v1);
  EXPECT_EQ(result->begin() - input.begin(), 9);
}

TEST(scnlib, scanTimePoint) {
  // system::env::set("TZ", "America/Godthab");

  using TimePointType = std::chrono::system_clock::time_point;
  TimePointType v1 = std::chrono::system_clock::now();
  // "2026-01-18 18:00:09.123456789", 29 chars
  string input = fmt::format("{:%Y-%m-%d %H:%M:%S}", v1);
  fmt::println("v1: {}", input); // XXX

  // Scan time point
  auto result = scn::scan<TimePointType>(input, "{:%Y-%m-%d %H:%M:%.S}");
  ASSERT_TRUE(result);
  auto v2 = result->value();
  fmt::println("v2: {}", v2); // XXX
  EXPECT_EQ(result->begin() - input.begin(), 29);

  // @bug in scnlib: Adjust the time point
  /*
  auto* tz = std::chrono::current_zone();
  std::chrono::sys_info info = tz->get_info(v1);
  cout << "info.offset: " << info.offset << endl;
  v2 += info.offset;
  */
  v2 += 1h;

  EXPECT_EQ(v2, v1);
}

// XXX Why does this work?
TEST(scnlib, ChronoTimePoint)
{
  string input = "2024-09-10 23:11:10";
  fmt::println("tp1: {}", input); // XXX
  auto result = scn::scan<std::chrono::system_clock::time_point>(
      input, "{:%Y-%m-%d %H:%M:%S}");
  ASSERT_TRUE(result);
  auto tp = result->value();
  fmt::println("tp2: {}", tp); // "2024-09-10 21:11:10.000000000"

  auto val = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch());


  std::tm expected_tm {};
  expected_tm.tm_sec = 10;
  expected_tm.tm_min = 11;
  expected_tm.tm_hour = 23;
  expected_tm.tm_mday = 10;
  expected_tm.tm_mon = 8;
  expected_tm.tm_year = 2024 - 1900;
  expected_tm.tm_wday = 0;
  expected_tm.tm_yday = 0;
  expected_tm.tm_isdst = -1;
  auto expectedVal = std::chrono::seconds{std::mktime(&expected_tm)};

  EXPECT_EQ(val, expectedVal);
}

TEST(scnlib, scanTuple) {
  using type = tuple<bool, int, double>;

  auto v1 = type { true, 1, 2.22 };
  string input = fmt::format("{}", v1); // "(true, 1, 2.22)", 15 chars
  auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  auto v2 = result->value();
  EXPECT_EQ(v2, v1);
  EXPECT_EQ(result->begin() - input.begin(), 15);
}

TEST(scnlib, scanVector) {
  using type = vector<int>;

  auto v1 = type { 1, 2, 3 };
  string input = fmt::format("{}", v1); // "[1, 2, 3]", 9 chars
  auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  auto v2 = result->value();
  EXPECT_EQ(v2, v1);
  EXPECT_EQ(result->begin() - input.begin(), 9);
}

// EOF
