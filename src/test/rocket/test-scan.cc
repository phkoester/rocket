/*
 * test-scan.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/format.h"
#include "rocket/scan.h"
#include "rocket/scope.h"
#include "rocket/system/system.h"

#include <chrono>
#include <locale>
#include <optional>
#include <time.h> // `tzset` and `_tzset`, which `<ctime>` does not declare
#include <unordered_map>
#include <unordered_set>
#include <utility>

#define MODULE rocketScan

namespace {

// Utilities ================================================================================================

/// A scanned value and the number of code units consumed.
template<typename T>
using Scanned = optional<pair<T, i64>>;

/// Returns the expected result of a successful scan.
template<typename T>
Scanned<T>
ok(T val, i64 length) {
  return pair<T, i64>(val, length);
}

/// Scans a single value from `input`; the result is empty on failure.
template<typename T>
Scanned<T>
scanOne(string_view input, scn::scan_format_string<string_view, T> fmt) {
  const auto result = scn::scan<T>(input, fmt);
  if (not result) {
    return nullopt;
  }
  return pair<T, i64>(result->value(), result->begin() - input.begin());
}

/// Like `scanOne`, but with a locale.
template<typename T>
Scanned<T>
scanOne(const std::locale& loc, string_view input, scn::scan_format_string<string_view, T> fmt) {
  const auto result = scn::scan<T>(loc, input, fmt);
  if (not result) {
    return nullopt;
  }
  return pair<T, i64>(result->value(), result->begin() - input.begin());
}

/// Scans a single value from `input` with a runtime format string, returning the error code on failure.
template<typename T>
optional<decltype(declval<scn::scan_error>().code())>
scanError(string_view input, string_view fmt) {
  const auto result = scn::scan<T>(input, scn::runtime_format(fmt));
  if (result) {
    return nullopt;
  }
  return result.error().code();
}

/// A locale that groups digits by three with `,` as the separator.
std::locale
groupingLocale() {
  struct Numpunct : std::numpunct<char> {
    char
    do_thousands_sep() const override {
      return ',';
    }

    std::string
    do_grouping() const override {
      return "\3";
    }
  };

  return std::locale(std::locale::classic(), new Numpunct());
}

} // namespace

// Tests ====================================================================================================

// Global ---------------------------------------------------------------------------------------------------

TEST(MODULE, bool) {
  EXPECT_EQ(scanOne<bool>("false", "{}"), ok(false, 5));
  EXPECT_EQ(scanOne<bool>("true", "{}"), ok(true, 4));
  EXPECT_EQ(scanOne<bool>("  true", "{}"), ok(true, 6));
  EXPECT_EQ(scanOne<bool>("0", "{}"), ok(false, 1));
  EXPECT_EQ(scanOne<bool>("1", "{}"), ok(true, 1));

  // Textual or numeric only
  EXPECT_EQ(scanOne<bool>("true", "{:s}"), ok(true, 4));
  EXPECT_EQ(scanError<bool>("1", "{:s}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanOne<bool>("1", "{:i}"), ok(true, 1));
  EXPECT_EQ(scanError<bool>("true", "{:i}"), scn::scan_error::invalid_scanned_value);

  EXPECT_EQ(scanError<bool>("yes", "{}"), scn::scan_error::invalid_scanned_value);
}

TEST(MODULE, char) {
  EXPECT_EQ(scanOne<char>("a", "{}"), ok('a', 1));
  EXPECT_EQ(scanOne<char>("ab", "{}"), ok('a', 1));
  EXPECT_EQ(scanOne<char>(" a", "{}"), ok(' ', 1)); // Whitespace is not skipped
  EXPECT_EQ(scanError<char>("", "{}"), scn::scan_error::end_of_input);
}

TEST(MODULE, unsignedChar) {
  // Scanned as an integer, not as a character
  EXPECT_EQ(scanOne<unsigned char>("97", "{}"), ok(static_cast<unsigned char>(97), 2));
  EXPECT_EQ(scanOne<unsigned char>("ff", "{:x}"), ok(static_cast<unsigned char>(255), 2));
  EXPECT_EQ(scanError<unsigned char>("256", "{}"), scn::scan_error::value_positive_overflow);
}

TEST(MODULE, char32) {
  EXPECT_EQ(scanOne<char32_t>("a", "{}"), ok(U'a', 1));
  EXPECT_EQ(scanOne<char32_t>("€", "{}"), ok(U'€', 3));
}

// `rocket` -------------------------------------------------------------------------------------------------

TEST(MODULE, rocketI8) {
  EXPECT_EQ(scanOne<i8>("+42", "{}"), ok(42_i8, 3));
  EXPECT_EQ(scanOne<i8>("-128", "{}"), ok(numeric_limits<i8>::min(), 4));
  EXPECT_EQ(scanError<i8>("128", "{}"), scn::scan_error::value_positive_overflow);
  EXPECT_EQ(scanError<i8>("-129", "{}"), scn::scan_error::value_negative_overflow);
}

TEST(MODULE, rocketU8) {
  EXPECT_EQ(scanOne<u8>("255", "{}"), ok(static_cast<u8>(255), 3));
  EXPECT_EQ(scanError<u8>("256", "{}"), scn::scan_error::value_positive_overflow);
  EXPECT_EQ(scanError<u8>("-1", "{}"), scn::scan_error::invalid_scanned_value);
}

TEST(MODULE, rocketI16) {
  EXPECT_EQ(scanOne<i16>("+42", "{}"), ok(42_i16, 3));
  EXPECT_EQ(scanOne<i16>("-32768", "{}"), ok(numeric_limits<i16>::min(), 6));
  EXPECT_EQ(scanError<i16>("32768", "{}"), scn::scan_error::value_positive_overflow);
}

TEST(MODULE, rocketU16) {
  EXPECT_EQ(scanOne<u16>("65535", "{}"), ok(static_cast<u16>(65535), 5));
  EXPECT_EQ(scanError<u16>("65536", "{}"), scn::scan_error::value_positive_overflow);
}

TEST(MODULE, rocketI32) {
  EXPECT_EQ(scanOne<i32>("123", "{}"), ok(123, 3));

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

TEST(MODULE, rocketU32) {
  EXPECT_EQ(scanOne<u32>("4294967295", "{}"), ok(numeric_limits<u32>::max(), 10));

  // Hexadecimal, up to the first character that is not a digit
  EXPECT_EQ(scanOne<u32>("1234abCD", "{:x}"), ok(0x1234ABCDu, 8));
  EXPECT_EQ(scanOne<u32>("abCDZZZZ", "{:x}"), ok(0xABCDu, 4));
  EXPECT_EQ(scanOne<u32>("12345678", "{:8x}"), ok(0x12345678u, 8));
  EXPECT_EQ(scanError<u32>("abCDZZZZ", "{:8x}"), scn::scan_error::length_too_short);

  {
    const auto input = "abCDeZZZ"sv;
    const string fmt = "{:x}";
    const auto result = scn::scan<u32>(input, scn::runtime_format(fmt));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 0xABCDEu);
    EXPECT_EQ(result->begin() - input.begin(), 5);
  }
}

TEST(MODULE, rocketI64) {
  EXPECT_EQ(scanOne<i64>("9223372036854775807", "{}"), ok(numeric_limits<i64>::max(), 19));
  EXPECT_EQ(scanOne<i64>("-9223372036854775808", "{}"), ok(numeric_limits<i64>::min(), 20));
  EXPECT_EQ(scanError<i64>("9223372036854775808", "{}"), scn::scan_error::value_positive_overflow);
}

TEST(MODULE, rocketU64) {
  EXPECT_EQ(scanOne<u64>("18446744073709551615", "{}"), ok(numeric_limits<u64>::max(), 20));
  EXPECT_EQ(scanOne<u64>("ffffffffffffffff", "{:x}"), ok(numeric_limits<u64>::max(), 16));
  EXPECT_EQ(scanError<u64>("18446744073709551616", "{}"), scn::scan_error::value_positive_overflow);
}

TEST(MODULE, rocketI128) {
  EXPECT_EQ(scanOne<i128>("-42", "{}"), ok(-42_i128, 3));
  EXPECT_EQ(scanOne<i128>("+42", "{}"), ok(42_i128, 3));
  EXPECT_EQ(scanOne<i128>("0", "{}"), ok(0_i128, 1));
  EXPECT_EQ(scanOne<i128>("-0", "{}"), ok(0_i128, 2));
  EXPECT_EQ(scanOne<i128>(" \t\n42 ", "{}"), ok(42_i128, 5)); // Leading whitespace is skipped
  EXPECT_EQ(scanOne<i128>("42abc", "{}"), ok(42_i128, 2));
  EXPECT_EQ(scanOne<i128>("0042", "{}"), ok(42_i128, 4));
  EXPECT_EQ(scanOne<i128>("12345678901234567890", "{}"), ok(12345678901234567890_i128, 20));
  EXPECT_EQ(
    scanOne<i128>("170141183460469231731687303715884105727", "{}"), ok(numeric_limits<i128>::max(), 39));
  EXPECT_EQ(
    scanOne<i128>("-170141183460469231731687303715884105728", "{}"), ok(numeric_limits<i128>::min(), 40));
  EXPECT_EQ(
    scanError<i128>("170141183460469231731687303715884105728", "{}"),
    scn::scan_error::value_positive_overflow);
  EXPECT_EQ(
    scanError<i128>("-170141183460469231731687303715884105729", "{}"),
    scn::scan_error::value_negative_overflow);

  // Errors
  EXPECT_EQ(scanError<i128>("", "{}"), scn::scan_error::end_of_input);
  EXPECT_EQ(scanError<i128>("   ", "{}"), scn::scan_error::end_of_input);
  EXPECT_EQ(scanError<i128>("abc", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<i128>("-", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<i128>("42", "{:c}"), scn::scan_error::invalid_format_string);
  EXPECT_EQ(scanError<i128>("42", "{:s}"), scn::scan_error::invalid_format_string);
  EXPECT_EQ(scanError<i128>("42", "{:Lb}"), scn::scan_error::invalid_format_string);

  // Types
  EXPECT_EQ(scanOne<i128>("-42", "{:d}"), ok(-42_i128, 3));
  EXPECT_EQ(scanOne<i128>("42", "{:u}"), ok(42_i128, 2));
  EXPECT_EQ(scanError<i128>("-42", "{:u}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanOne<i128>("101", "{:b}"), ok(5_i128, 3));
  EXPECT_EQ(scanOne<i128>("0b101", "{:b}"), ok(5_i128, 5));
  EXPECT_EQ(scanOne<i128>("0B101", "{:B}"), ok(5_i128, 5));
  EXPECT_EQ(scanOne<i128>("17", "{:o}"), ok(15_i128, 2));
  EXPECT_EQ(scanOne<i128>("017", "{:o}"), ok(15_i128, 3));
  EXPECT_EQ(scanOne<i128>("0o17", "{:o}"), ok(15_i128, 4));
  EXPECT_EQ(scanOne<i128>("ff", "{:x}"), ok(255_i128, 2));
  EXPECT_EQ(scanOne<i128>("-0xFF", "{:X}"), ok(-255_i128, 5));
  EXPECT_EQ(scanOne<i128>("0x", "{:x}"), ok(0_i128, 1)); // The prefix without digits is just `0`
  EXPECT_EQ(scanOne<i128>("7fffffffffffffffffffffffffffffff", "{:x}"), ok(numeric_limits<i128>::max(), 32));
  EXPECT_EQ(scanOne<i128>("-80000000000000000000000000000000", "{:x}"), ok(numeric_limits<i128>::min(), 33));

  // Base detection
  EXPECT_EQ(scanOne<i128>("42", "{:i}"), ok(42_i128, 2));
  EXPECT_EQ(scanOne<i128>("0x1f", "{:i}"), ok(31_i128, 4));
  EXPECT_EQ(scanOne<i128>("0b101", "{:i}"), ok(5_i128, 5));
  EXPECT_EQ(scanOne<i128>("0o17", "{:i}"), ok(15_i128, 4));
  EXPECT_EQ(scanOne<i128>("017", "{:i}"), ok(15_i128, 3));
  EXPECT_EQ(scanOne<i128>("08", "{:i}"), ok(0_i128, 1));

  // Fill, alignment, and width (the minimum field width)
  EXPECT_EQ(scanOne<i128>("***-42", "{:*>}"), ok(-42_i128, 6));
  EXPECT_EQ(scanOne<i128>("-42***", "{:*<}"), ok(-42_i128, 6));
  EXPECT_EQ(scanOne<i128>("**-42**", "{:*^}"), ok(-42_i128, 7));
  EXPECT_EQ(scanOne<i128>("€€-42", "{:€>}"), ok(-42_i128, 9));
  EXPECT_EQ(scanOne<i128>("   -42", "{:6}"), ok(-42_i128, 6));
  EXPECT_EQ(scanOne<i128>("-42***", "{:*<6}"), ok(-42_i128, 6));
  EXPECT_EQ(scanError<i128>("-42", "{:6}"), scn::scan_error::length_too_short);
  EXPECT_EQ(scanError<i128>("-42**", "{:*<6}"), scn::scan_error::length_too_short);

  // Precision (the maximum field width)
  EXPECT_EQ(scanOne<i128>("12345", "{:.3}"), ok(123_i128, 3));
  EXPECT_EQ(scanOne<i128>("  12345", "{:.3}"), ok(1_i128, 3));
  EXPECT_EQ(scanOne<i128>("**12345", "{:*>.4}"), ok(12_i128, 4));
  EXPECT_EQ(scanOne<i128>("12345", "{:2.4}"), ok(1234_i128, 4));
  EXPECT_EQ(scanError<i128>("***12345", "{:*>.3}"), scn::scan_error::invalid_fill);

  // Localization
  EXPECT_EQ(scanOne<i128>("1,234,567", "{}"), ok(1_i128, 1));
  EXPECT_EQ(scanOne<i128>(groupingLocale(), "1,234,567", "{:L}"), ok(1'234'567_i128, 9));
  EXPECT_EQ(scanOne<i128>(groupingLocale(), "-1,234,567", "{:L}"), ok(-1'234'567_i128, 10));
  EXPECT_EQ(scanOne<i128>(groupingLocale(), "1,234,567", "{}"), ok(1_i128, 1));
}

TEST(MODULE, rocketU128) {
  EXPECT_EQ(scanOne<u128>("42", "{}"), ok(42_u128, 2));
  EXPECT_EQ(scanOne<u128>("+42", "{}"), ok(42_u128, 3));
  EXPECT_EQ(scanOne<u128>("0", "{}"), ok(0_u128, 1));
  EXPECT_EQ(
    scanOne<u128>("340282366920938463463374607431768211455", "{}"), ok(numeric_limits<u128>::max(), 39));
  EXPECT_EQ(
    scanError<u128>("340282366920938463463374607431768211456", "{}"),
    scn::scan_error::value_positive_overflow);

  // Errors
  EXPECT_EQ(scanError<u128>("-1", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<u128>("-0", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<u128>("", "{}"), scn::scan_error::end_of_input);
  EXPECT_EQ(scanError<u128>("abc", "{}"), scn::scan_error::invalid_scanned_value);

  // Types
  EXPECT_EQ(scanOne<u128>("42", "{:d}"), ok(42_u128, 2));
  EXPECT_EQ(scanOne<u128>("42", "{:u}"), ok(42_u128, 2));
  EXPECT_EQ(scanOne<u128>("0b101", "{:b}"), ok(5_u128, 5));
  EXPECT_EQ(scanOne<u128>("0o17", "{:o}"), ok(15_u128, 4));
  EXPECT_EQ(scanOne<u128>("0xff", "{:x}"), ok(255_u128, 4));
  EXPECT_EQ(scanOne<u128>("ffffffffffffffffffffffffffffffff", "{:x}"), ok(numeric_limits<u128>::max(), 32));
  EXPECT_EQ(
    scanError<u128>("100000000000000000000000000000000", "{:x}"), scn::scan_error::value_positive_overflow);
  EXPECT_EQ(scanOne<u128>("0x1f", "{:i}"), ok(31_u128, 4));

  // Fill, alignment, and width
  EXPECT_EQ(scanOne<u128>("***42", "{:*>}"), ok(42_u128, 5));
  EXPECT_EQ(scanOne<u128>("42***", "{:*<5}"), ok(42_u128, 5));
  EXPECT_EQ(scanOne<u128>("*42*", "{:*^}"), ok(42_u128, 4));
  EXPECT_EQ(scanError<u128>("42", "{:5}"), scn::scan_error::length_too_short);

  // Precision
  EXPECT_EQ(scanOne<u128>("12345", "{:.2}"), ok(12_u128, 2));

  // Localization
  EXPECT_EQ(scanOne<u128>(groupingLocale(), "1,234,567", "{:L}"), ok(1'234'567_u128, 9));
}

TEST(MODULE, rocketF32) {
  EXPECT_EQ(scanOne<f32>("3.5", "{}"), ok(3.5_f32, 3));
  EXPECT_EQ(scanOne<f32>("-1e3", "{}"), ok(-1000.0_f32, 4));
  EXPECT_EQ(scanOne<f32>("inf", "{}"), ok(numeric_limits<f32>::infinity(), 3));
  EXPECT_EQ(scanOne<f32>("-inf", "{}"), ok(-numeric_limits<f32>::infinity(), 4));

  const auto nan = scanOne<f32>("nan", "{}");
  ASSERT_TRUE(nan);
  EXPECT_TRUE(std::isnan(nan->first));
  EXPECT_EQ(nan->second, 3);
}

TEST(MODULE, rocketF64) {
  EXPECT_EQ(scanOne<f64>("0.25", "{}"), ok(0.25_f64, 4));
  EXPECT_EQ(scanOne<f64>("1.5", "{:f}"), ok(1.5_f64, 3));
  EXPECT_EQ(scanOne<f64>("2.5e-1", "{:e}"), ok(0.25_f64, 6));
  EXPECT_EQ(scanOne<f64>("0x1.8p1", "{:a}"), ok(3.0_f64, 7));
  EXPECT_EQ(scanError<f64>("abc", "{}"), scn::scan_error::invalid_scanned_value);
}

TEST(MODULE, rocketBimap) {
  using type = bimap<i32, f64>;

  const auto val1 = make_bimap<i32, f64>({ { 1, 1.11 }, { 2, 2.22 }, { 3, 3.33 } });
  const string input = fmt::format("{}", val1); // "{1: 1.11, 2: 2.22, 3: 3.33}", 27 chars
  const auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  const auto& val2 = result->value();
  EXPECT_EQ(fmt::format("{}", val2), input);
  EXPECT_EQ(result->begin() - input.begin(), 27);

  EXPECT_EQ(scanError<type>("{1: 1.11", "{}"), scn::scan_error::end_of_input);
  EXPECT_EQ(scanError<type>("{1 1.11}", "{}"), scn::scan_error::invalid_scanned_value);
}

TEST(MODULE, rocketUnorderedBimap) {
  using type = unordered_bimap<i32, f64>;

  const auto result = scn::scan<type>("{1: 1.5, 2: 2.5}", "{}");
  ASSERT_TRUE(result);
  const auto& val = result->value();
  EXPECT_EQ(val.size(), 2);
  EXPECT_EQ(val.left.at(1), 1.5);
  EXPECT_EQ(val.left.at(2), 2.5);
}

TEST(MODULE, rocketChronoZonedTime) {
  using namespace std::chrono;
  using rocket::chrono::zoned_seconds;
  using rocket::chrono::zoned_time;

  const auto* berlin = rocket::chrono::locate_zone("Europe/Berlin");
  const auto* newYork = rocket::chrono::locate_zone("America/New_York");
  const auto* utc = rocket::chrono::locate_zone("UTC");

  const sys_seconds summer = sys_days(2026y / October / 23d) + 10h;
  const sys_seconds winter = sys_days(2026y / December / 1d) + 10h;

  // Default format: `%F %T %Z`, with the name of the time zone
  EXPECT_EQ(
    scanOne<zoned_seconds>("2026-10-23 12:00:00 Europe/Berlin", "{}"), ok(zoned_seconds(berlin, summer), 33));
  EXPECT_EQ(
    scanOne<zoned_seconds>("2026-12-01 11:00:00 Europe/Berlin", "{}"), ok(zoned_seconds(berlin, winter), 33));
  EXPECT_EQ(
    scanOne<zoned_seconds>("2026-12-01 05:00:00 America/New_York", "{}"),
    ok(zoned_seconds(newYork, winter), 36));

  // The output of the formatter, if the abbreviation is also a name
  {
    const zoned_seconds val(utc, winter);
    const string input = fmt::format("{}", val); // "2026-12-01 10:00:00 UTC"
    EXPECT_EQ(scanOne<zoned_seconds>(input, "{}"), ok(val, 23));
  }

  // Subseconds: `%F %H:%M:%.S %Z`
  {
    using ZonedMs = zoned_time<milliseconds>;
    const ZonedMs val(berlin, sys_time<milliseconds>(summer + 123ms));
    EXPECT_EQ(scanOne<ZonedMs>("2026-10-23 12:00:00.123 Europe/Berlin", "{}"), ok(val, 37));
    EXPECT_EQ(scanOne<ZonedMs>(fmt::format("{:%F %T %Z}", ZonedMs(utc, val)), "{}"), ok(ZonedMs(utc, val), 27));
  }

  // Other specifications
  EXPECT_EQ(
    scanOne<zoned_seconds>("23.10.2026 12:00 Europe/Berlin", "{:%d.%m.%Y %H:%M %Z}"),
    ok(zoned_seconds(berlin, summer), 30));
  EXPECT_EQ(
    scanOne<zoned_seconds>("2026-10-23 12:00:00 +0200 Europe/Berlin", "{:%F %T %z %Z}"),
    ok(zoned_seconds(berlin, summer), 39));

  // An ambiguous local time, when DST ends, requires the UTC offset
  const sys_seconds fold = sys_days(2026y / October / 25d) + 0h + 30min;
  EXPECT_EQ(
    scanError<zoned_seconds>("2026-10-25 02:30:00 Europe/Berlin", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(
    scanOne<zoned_seconds>("2026-10-25 02:30:00 +0200 Europe/Berlin", "{:%F %T %z %Z}"),
    ok(zoned_seconds(berlin, fold), 39));
  EXPECT_EQ(
    scanOne<zoned_seconds>("2026-10-25 02:30:00 +0100 Europe/Berlin", "{:%F %T %z %Z}"),
    ok(zoned_seconds(berlin, fold + 1h), 39));
  EXPECT_EQ(
    scanError<zoned_seconds>("2026-10-25 02:30:00 +0300 Europe/Berlin", "{:%F %T %z %Z}"),
    scn::scan_error::invalid_scanned_value);

  // A nonexistent local time, when DST starts
  EXPECT_EQ(
    scanError<zoned_seconds>("2026-03-29 02:30:00 Europe/Berlin", "{}"), scn::scan_error::invalid_scanned_value);

  // Unknown time zones; abbreviations are not supported
  EXPECT_EQ(
    scanError<zoned_seconds>("2026-10-23 12:00:00 Mars/Olympus", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<zoned_seconds>("2026-10-23 12:00:00 CEST", "{}"), scn::scan_error::invalid_scanned_value);

  // Invalid dates and specifications
  EXPECT_EQ(
    scanError<zoned_seconds>("2026-02-30 12:00:00 Europe/Berlin", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(
    scanError<zoned_seconds>("12:00:00 Europe/Berlin", "{:%T %Z}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<zoned_seconds>("2026-10-23 12:00:00", "{:%F %T}"), scn::scan_error::invalid_format_string);
}

// `std` ----------------------------------------------------------------------------------------------------

TEST(MODULE, stdMap) {
  using type = map<int, double>;

  const auto val1 = type { { 1, 1.11 }, { 2, 2.22 }, { 3, 3.33 } };
  const string input = fmt::format("{}", val1); // "{1: 1.11, 2: 2.22, 3: 3.33}", 27 chars
  const auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  const auto val2 = result->value();
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(result->begin() - input.begin(), 27);
}

TEST(MODULE, stdPair) {
  using type = pair<bool, int>;

  const auto val1 = type { true, 12 };
  const string input = fmt::format("{}", val1); // "(true, 12)", 10 chars
  const auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  const auto val2 = result->value();
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(result->begin() - input.begin(), 10);
}

TEST(MODULE, stdSet) {
  using type = set<i32>;

  const auto val1 = type { 1, 2, 3 };
  const string input = fmt::format("{}", val1); // "{1, 2, 3}", 9 chars
  const auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  const auto val2 = result->value();
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(result->begin() - input.begin(), 9);
}

TEST(MODULE, stdString) {
  {
    const auto result = scn::scan<string, string>("[a    ][bbb  ]", "[{: <5}][{: <5}]");
    ASSERT_TRUE(result);
    const auto [val1, val2] = result->values();
    EXPECT_EQ(val1, "a");
    EXPECT_EQ(val2, "bbb");
  }

  // Up to the next whitespace
  EXPECT_EQ(scanOne<string>("  hello world", "{}"), ok("hello"s, 7));
  EXPECT_EQ(scanOne<string>("⊕⊕ x", "{}"), ok("⊕⊕"s, 6));
}

TEST(MODULE, stdTuple) {
  using type = tuple<bool, int, double>;

  const auto val1 = type { true, 1, 2.22 };
  const string input = fmt::format("{}", val1); // "(true, 1, 2.22)", 15 chars
  const auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  const auto val2 = result->value();
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(result->begin() - input.begin(), 15);
}

TEST(MODULE, stdUnorderedMap) {
  using type = unordered_map<i32, f64>;

  const auto result = scn::scan<type>("{1: 1.5, 2: 2.5}", "{}");
  ASSERT_TRUE(result);
  EXPECT_EQ(result->value(), (type { { 1, 1.5 }, { 2, 2.5 } }));
}

TEST(MODULE, stdUnorderedSet) {
  using type = unordered_set<i32>;

  const auto result = scn::scan<type>("{3, 1, 2}", "{}");
  ASSERT_TRUE(result);
  EXPECT_EQ(result->value(), (type { 1, 2, 3 }));
}

TEST(MODULE, stdVector) {
  using type = vector<i32>;

  const auto val1 = type { 1, 2, 3 };
  const string input = fmt::format("{}", val1); // "[1, 2, 3]", 9 chars
  const auto result = scn::scan<type>(input, "{}");
  ASSERT_TRUE(result);
  const auto val2 = result->value();
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(result->begin() - input.begin(), 9);
}

TEST(MODULE, stdChronoDuration) {
  using namespace std::chrono;

  // Default format: `%Q%q`, the output of the formatter
  EXPECT_EQ(scanOne<nanoseconds>("1ns", "{}"), ok(1ns, 3));
  EXPECT_EQ(scanOne<microseconds>("1µs", "{}"), ok(1us, 4));
  EXPECT_EQ(scanOne<milliseconds>("42ms", "{}"), ok(42ms, 4));
  EXPECT_EQ(scanOne<seconds>("1s", "{}"), ok(1s, 2));
  EXPECT_EQ(scanOne<minutes>("1min", "{}"), ok(1min, 4));
  EXPECT_EQ(scanOne<hours>("1h", "{}"), ok(1h, 2));
  EXPECT_EQ(scanOne<days>("1d", "{}"), ok(days(1), 2));
  EXPECT_EQ(scanOne<weeks>("1[604800]s", "{}"), ok(weeks(1), 10));
  EXPECT_EQ(scanOne<months>("1[2629746]s", "{}"), ok(months(1), 11));
  EXPECT_EQ(scanOne<years>("1[31556952]s", "{}"), ok(years(1), 12));
  for (const auto val : { -90s, 0s, 1s, 3600s, 86400s }) {
    const string input = fmt::format("{}", val);
    EXPECT_EQ(scanOne<seconds>(input, "{}"), ok(val, static_cast<i64>(input.size())));
  }

  // Leading whitespace, signs, and other units
  EXPECT_EQ(scanOne<seconds>("  42s", "{}"), ok(42s, 5));
  EXPECT_EQ(scanOne<seconds>("-5min", "{}"), ok(-300s, 5));
  EXPECT_EQ(scanOne<nanoseconds>("1us", "{}"), ok(1000ns, 3));
  EXPECT_EQ(scanOne<seconds>("1das", "{}"), ok(10s, 4));
  EXPECT_EQ(scanOne<hours>("1d", "{}"), ok(24h, 2));
  EXPECT_EQ(scanOne<seconds>("1[1/1]s", "{}"), ok(1s, 7));
  EXPECT_EQ(scanOne<milliseconds>("3[1/3]s", "{}"), ok(1000ms, 7));

  // The count is converted exactly; a floating-point count need not be
  EXPECT_EQ(scanError<seconds>("1500ms", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanOne<duration<f64>>("1500ms", "{}"), ok(duration<f64>(1.5), 6));
  using MillisecondsF64 = duration<f64, std::milli>;
  EXPECT_EQ(scanOne<MillisecondsF64>("2.5s", "{}"), ok(MillisecondsF64(2500.0), 4));

  // Overflow, of the count or of the conversion
  EXPECT_EQ(scanError<seconds>("9223372036854775808s", "{}"), scn::scan_error::value_positive_overflow);
  EXPECT_EQ(scanError<nanoseconds>("9223372036854775807s", "{}"), scn::scan_error::value_positive_overflow);
  EXPECT_EQ(scanError<nanoseconds>("-9223372036854775807s", "{}"), scn::scan_error::value_negative_overflow);
  EXPECT_EQ(scanOne<nanoseconds>("-9223372036854775808ns", "{}"), ok(nanoseconds::min(), 22));

  // Invalid units
  EXPECT_EQ(scanError<seconds>("42", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<seconds>("42x", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<seconds>("42[0]s", "{}"), scn::scan_error::invalid_scanned_value);

  // Other specifications
  EXPECT_EQ(scanOne<milliseconds>("42", "{:%Q}"), ok(42ms, 2));
  EXPECT_EQ(scanOne<milliseconds>("42 ms", "{:%Q %q}"), ok(42ms, 5));
  EXPECT_EQ(scanOne<milliseconds>("42ms", "{:%Q %q}"), ok(42ms, 4));
  EXPECT_EQ(scanOne<milliseconds>("42%", "{:%Q%%}"), ok(42ms, 3));
  EXPECT_EQ(scanOne<seconds>("t=42s", "{:t=%Q%q}"), ok(42s, 5));
  EXPECT_EQ(scanError<seconds>("x=42s", "{:t=%Q%q}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<seconds>("42s", "{:%Y}"), scn::scan_error::invalid_format_string);
  EXPECT_EQ(scanError<seconds>("42s", "{:%q}"), scn::scan_error::invalid_format_string);
  EXPECT_EQ(scanError<seconds>("42s", "{:%Q%Q}"), scn::scan_error::invalid_format_string);
}

// The scanner for `std::chrono::time_point` used to call `std::mktime`, which takes the fields for local
// time, although a `system_clock` time point is UTC. `cmake/patches/scnlib.patch` makes it take the fields
// as UTC. Under a time zone other than UTC, these tests fail without the patch
TEST(MODULE, stdChronoTimePoint) {
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

  // A coarser duration truncates
  EXPECT_EQ(
    scanOne<sys_seconds>("2026-07-15T12:34:56", "{:%FT%T}"), ok(sys_seconds(day + 12h + 34min + 56s), 19));

  // A UTC offset is applied
  EXPECT_EQ(scanOne<sys_seconds>("2026-07-15T12:00:00+0200", "{:%FT%T%z}"), ok(sys_seconds(day + 10h), 24));

  // Before the epoch
  EXPECT_EQ(scanOne<sys_seconds>("1969-12-31T23:59:59", "{:%FT%T}"), ok(sys_seconds(-1s), 19));

  // A day that does not exist in the month fails, instead of being moved into the next month
  EXPECT_FALSE(scanOne<sys_seconds>("2026-02-30T00:00:00", "{:%FT%T}"));
  EXPECT_FALSE(scanOne<sys_seconds>("2026-04-31T00:00:00", "{:%FT%T}"));
  EXPECT_FALSE(scanOne<sys_seconds>("2026-02-29T00:00:00", "{:%FT%T}")); // Not a leap year
  EXPECT_FALSE(scanOne<sys_seconds>("1900-02-29T00:00:00", "{:%FT%T}")); // Not a leap year
  EXPECT_EQ(
    scanOne<sys_seconds>("2000-02-29T00:00:00", "{:%FT%T}"),
    ok(sys_seconds(sys_days(2000y / February / 29)), 19));
}

TEST(MODULE, stdChronoYearMonthDay) {
  using namespace std::chrono;

  const year_month_day date { 2026y, October, 23d };

  EXPECT_EQ(scanOne<year_month_day>("2026-10-23", "{:%F}"), ok(date, 10));
  EXPECT_EQ(
    scanOne<year_month_day>("1970-01-01", "{:%Y-%m-%d}"), ok(year_month_day { 1970y, January, 1d }, 10));
  EXPECT_EQ(scanOne<year_month_day>("23.10.2026", "{:%d.%m.%Y}"), ok(date, 10));
  EXPECT_EQ(scanOne<year_month_day>("2026-10-23 rest", "{:%F}"), ok(date, 10));

  // There is no default format
  EXPECT_EQ(scanError<year_month_day>("2026-10-23", "{}"), scn::scan_error::invalid_format_string);
  EXPECT_EQ(scanError<year_month_day>("2026-13-01", "{:%F}"), scn::scan_error::invalid_scanned_value);
}

// EOF
