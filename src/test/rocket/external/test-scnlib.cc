/*
 * test-scnlib.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/format.h"
#include "rocket/scan.h"
#include "rocket/scope.h"
#include "rocket/system/system.h"

#include <scn/regex.h>

#include <chrono>
#include <time.h> // `tzset` and `_tzset`, which <ctime> does not declare

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(scnlib, scanI32) {
  {
    const string_view input = "123"sv;
    const auto result = scn::scan<i32>(input, "{}");
    ASSERT_TRUE(result);
    const auto val = result->value();
    static_assert(same_as<decltype(val), const i32>);
    EXPECT_EQ(val, 123);
    EXPECT_EQ(result->begin() - input.begin(), 3);
  }

  {
    const string_view input = "123, 456"sv;
    const auto result = scn::scan<i32, i32>(input, "{}, {}");
    ASSERT_TRUE(result);
    const auto [val1, val2] = result->values();
    static_assert(same_as<decltype(val1), const i32>);
    EXPECT_EQ(val1, 123);
    static_assert(same_as<decltype(val2), const i32>);
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
    static_assert(same_as<decltype(val), const u32>);
    EXPECT_EQ(val, 0xABCD);
    EXPECT_EQ(result->begin() - input.begin(), 4);
  }

  {
    const auto input = "abCDeZZZ"sv;
    const string fmt = "{:x}";
    const auto result = scn::scan<u32>(input, scn::runtime_format(fmt));
    ASSERT_TRUE(result);
    const auto [val] = result->values();
    static_assert(same_as<decltype(val), const u32>);
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

TEST(scnlib, scanI128) {
  const auto result = scn::scan<i128>("12345678901234567890", "{}");
  const auto val = result->value();
  EXPECT_EQ(fmt::format("{}", val), "12345678901234567890");
}

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

// The scanner for `std::chrono::time_point` used to call `std::mktime`, which takes the fields for local
// time, although a `system_clock` time point is UTC. `cmake/patches/scnlib.patch` makes it take the fields
// as UTC. Under a time zone other than UTC, these tests fail without the patch
TEST(scnlib, scanChronoTimePoint) {
  using namespace std::chrono;
  using TimePoint = time_point<system_clock, nanoseconds>;

  // Makes the C runtime pick up the current value of `TZ`. Otherwise, Windows only reads it once
  const auto applyTz = [] {
#ifdef ROCKET_OS_WINDOWS
    _tzset();
#else
    tzset();
#endif
  };

  const auto oldTz = system::env::get<string>("TZ");
  ROCKET_SCOPE_EXIT([&] {
    if (oldTz) {
      system::env::set("TZ", *oldTz);
    } else {
      system::env::unset("TZ");
    }
    applyTz();
  });

  // A fixed UTC offset of +9 hours without DST, so the result depends on neither the machine nor the date.
  // The POSIX notation works on all platforms, unlike a name such as `Asia/Tokyo`
  system::env::set("TZ", "JST-9");
  applyTz();

  const sys_days day = 2026y / July / 15;

  {
    const string_view input = "2026-07-15 12:00:00.25"sv;
    const auto result = scn::scan<TimePoint>(input, "{:%Y-%m-%d %H:%M:%.S}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin() - input.begin(), 22);
    EXPECT_EQ(result->value(), day + 12h + 250ms);
  }

  {
    // A coarser duration truncates
    const auto result = scn::scan<sys_seconds>("2026-07-15T12:34:56", "{:%FT%T}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), day + 12h + 34min + 56s);
  }

  {
    // A UTC offset is applied
    const auto result = scn::scan<sys_seconds>("2026-07-15T12:00:00+0200", "{:%FT%T%z}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), day + 10h);
  }

  {
    // Before the epoch
    const auto result = scn::scan<sys_seconds>("1969-12-31T23:59:59", "{:%FT%T}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), sys_seconds(-1s));
  }
}

TEST(scnlib, scanChronoTimePointDayOfMonth) {
  using namespace std::chrono;

  // A day that does not exist in the month fails, instead of being moved into the next month
  EXPECT_FALSE(scn::scan<sys_seconds>("2026-02-30T00:00:00", "{:%FT%T}"));
  EXPECT_FALSE(scn::scan<sys_seconds>("2026-04-31T00:00:00", "{:%FT%T}"));
  EXPECT_FALSE(scn::scan<sys_seconds>("2026-02-29T00:00:00", "{:%FT%T}")); // Not a leap year
  EXPECT_FALSE(scn::scan<sys_seconds>("1900-02-29T00:00:00", "{:%FT%T}")); // Not a leap year

  const auto leap = scn::scan<sys_seconds>("2000-02-29T00:00:00", "{:%FT%T}"); // A leap year
  ASSERT_TRUE(leap);
  EXPECT_EQ(leap->value(), sys_days(2000y / February / 29));
}

// EOF
