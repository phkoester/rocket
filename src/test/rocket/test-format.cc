/*
 * test-format.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/format.h"

#include <locale>
#include <span>
#include <unordered_map>
#include <unordered_set>

#define MODULE rocketFormat

// Tests ====================================================================================================

// Global ---------------------------------------------------------------------------------------------------

TEST(MODULE, bool) {
  EXPECT_EQ(fmt::format("{}", false), "false");
  EXPECT_EQ(fmt::format("{}", true), "true");
}

TEST(MODULE, char) {
  EXPECT_EQ(fmt::format("{}", 'a'), "a");
  EXPECT_EQ(fmt::format("{:?}", 'a'), "'a'");
  EXPECT_EQ(fmt::format("{}", '\x01'), "\x01");
  EXPECT_EQ(fmt::format("{:?}", '\x01'), "'\\x01'");
  EXPECT_EQ(fmt::format("{:?}", '\xff'), "'\\xff'");
}

TEST(MODULE, unsignedChar) {
  EXPECT_EQ(fmt::format("{}", static_cast<unsigned char>('a')), "97");
  EXPECT_EQ(fmt::format("{:X}", static_cast<unsigned char>(-1)), "FF");
  EXPECT_EQ(fmt::format("{:#x}", static_cast<unsigned char>(-1)), "0xff");
}

TEST(MODULE, char32) {
  EXPECT_EQ(fmt::format(U"{}", U'a'), U"a");
  EXPECT_EQ(fmt::format(U"{:?}", U'€'), U"'€'");
}

TEST(MODULE, charPtr) {
  EXPECT_EQ(fmt::format("{}", "hello"), "hello");
  EXPECT_EQ(fmt::format("{:?}", "hello"), "\"hello\"");
  EXPECT_EQ(fmt::format("{}", "a\bc"), "a\bc");
  EXPECT_EQ(fmt::format("{:?}", "a\bc"), "\"a\\x08c\"");
  EXPECT_EQ(fmt::format("{}", "⊕"), "⊕");
  EXPECT_EQ(fmt::format("{:?}", "⊕"), "\"⊕\"");
  // U+01F9D1 (Adult), U+200D (ZWJ), U+01F33E (Ear of rice)
  EXPECT_EQ(fmt::format("{}", "🧑‍🌾"), "🧑‍🌾");
  EXPECT_EQ(fmt::format("{:?}", "🧑‍🌾"), "\"🧑\\u200d🌾\"");
}

TEST(MODULE, char32Ptr) {
  EXPECT_EQ(fmt::format(U"{}", U"hello"), U"hello");
}

// `rocket` -------------------------------------------------------------------------------------------------

TEST(MODULE, rocketI8) {
  EXPECT_EQ(fmt::format("{:+}", 42_i8), "+42");
}

TEST(MODULE, rocketU8) {
  EXPECT_EQ(fmt::format("{}", static_cast<u8>(-1)), "255");
}

TEST(MODULE, rocketI16) {
  EXPECT_EQ(fmt::format("{:+}", 42_i16), "+42");
}

TEST(MODULE, rocketU16) {
  EXPECT_EQ(fmt::format("{}", static_cast<u16>(-1)), "65535");
}

TEST(MODULE, rocketI32) {
  EXPECT_EQ(fmt::format("{:+}", 42_i32), "+42");
}

TEST(MODULE, rocketU32) {
  EXPECT_EQ(fmt::format("{}", static_cast<u32>(-1)), "4294967295");
}

TEST(MODULE, rocketI64) {
  EXPECT_EQ(fmt::format("{:+}", 42_i64), "+42");
}

TEST(MODULE, rocketU64) {
  EXPECT_EQ(fmt::format("{}", static_cast<u64>(-1)), "18446744073709551615");
}

namespace {

/// A locale that groups digits by three with `.` as the separator.
std::locale
groupingLocale() {
  struct Numpunct : std::numpunct<char> {
    char
    do_thousands_sep() const override {
      return '.';
    }

    std::string
    do_grouping() const override {
      return "\3";
    }
  };

  return std::locale(std::locale::classic(), new Numpunct());
}

} // namespace

TEST(MODULE, rocketI128) {
  EXPECT_EQ(fmt::format("{}", -42_i128), "-42");
  EXPECT_EQ(fmt::format("{}", 0_i128), "0");
  EXPECT_EQ(fmt::format("{}", numeric_limits<i128>::max()), "170141183460469231731687303715884105727");
  EXPECT_EQ(fmt::format("{}", numeric_limits<i128>::min()), "-170141183460469231731687303715884105728");

  // Fill, alignment, and width
  EXPECT_EQ(fmt::format("{:6}", -42_i128), "   -42");
  EXPECT_EQ(fmt::format("{:<6}", -42_i128), "-42   ");
  EXPECT_EQ(fmt::format("{:>6}", -42_i128), "   -42");
  EXPECT_EQ(fmt::format("{:^7}", -42_i128), "  -42  ");
  EXPECT_EQ(fmt::format("{:*^7}", -42_i128), "**-42**");
  EXPECT_EQ(fmt::format("{:>{}}", -42_i128, 6), "   -42");
  EXPECT_EQ(fmt::format("{:2}", -42_i128), "-42");
  EXPECT_EQ(fmt::format(U"{:€>6}", -42_i128), U"€€€-42");

  // Type
  EXPECT_EQ(fmt::format("{:d}", -42_i128), "-42");

  // Sign
  EXPECT_EQ(fmt::format("{:+}", 42_i128), "+42");
  EXPECT_EQ(fmt::format("{:+}", -42_i128), "-42");
  EXPECT_EQ(fmt::format("{:-}", 42_i128), "42");
  EXPECT_EQ(fmt::format("{: }", 42_i128), " 42");
  EXPECT_EQ(fmt::format("{: }", -42_i128), "-42");

  // Zero padding
  EXPECT_EQ(fmt::format("{:06}", 42_i128), "000042");
  EXPECT_EQ(fmt::format("{:06}", -42_i128), "-00042");
  EXPECT_EQ(fmt::format("{:+06}", 42_i128), "+00042");
  EXPECT_EQ(fmt::format("{:*>06}", -42_i128), "***-42"); // Alignment overrides zero padding

  // Bases
  EXPECT_EQ(fmt::format("{:b}", 5_i128), "101");
  EXPECT_EQ(fmt::format("{:#b}", 5_i128), "0b101");
  EXPECT_EQ(fmt::format("{:#B}", 5_i128), "0B101");
  EXPECT_EQ(fmt::format("{:o}", 8_i128), "10");
  EXPECT_EQ(fmt::format("{:#o}", 8_i128), "010");
  EXPECT_EQ(fmt::format("{:x}", 255_i128), "ff");
  EXPECT_EQ(fmt::format("{:X}", 255_i128), "FF");
  EXPECT_EQ(fmt::format("{:#x}", 255_i128), "0xff");
  EXPECT_EQ(fmt::format("{:#X}", 255_i128), "0XFF");
  EXPECT_EQ(fmt::format("{:x}", -255_i128), "-ff");
  EXPECT_EQ(fmt::format("{:#x}", -255_i128), "-0xff");
  EXPECT_EQ(fmt::format("{:#010x}", 255_i128), "0x000000ff");
  EXPECT_EQ(fmt::format("{:+#x}", 255_i128), "+0xff");
  EXPECT_EQ(fmt::format("{:x}", numeric_limits<i128>::max()), "7fffffffffffffffffffffffffffffff");
  EXPECT_EQ(fmt::format("{:x}", numeric_limits<i128>::min()), "-80000000000000000000000000000000");

  // Localization
  EXPECT_EQ(fmt::format("{:L}", 1'234'567_i128), "1234567");
  EXPECT_EQ(fmt::format(groupingLocale(), "{:L}", 1'234'567_i128), "1.234.567");
  EXPECT_EQ(fmt::format(groupingLocale(), "{:L}", -1'234'567_i128), "-1.234.567");
  EXPECT_EQ(fmt::format(groupingLocale(), "{}", 1'234'567_i128), "1234567");
}

TEST(MODULE, rocketU128) {
  EXPECT_EQ(fmt::format("{}", 42_u128), "42");
  EXPECT_EQ(fmt::format("{}", 0_u128), "0");
  EXPECT_EQ(fmt::format("{}", static_cast<u128>(-1)), "340282366920938463463374607431768211455");
  EXPECT_EQ(fmt::format("{}", numeric_limits<u128>::max()), "340282366920938463463374607431768211455");

  // Fill, alignment, and width
  EXPECT_EQ(fmt::format("{:5}", 42_u128), "   42");
  EXPECT_EQ(fmt::format("{:<5}", 42_u128), "42   ");
  EXPECT_EQ(fmt::format("{:>5}", 42_u128), "   42");
  EXPECT_EQ(fmt::format("{:^6}", 42_u128), "  42  ");
  EXPECT_EQ(fmt::format("{:*^6}", 42_u128), "**42**");
  EXPECT_EQ(fmt::format("{:>{}}", 42_u128, 5), "   42");
  EXPECT_EQ(fmt::format("{:1}", 42_u128), "42");
  EXPECT_EQ(fmt::format(U"{:€>5}", 42_u128), U"€€€42");

  // Type
  EXPECT_EQ(fmt::format("{:d}", 42_u128), "42");

  // Sign is not supported for unsigned types
  EXPECT_THROW((void) fmt::format(fmt::runtime("{:+}"), 42_u128), fmt::format_error);
  EXPECT_THROW((void) fmt::format(fmt::runtime("{:-}"), 42_u128), fmt::format_error);
  EXPECT_THROW((void) fmt::format(fmt::runtime("{: }"), 42_u128), fmt::format_error);

  // Zero padding
  EXPECT_EQ(fmt::format("{:05}", 42_u128), "00042");
  EXPECT_EQ(fmt::format("{:#06x}", 42_u128), "0x002a");
  EXPECT_EQ(fmt::format("{:*<05}", 42_u128), "42***"); // Alignment overrides zero padding

  // Bases
  EXPECT_EQ(fmt::format("{:b}", 5_u128), "101");
  EXPECT_EQ(fmt::format("{:#b}", 5_u128), "0b101");
  EXPECT_EQ(fmt::format("{:#B}", 5_u128), "0B101");
  EXPECT_EQ(fmt::format("{:o}", 8_u128), "10");
  EXPECT_EQ(fmt::format("{:#o}", 8_u128), "010");
  EXPECT_EQ(fmt::format("{:x}", 255_u128), "ff");
  EXPECT_EQ(fmt::format("{:X}", 255_u128), "FF");
  EXPECT_EQ(fmt::format("{:#x}", 255_u128), "0xff");
  EXPECT_EQ(fmt::format("{:#X}", 255_u128), "0XFF");
  EXPECT_EQ(fmt::format("{:#010x}", 255_u128), "0x000000ff");
  EXPECT_EQ(fmt::format("{:x}", numeric_limits<u128>::max()), "ffffffffffffffffffffffffffffffff");
  EXPECT_EQ(fmt::format("{:#X}", numeric_limits<u128>::max()), "0XFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
  EXPECT_EQ(fmt::format("{:b}", u128(1) << 127), "1" + string(127, '0'));
  EXPECT_EQ(fmt::format("{:o}", numeric_limits<u128>::max()), "3" + string(42, '7'));

  // Localization
  EXPECT_EQ(fmt::format("{:L}", 1'234'567_u128), "1234567");
  EXPECT_EQ(fmt::format(groupingLocale(), "{:L}", 1'234'567_u128), "1.234.567");
  EXPECT_EQ(fmt::format(groupingLocale(), "{}", 1'234'567_u128), "1234567");
}

TEST(MODULE, rocketF32) {
  EXPECT_EQ(fmt::format("{}", numeric_limits<f32>::quiet_NaN()), "nan");
  EXPECT_EQ(fmt::format("{}", numeric_limits<f32>::signaling_NaN()), "nan");
  EXPECT_EQ(fmt::format("{}", numeric_limits<f32>::infinity()), "inf");
  EXPECT_EQ(fmt::format("{}", -numeric_limits<f32>::infinity()), "-inf");

  EXPECT_EQ(fmt::format("{:.5}", 0.999'999_f32), "1");
  EXPECT_EQ(fmt::format("{:.5}", 0.999'99_f32), "0.99999");
  // Round to 3 significant digits after the decimal point
  EXPECT_EQ(fmt::format("{:.3f}", 12.1236_f32), "12.124");
  EXPECT_EQ(fmt::format("{:.5}", 1.0_f32 / 3), "0.33333");
}

TEST(MODULE, rocketF64) {
  EXPECT_EQ(fmt::format("{:.5}", 0.999'999_f64), "1");
  EXPECT_EQ(fmt::format("{:.5}", 0.999'99_f64), "0.99999");
  // Round to 3 significant digits after the decimal point
  EXPECT_EQ(fmt::format("{:.3f}", 12.1236_f64), "12.124");
  EXPECT_EQ(fmt::format("{:.5}", 1.0_f64 / 3), "0.33333");
}

TEST(MODULE, rocketChronoZonedTime) {
  using namespace std::chrono;

  const auto* berlin = rocket::chrono::locate_zone("Europe/Berlin");
  const auto* newYork = rocket::chrono::locate_zone("America/New_York");
  const auto* utc = rocket::chrono::locate_zone("UTC");

  const sys_seconds summer = sys_days(2026y / October / 23d) + 10h;
  const sys_seconds winter = sys_days(2026y / December / 1d) + 10h;

  // Default format: `%F %T %Z`
  EXPECT_EQ(fmt::format("{}", rocket::chrono::zoned_time(berlin, summer)), "2026-10-23 12:00:00 CEST");
  EXPECT_EQ(fmt::format("{}", rocket::chrono::zoned_time(berlin, winter)), "2026-12-01 11:00:00 CET");
  EXPECT_EQ(fmt::format("{}", rocket::chrono::zoned_time(newYork, winter)), "2026-12-01 05:00:00 EST");
  EXPECT_EQ(fmt::format("{}", rocket::chrono::zoned_time(utc, winter)), "2026-12-01 10:00:00 UTC");

  // Subseconds, also before the epoch
  EXPECT_EQ(
    fmt::format("{}", rocket::chrono::zoned_time(berlin, sys_time<milliseconds>(summer + 123ms))),
    "2026-10-23 12:00:00.123 CEST");
  EXPECT_EQ(
    fmt::format("{}", rocket::chrono::zoned_time(berlin, sys_time<milliseconds>(sys_seconds() - 500ms))),
    "1970-01-01 00:59:59.500 CET");

  // UTC offsets
  EXPECT_EQ(
    fmt::format("{:%F %T %z}", rocket::chrono::zoned_time(berlin, summer)), "2026-10-23 12:00:00 +0200");
  EXPECT_EQ(fmt::format("{:%T %Ez}", rocket::chrono::zoned_time(berlin, summer)), "12:00:00 +02:00");
  EXPECT_EQ(fmt::format("{:%T %Oz}", rocket::chrono::zoned_time(newYork, winter)), "05:00:00 -05:00");
  EXPECT_EQ(fmt::format("{:%T %z}", rocket::chrono::zoned_time(utc, winter)), "10:00:00 +0000");
  EXPECT_EQ(fmt::format("{:%z}", rocket::chrono::zoned_time(newYork, winter)), "-0500");
  EXPECT_EQ(fmt::format("{:%z}", rocket::chrono::zoned_time(utc, winter)), "+0000");
  EXPECT_EQ(fmt::format("{:%Ez}", rocket::chrono::zoned_time(berlin, summer)), "+02:00");

  // Specifications starting with `%Z` or `%z`
  EXPECT_EQ(fmt::format("{:%Z}", rocket::chrono::zoned_time(berlin, summer)), "CEST");
  EXPECT_EQ(fmt::format("{:%Z %T}", rocket::chrono::zoned_time(berlin, summer)), "CEST 12:00:00");
  EXPECT_EQ(fmt::format("{:%z%Z}", rocket::chrono::zoned_time(berlin, summer)), "+0200CEST");
  EXPECT_EQ(fmt::format("{:>8%z}", rocket::chrono::zoned_time(berlin, summer)), "   +0200");

  // Other specifications
  EXPECT_EQ(fmt::format("{:%H:%M %Z}", rocket::chrono::zoned_time(berlin, summer)), "12:00 CEST");
  EXPECT_EQ(fmt::format("{:%%Z}", rocket::chrono::zoned_time(berlin, summer)), "%Z");

  // Fill and alignment
  EXPECT_EQ(fmt::format("{:>26}", rocket::chrono::zoned_time(berlin, summer)), "  2026-10-23 12:00:00 CEST");
  EXPECT_EQ(fmt::format("{:*<10%H:%M %Z}", rocket::chrono::zoned_time(berlin, summer)), "12:00 CEST");
  EXPECT_EQ(fmt::format("{:*<12%H:%M %Z}", rocket::chrono::zoned_time(berlin, summer)), "12:00 CEST**");
  EXPECT_EQ(fmt::format("{:*^12%Z}", rocket::chrono::zoned_time(berlin, summer)), "****CEST****");

  // Localization, literal text, and other character types
  EXPECT_EQ(fmt::format("{:L%H:%M %Z}", rocket::chrono::zoned_time(berlin, summer)), "12:00 CEST");
  EXPECT_EQ(fmt::format("{:%H{%Z}", rocket::chrono::zoned_time(berlin, summer)), "12{CEST");
  EXPECT_EQ(fmt::format(U"{}", rocket::chrono::zoned_time(berlin, summer)), U"2026-10-23 12:00:00 CEST");
  EXPECT_EQ(fmt::format(U"{:%Z %Ez}", rocket::chrono::zoned_time(berlin, summer)), U"CEST +02:00");

  // Invalid specifications
  EXPECT_THROW(
    (void) fmt::format(fmt::runtime("{:%}"), rocket::chrono::zoned_time(berlin, summer)), fmt::format_error);
  EXPECT_THROW(
    (void) fmt::format(fmt::runtime("{:x%Z}"), rocket::chrono::zoned_time(berlin, summer)),
    fmt::format_error);
}

// `std` ----------------------------------------------------------------------------------------------------

TEST(MODULE, stdInitializerList) {
  EXPECT_EQ(fmt::format("{}", initializer_list<i32> { 1, 2, 3 }), "[1, 2, 3]");
}

TEST(MODULE, stdMap) {
  EXPECT_EQ(
    fmt::format("{}", map<i32, string> { { 1, "one" }, { 2, "two" }, { 3, "three" } }),
    "{1: \"one\", 2: \"two\", 3: \"three\"}");
}

TEST(MODULE, stdPair) {
  EXPECT_EQ(fmt::format("{}", pair<i32, string> { 1, "one" }), "(1, \"one\")");
}

TEST(MODULE, stdSet) {
  EXPECT_EQ(fmt::format("{}", set<string> { "one", "two", "three" }), "{\"one\", \"three\", \"two\"}");
  EXPECT_EQ(fmt::format("{}", set<i32> { 1, 2, 3 }), "{1, 2, 3}");
  EXPECT_EQ(fmt::format("{::}", set<string> { "one", "two", "three" }), "{one, three, two}");
}

TEST(MODULE, stdSpan) {
  vector<i32> vec { 1, 2, 3 };
  EXPECT_EQ(fmt::format("{}", span<i32>(vec.begin(), vec.end())), "[1, 2, 3]");
}

TEST(MODULE, stdString) {
  static_assert(same_as<decltype("hello"s), string>);
  EXPECT_EQ(fmt::format("{}", "hello"s), "hello");
  EXPECT_EQ(fmt::format("{:?}", "hello"s), "\"hello\"");
  EXPECT_EQ(fmt::format("{}", "a\bc"s), "a\bc");
  EXPECT_EQ(fmt::format("{:?}", "a\bc"s), "\"a\\x08c\"");
  EXPECT_EQ(fmt::format("{}", "⊕"s), "⊕");
  EXPECT_EQ(fmt::format("{:?}", "⊕"s), "\"⊕\"");
  // U+01F9D1 (Adult), U+200D (ZWJ), U+01F33E (Ear of rice)
  EXPECT_EQ(fmt::format("{}", "🧑‍🌾"s), "🧑‍🌾");
  EXPECT_EQ(fmt::format("{:?}", "🧑‍🌾"s), "\"🧑\\u200d🌾\"");
}

TEST(MODULE, stdStringView) {
  static_assert(same_as<decltype("hello"sv), string_view>);
  EXPECT_EQ(fmt::format("{}", "hello"sv), "hello");
  EXPECT_EQ(fmt::format("{:?}", "hello"sv), "\"hello\"");
  EXPECT_EQ(fmt::format("{}", "a\bc"sv), "a\bc");
  EXPECT_EQ(fmt::format("{:?}", "a\bc"sv), "\"a\\x08c\"");
  EXPECT_EQ(fmt::format("{}", "⊕"sv), "⊕");
  EXPECT_EQ(fmt::format("{:?}", "⊕"sv), "\"⊕\"");
  // U+01F9D1 (Adult), U+200D (ZWJ), U+01F33E (Ear of rice)
  EXPECT_EQ(fmt::format("{}", "🧑‍🌾"sv), "🧑‍🌾");
  EXPECT_EQ(fmt::format("{:?}", "🧑‍🌾"sv), "\"🧑\\u200d🌾\"");
}

TEST(MODULE, stdTuple) {
  EXPECT_EQ(fmt::format("{}", tuple<i32, string, i64> { 1, "one", -1L }), "(1, \"one\", -1)");
}

TEST(MODULE, stdUnorderedMap) {
  EXPECT_THAT(
    fmt::format("{}", unordered_map<i32, string> { { 3, "three" }, { 2, "two" }, { 1, "one" } }),
    matchesRegex("\\{\\d: \".*\", \\d: \".*\", \\d: \".*\"\\}"));
}

TEST(MODULE, stdUnorderedSet) {
  EXPECT_THAT(fmt::format("{}", unordered_set<i32> { 1, 2, 3 }), matchesRegex("\\{\\d, \\d, \\d\\}"));
}

TEST(MODULE, stdVector) {
  EXPECT_EQ(fmt::format("{::0>5}", vector<i32> { 1, 2, 3 }), "[00001, 00002, 00003]");
  EXPECT_EQ(fmt::format("{}", vector<string> { "one", "two", "three" }), "[\"one\", \"two\", \"three\"]");
  EXPECT_EQ(fmt::format("{::}", vector<string> { "one", "two", "three" }), "[one, two, three]");
}

TEST(MODULE, stdChronoDuration) {
  using namespace std::chrono;

  EXPECT_EQ(fmt::format("{}", nanoseconds(1)), "1ns");
  EXPECT_EQ(fmt::format("{}", microseconds(1)), "1µs");
  EXPECT_EQ(fmt::format("{}", milliseconds(1)), "1ms");
  EXPECT_EQ(fmt::format("{}", seconds(1)), "1s");
  EXPECT_EQ(fmt::format("{}", minutes(1)), "1min");
  EXPECT_EQ(fmt::format("{}", hours(1)), "1h");
  EXPECT_EQ(fmt::format("{}", days(1)), "1d");
  EXPECT_EQ(fmt::format("{}", weeks(1)), "1[604800]s");
  EXPECT_EQ(fmt::format("{}", months(1)), "1[2629746]s");
  EXPECT_EQ(fmt::format("{}", years(1)), "1[31556952]s");
}

TEST(MODULE, stdChronoTimePoint) {
  using namespace std::chrono;

  const sys_days date = 2026y / October / 23d;

  EXPECT_EQ(fmt::format("{}", sys_seconds(date + 12h + 34min + 56s)), "2026-10-23 12:34:56");
  EXPECT_EQ(
    fmt::format("{}", sys_time<milliseconds>(date + 12h + 34min + 56s + 789ms)), "2026-10-23 12:34:56.789");
  EXPECT_EQ(fmt::format("{:%Y/%m/%d %H:%M}", sys_seconds(date + 12h + 34min)), "2026/10/23 12:34");
  EXPECT_EQ(fmt::format("{}", sys_seconds()), "1970-01-01 00:00:00");

  // Before the epoch
  const sys_days preEpoch = 1969y / December / 31d;
  EXPECT_EQ(fmt::format("{}", sys_seconds(preEpoch + 23h + 59min + 59s)), "1969-12-31 23:59:59");
  EXPECT_EQ(
    fmt::format("{}", sys_time<milliseconds>(preEpoch + 23h + 59min + 59s + 500ms)),
    "1969-12-31 23:59:59.500");
}

TEST(MODULE, stdChronoYearMonthDay) {
  using namespace std::chrono;

  EXPECT_EQ(fmt::format("{}", year_month_day { 2026y, October, 23d }), "2026-10-23");
  EXPECT_EQ(fmt::format("{}", year_month_day { 1970y, January, 1d }), "1970-01-01");
  EXPECT_EQ(fmt::format("{:%d.%m.%Y}", year_month_day { 2026y, October, 23d }), "23.10.2026");
  EXPECT_EQ(fmt::format("{:%F}", year_month_day { 2026y, October, 23d }), "2026-10-23");
  EXPECT_EQ(fmt::format("{}", vector { year_month_day { 2026y, October, 23d } }), "[2026-10-23]");
}

// EOF
