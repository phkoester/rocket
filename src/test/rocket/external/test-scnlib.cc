/*
 * test-scnlib.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/system/system.h"

#include <fmt/format.h>

#include <scn/chrono.h>
#include <scn/ranges.h>
#include <scn/regex.h>

// #TEST ----------------------------------------------------------------------------------------------------

TEST(scnlib, scanI32) {
  {
    const string_view input = "123"sv;
    const auto result = scn::scan<i32>(input, "{}");
    ASSERT_TRUE(result);
    const auto val = result->value();
    static_assert(is_same_v<decltype(val), const i32>);
    EXPECT_EQ(val, 123);
    EXPECT_EQ(result->begin() - input.begin(), 3);
  }

  {
    const string_view input = "123, 456"sv;
    const auto result = scn::scan<i32, i32>(input, "{}, {}");
    ASSERT_TRUE(result);
    const auto [val1, val2] = result->values();
    static_assert(is_same_v<decltype(val1), const i32>);
    EXPECT_EQ(val1, 123);
    static_assert(is_same_v<decltype(val2), const i32>);
    EXPECT_EQ(val2, 456);
    EXPECT_EQ(result->begin() - input.begin(), 8);
  }
}

TEST(scnlib, scanU32Hex) {
  {
    auto result = scn::scan<u32>("1234abCD", "{:x}");
    ASSERT_TRUE(result);
    const auto [val] = result->values();
    EXPECT_EQ(val, 0x1234ABCD);
    // `result.error()` may not be called here!
  }

  {
    const auto input = "abCDZZZZ"sv;
    const auto result = scn::scan<u32>(input, "{:x}");
    ASSERT_TRUE(result);
    const auto [val] = result->values();
    static_assert(is_same_v<decltype(val), const u32>);
    EXPECT_EQ(val, 0xABCD);
    EXPECT_EQ(result->begin() - input.begin(), 4);
  }

  {
    const auto input = "abCDeZZZ"sv;
    const string fmt = "{:x}";
    const auto result = scn::scan<u32>(input, scn::runtime_format(fmt));
    ASSERT_TRUE(result);
    const auto [val] = result->values();
    static_assert(is_same_v<decltype(val), const u32>);
    EXPECT_EQ(val, 0xABCDE);
    EXPECT_EQ(result->begin() - input.begin(), 5);
  }

  {
    const auto input = "abCDZZZZ"sv;
    const auto result = scn::scan<u32>(input, "{:8x}");
    ASSERT_FALSE(result);
    // `result->begin()` may not be called here!
  }

  {
    const auto input = "12345678"sv;
    const auto result = scn::scan<u32>(input, "{:8x}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 0x12345678);
  }
}

#ifdef ROCKET_HAS_128

TEST(scnlib, scanI128) {
  const auto result = scn::scan<i128>("12345678901234567890", "{}");
  const auto val = result->value();
  EXPECT_EQ(fmt::format("{}", val), "12345678901234567890");
}

TEST(scnlib, scanF128) {
  const auto result = scn::scan<f128>("3.14159265358979323846", "{}");
  const auto val = result->value();
  EXPECT_EQ(fmt::format("{}", val), "3.1415926535897932385");
}

#endif // ROCKET_HAS_128

TEST(scnlib, scanString) {
  {
    const auto result = scn::scan<string, string>("[a    ][bbb  ]", "[{: <5}][{: <5}]");
    ASSERT_TRUE(result);
    const auto [val1, val2] = result->values();
    EXPECT_EQ(val1, "a");
    EXPECT_EQ(val2, "bbb");
  }

  {
    const auto result = scn::scan<string_view>("\"hi\"", "{}");
    ASSERT_TRUE(result);
    const auto val = result->value();
    EXPECT_EQ(val, "\"hi\"");
  }
}

TEST(scnlib, scanMap) {
  using type = map<int, double>;

  const auto val1 = type { { 1, 1.11 }, { 2, 2.22 }, { 3, 3.33 } };
  const string input = fmt::format("{}", val1); // "{1: 1.11, 2: 2.22, 3: 3.33}", 27 chars
  const auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  const auto val2 = result->value();
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(result->begin() - input.begin(), 27);
}

#if 0 // NOLINT
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

  const auto val1 = type { true, 12 };
  const string input = fmt::format("{}", val1); // "(true, 12)", 10 chars
  const auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  const auto val2 = result->value();
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(result->begin() - input.begin(), 10);
}

TEST(scnlib, scanSet) {
  using type = set<i32>;

  const auto val1 = type { 1, 2, 3 };
  const string input = fmt::format("{}", val1); // "{1, 2, 3}", 9 chars
  const auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  const auto val2 = result->value();
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(result->begin() - input.begin(), 9);
}

/// @todo Wait for a fix in scnlib
TEST(scnlib, scanTimePoint) {
  // This interferes somehow ...
  // system::env::set("TZ", "America/Godthab");

  using TimePoint = chrono::time_point<chrono::system_clock, chrono::nanoseconds>;
  const TimePoint val1 = chrono::system_clock::now();
  const string input = std::format("{:%Y-%m-%d %H:%M:%S}", val1); // "2026-01-27 05:51:13.396968455", 29 chars
  const auto result = scn::scan<TimePoint>(input, "{:%Y-%m-%d %H:%M:%.S}");
  ASSERT_TRUE(result);
  auto val2 = result->value();
  EXPECT_EQ(result->begin() - input.begin(), 29);

  // The time zone of the scanned time point is unclear. The following adjustment seems to convert the
  // time point to UTC
  const auto* tz = std::chrono::current_zone();
  const auto info = tz->get_info(val1);
  val2 += info.offset;

  // Cast to microseconds, because there might be a rounding issue
  const auto val1Micros = chrono::time_point_cast<chrono::microseconds>(val1);
  const auto val2Micros = chrono::time_point_cast<chrono::microseconds>(val2);

  EXPECT_EQ(val2Micros, val1Micros);
}

TEST(scnlib, scanTuple) {
  using type = tuple<bool, int, double>;

  const auto val1 = type { true, 1, 2.22 };
  const string input = fmt::format("{}", val1); // "(true, 1, 2.22)", 15 chars
  const auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  const auto val2 = result->value();
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(result->begin() - input.begin(), 15);
}

TEST(scnlib, scanVector) {
  using type = vector<i32>;

  const auto val1 = type { 1, 2, 3 };
  const string input = fmt::format("{}", val1); // "[1, 2, 3]", 9 chars
  const auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  const auto val2 = result->value();
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(result->begin() - input.begin(), 9);
}

// EOF
