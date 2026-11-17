/*
 * test-scan.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/bimap-format.h"
#include "rocket/bimap-scan.h"
#include "rocket/enum-format.h"
#include "rocket/enum-scan.h"
#include "rocket/int128-format.h"
#include "rocket/int128-scan.h"
#include <rocket/chrono/chrono-format.h>
#include <rocket/chrono/chrono-scan.h>
#include "rocket/log/log.h"
#include "rocket/math/Interval-format.h"
#include "rocket/math/Interval-scan.h"
#include "rocket/nio/nio.h"
#include "rocket/reflect/reflect.h"
#include "rocket/reflect/reflect-format.h"
#include "rocket/reflect/reflect-scan.h"
#include "rocket/unicode/unicode-format.h"
#include "rocket/unicode/unicode-scan.h"
#include "rocket/scope.h"
#include "rocket/system/system.h"

#include <chrono>
#include <locale>
#include <optional>
#include <time.h> // NOLINT(modernize-deprecated-headers): for `tzset` and `_tzset`, missing in `<ctime>`
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
ok(const T& val, i64 length) {
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

  return { std::locale::classic(), new Numpunct() };
}

} // namespace

// Types ====================================================================================================

// `MyStruct` -----------------------------------------------------------------------------------------------

struct MyStruct {
  i32 ä = 0;
  string b;

  MyStruct() = default;

  MyStruct(i32 ä, string_view b, bool c) : ä(ä), b(b), c(c) {}

  [[nodiscard]] bool getC() const { return c; }

private:

  bool c = false;

public:

  ROCKET_REFLECT_MEMBERS(MyStruct, Index, (ä)(b)(c));

  ROCKET_REFLECT_MEMBERS(MyStruct, Public, (ä)(b));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyStruct, Index); // NOLINT(*-internal-linkage)
ROCKET_REFLECT_MEMBERS_DEFINE(, MyStruct, Index);

// `MyDerivedStruct` ----------------------------------------------------------------------------------------

struct MyDerivedStruct : MyStruct {
  string d;

  MyDerivedStruct() = default;

  MyDerivedStruct(i32 ä, string_view b, bool c, string_view d, f32 e) : MyStruct(ä, b, c), d(d), e(e) {}

private:

  f32 e = 0;

public:

  ROCKET_REFLECT_MEMBERS_DERIVED(MyStruct, Index, MyDerivedStruct, Index, (d)(e));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyDerivedStruct, Index); // NOLINT(*-internal-linkage)
ROCKET_REFLECT_MEMBERS_DEFINE(, MyDerivedStruct, Index);

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
  EXPECT_EQ(scanOne<u32>("1234abCD", "{:x}"), ok(0x1234ABCDU, 8));
  EXPECT_EQ(scanOne<u32>("abCDZZZZ", "{:x}"), ok(0xABCDU, 4));
  EXPECT_EQ(scanOne<u32>("12345678", "{:8x}"), ok(0x12345678U, 8));
  EXPECT_EQ(scanError<u32>("abCDZZZZ", "{:8x}"), scn::scan_error::length_too_short);

  {
    const auto input = "abCDeZZZ"sv;
    const string fmt = "{:x}";
    const auto result = scn::scan<u32>(input, scn::runtime_format(fmt));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 0xABCDEU);
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

TEST(MODULE, rocketEnum) {
  using log::LogLevel;

  EXPECT_EQ(scanOne<LogLevel>("none", "{}"), ok(LogLevel::none, 4));
  EXPECT_EQ(scanOne<LogLevel>("error", "{}"), ok(LogLevel::error, 5));
  EXPECT_EQ(scanOne<LogLevel>("warn", "{}"), ok(LogLevel::warn, 4));
  EXPECT_EQ(scanOne<LogLevel>("info", "{}"), ok(LogLevel::info, 4));
  EXPECT_EQ(scanOne<LogLevel>("debug", "{}"), ok(LogLevel::debug, 5));
  EXPECT_EQ(scanOne<LogLevel>("trace", "{}"), ok(LogLevel::trace, 5));

  // Leading whitespace is skipped, and scanning stops at the next whitespace
  EXPECT_EQ(scanOne<LogLevel>("  info debug", "{}"), ok(LogLevel::info, 6));

  // What follows the enum name within the word is given back
  EXPECT_EQ(scanOne<LogLevel>("warn,", "{}"), ok(LogLevel::warn, 4));
  EXPECT_EQ(scanOne<LogLevel>("debugger", "{}"), ok(LogLevel::debug, 5));

  {
    const auto result = scn::scan<LogLevel, LogLevel>("error, trace", "{}, {}");
    ASSERT_TRUE(result);
    const auto [val1, val2] = result->values();
    EXPECT_EQ(val1, LogLevel::error);
    EXPECT_EQ(val2, LogLevel::trace);
  }

  // Roundtrip
  for (const auto val : {
    LogLevel::none, LogLevel::error, LogLevel::warn, LogLevel::info, LogLevel::debug, LogLevel::trace }) {
    const string input = fmt::format("{}", val);
    EXPECT_EQ(scanOne<LogLevel>(input, "{}"), ok(val, static_cast<i64>(input.size())));
  }

  EXPECT_EQ(scanError<LogLevel>("", "{}"), scn::scan_error::end_of_input);
  EXPECT_EQ(scanError<LogLevel>("bogus", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<LogLevel>("INFO", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<LogLevel>(",info", "{}"), scn::scan_error::invalid_scanned_value);
}

TEST(MODULE, rocketEnumSource) {
  using log::LogLevel;

  // Positions: `debug` 0-4, `info` 6-9, `,` 10, `warn` 12-15, `trace` 17-21, `none` 23-26
  nio::StringSource in("debug info, warn\ttrace\nnone");
  EXPECT_EQ(in.tell(), 0);

  {
    const auto result = in.scan<LogLevel>("{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), LogLevel::debug);
    EXPECT_EQ(in.tell(), 5);
  }

  // The comma that follows `info` is given back to the source
  {
    const auto result = in.scan<LogLevel>("{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), LogLevel::info);
    EXPECT_EQ(in.tell(), 10);
  }

  {
    const auto result = in.scan<LogLevel>(",{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), LogLevel::warn);
    EXPECT_EQ(in.tell(), 16);
  }

  // A space in the format string matches any whitespace, including tabs and newlines
  {
    const auto result = in.scan<LogLevel, LogLevel>("{} {}");
    ASSERT_TRUE(result);
    const auto [val1, val2] = result->values();
    EXPECT_EQ(val1, LogLevel::trace);
    EXPECT_EQ(val2, LogLevel::none);
    EXPECT_EQ(in.tell(), 27);
  }

  {
    const auto result = in.scan<LogLevel>("{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::end_of_input);
    EXPECT_EQ(in.tell(), 27);
  }
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
    scanOne<zoned_seconds>("2026-10-23 12:00:00 Europe/Berlin", "{}"),
    ok(zoned_seconds(berlin, summer), 33));
  EXPECT_EQ(
    scanOne<zoned_seconds>("2026-12-01 11:00:00 Europe/Berlin", "{}"),
    ok(zoned_seconds(berlin, winter), 33));
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
    EXPECT_EQ(
      scanOne<ZonedMs>("2026-10-23 12:00:00.123 Europe/Berlin", "{}"),
      ok(val, 37));
    EXPECT_EQ(
      scanOne<ZonedMs>(fmt::format("{:%F %T %Z}", ZonedMs(utc, val)), "{}"),
      ok(ZonedMs(utc, val), 27));
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
    scanError<zoned_seconds>("2026-10-25 02:30:00 Europe/Berlin", "{}"),
    scn::scan_error::invalid_scanned_value);
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
    scanError<zoned_seconds>("2026-03-29 02:30:00 Europe/Berlin", "{}"),
    scn::scan_error::invalid_scanned_value);

  // Unknown time zones; abbreviations are not supported
  EXPECT_EQ(
    scanError<zoned_seconds>("2026-10-23 12:00:00 Mars/Olympus", "{}"),
    scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(
    scanError<zoned_seconds>("2026-10-23 12:00:00 CEST", "{}"),
    scn::scan_error::invalid_scanned_value);

  // Legacy names that look like abbreviations are rejected, too
  EXPECT_EQ(
    scanError<zoned_seconds>("2026-10-23 12:00:00 CET", "{}"),
    scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(
    scanError<zoned_seconds>("2026-10-23 12:00:00 EST", "{}"),
    scn::scan_error::invalid_scanned_value);
  EXPECT_TRUE(scanOne<zoned_seconds>("2026-10-23 12:00:00 GMT", "{}"));

  // Invalid dates and specifications
  EXPECT_EQ(
    scanError<zoned_seconds>("2026-02-30 12:00:00 Europe/Berlin", "{}"),
    scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(
    scanError<zoned_seconds>("12:00:00 Europe/Berlin", "{:%T %Z}"),
    scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(
    scanError<zoned_seconds>("2026-10-23 12:00:00", "{:%F %T}"),
    scn::scan_error::invalid_format_string);
}

TEST(MODULE, rocketMathClosedInterval) {
  using type = rocket::math::ClosedInterval<i32>;

  EXPECT_EQ(scanOne<type>("∅", "{}"), ok(type(), 3));
  EXPECT_EQ(scanOne<type>("[1,1]", "{}"), ok(type(1, 1), 5));
  EXPECT_EQ(scanOne<type>("[1,2]", "{}"), ok(type(1, 2), 5));
  EXPECT_EQ(scanOne<type>(" [ -1 , 2 ] ", "{}"), ok(type(-1, 2), 11));

  EXPECT_EQ(scanError<type>("", "{}"), scn::scan_error::end_of_input);
  EXPECT_EQ(scanError<type>("[1,2", "{}"), scn::scan_error::end_of_input);
  EXPECT_EQ(scanError<type>("[1,2)", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("(1,2]", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("[1 2]", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("[-∞,2]", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("{}", "{}"), scn::scan_error::invalid_scanned_value);
}

TEST(MODULE, rocketMathRightOpenIntervalI32) {
  using type = rocket::math::RightOpenInterval<i32>;

  EXPECT_EQ(scanOne<type>("∅", "{}"), ok(type(), 3));
  EXPECT_EQ(scanOne<type>("[1,2)", "{}"), ok(type(1, 2), 5));
  EXPECT_EQ(scanOne<type>("[1000,2000)", "{}"), ok(type(1'000, 2'000), 11));
  EXPECT_EQ(scanOne<type>("[5,∞)", "{}"), ok(type(5, nullopt), 7));

  EXPECT_EQ(scanError<type>("[-∞,5)", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("[1,2]", "{}"), scn::scan_error::invalid_scanned_value);
}

TEST(MODULE, rocketMathRightOpenIntervalF64) {
  using type = rocket::math::RightOpenInterval<f64>;
  constexpr auto inf = numeric_limits<f64>::infinity();

  EXPECT_EQ(scanOne<type>("∅", "{}"), ok(type(), 3));
  EXPECT_EQ(scanOne<type>("[1.5,2.5)", "{}"), ok(type(1.5, 2.5), 9));
  EXPECT_EQ(scanOne<type>("[-∞,∞)", "{}"), ok(type(-inf, nullopt), 10));
  EXPECT_EQ(scanOne<type>("[-1e3,inf)", "{}"), ok(type(-1'000.0, inf), 10));
}

TEST(MODULE, rocketMathIntervalRoundtrip) {
  using namespace rocket::math;
  constexpr auto inf = numeric_limits<f64>::infinity();

  const auto roundtrip = [](const auto& val) {
    using type = std::remove_cvref_t<decltype(val)>;
    const string input = fmt::format("{}", val);
    EXPECT_EQ(scanOne<type>(input, "{}"), ok(val, static_cast<i64>(input.size()))) << input;
  };

  roundtrip(ClosedInterval<i32>());
  roundtrip(ClosedInterval<i32>(-3, 7));
  roundtrip(ClosedInterval<u8>(1, 200));
  roundtrip(ClosedInterval<i64>(numeric_limits<i64>::min(), numeric_limits<i64>::max()));
  roundtrip(ClosedInterval<f64>(-inf, inf));
  roundtrip(ClosedInterval<f64>(-1.25, 3.4e100));
  roundtrip(ClosedInterval<f32>(0.1F, 0.2F));
  roundtrip(OpenInterval<i32>(nullopt, nullopt));
  roundtrip(OpenInterval<i32>(1, 5));
  roundtrip(OpenInterval<f64>(nullopt, 2.5));
  roundtrip(LeftOpenInterval<i16>(nullopt, -12));
  roundtrip(LeftOpenInterval<f64>(1e-9, 2));
  roundtrip(RightOpenInterval<f64>(12, nullopt));
  roundtrip(RightOpenInterval<u64>(0, numeric_limits<u64>::max()));
}

TEST(MODULE, rocketReflectDeclared) { // NOLINT(*-complexity)
  using type = MyStruct;
  const type val(42, "rocket", true);

  EXPECT_EQ(scanOne<type>("(ä=42, b=\"rocket\", c=true)", "{}"), ok(val, 27));
  EXPECT_EQ(scanOne<type>("MyStruct(ä=42, b=\"rocket\", c=true)", "{:t}"), ok(val, 35));
  EXPECT_EQ(scanOne<type>(" \n MyStruct \n ( c = true , ä = 42 , b = \"rocket\" ) ", "{:t}"), ok(val, 51));
  EXPECT_EQ(scanOne<type>("(c=true)", "{}"), ok(type(0, "", true), 8));
  EXPECT_EQ(scanOne<type>("()", "{}"), ok(type(), 2));

  // Roundtrips, also of indented output
  for (const char* fmt : { "{}", "{:i}" }) {
    const string input = fmt::format(fmt::runtime(fmt), val);
    EXPECT_EQ(scanOne<type>(input, "{}"), ok(val, static_cast<i64>(input.size()))) << input;
  }
  for (const char* fmt : { "{:t}", "{:it}" }) {
    const string input = fmt::format(fmt::runtime(fmt), val);
    EXPECT_EQ(scanOne<type>(input, "{:t}"), ok(val, static_cast<i64>(input.size()))) << input;
  }

  // Errors
  EXPECT_EQ(scanError<type>("", "{}"), scn::scan_error::end_of_input);
  EXPECT_EQ(scanError<type>("(ä=42", "{}"), scn::scan_error::end_of_input);
  EXPECT_EQ(scanError<type>("(x=42)", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("(ä=\"x\")", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("MyStruct(ä=42)", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("(ä=42)", "{:t}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("MyStructure(ä=42)", "{:t}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("MyStr", "{:t}"), scn::scan_error::end_of_input);
  EXPECT_EQ(scanError<type>("(ä=42)", "{:i}"), scn::scan_error::invalid_format_string);
}

TEST(MODULE, rocketReflectDeclaredDerived) {
  using type = MyDerivedStruct;
  const type val(41, "rocket", true, "everywhere", 4.2_f32);

  for (const char* fmt : { "{}", "{:i}" }) {
    const string input = fmt::format(fmt::runtime(fmt), val);
    EXPECT_EQ(scanOne<type>(input, "{}"), ok(val, static_cast<i64>(input.size()))) << input;
  }
  for (const char* fmt : { "{:t}", "{:it}" }) {
    const string input = fmt::format(fmt::runtime(fmt), val);
    EXPECT_EQ(scanOne<type>(input, "{:t}"), ok(val, static_cast<i64>(input.size()))) << input;
  }

  // The name must be that of the scanned type
  EXPECT_EQ(scanError<type>("MyStruct(ä=41)", "{:t}"), scn::scan_error::invalid_scanned_value);
}

TEST(MODULE, rocketReflectInstance) {
  using type = rocket::reflect::Instance<MyStruct, MyStruct::Public>;

  const auto plain = scanOne<type>("(ä=42, b=\"rocket\")", "{}");
  ASSERT_TRUE(plain);
  EXPECT_EQ(plain->first.get().ä, 42);
  EXPECT_EQ(plain->first.get().b, "rocket");
  EXPECT_FALSE(plain->first.get().getC());
  EXPECT_EQ(plain->second, 19);

  const auto named = scanOne<type>("MyStruct(ä=42, b=\"rocket\")", "{:t}");
  ASSERT_TRUE(named);
  EXPECT_EQ(named->first, plain->first);
  EXPECT_EQ(named->second, 27);

  // Only the public members are known
  EXPECT_EQ(scanError<type>("(c=true)", "{}"), scn::scan_error::invalid_scanned_value);

  const type val(MyStruct(43, "everywhere", false));
  for (const char* fmt : { "{:t}", "{:it}" }) {
    const string input = fmt::format(fmt::runtime(fmt), val);
    EXPECT_EQ(scanOne<type>(input, "{:t}"), ok(val, static_cast<i64>(input.size()))) << input;
  }
}

TEST(MODULE, rocketReflectSequence) {
  // The scanner consumes only the value, so further values can follow
  const auto result = scn::scan<MyStruct, i32>("MyStruct(ä=1) 2", "{:t} {}");
  ASSERT_TRUE(result);
  const auto& [val, i] = result->values();
  EXPECT_EQ(val, MyStruct(1, "", false));
  EXPECT_EQ(i, 2);
}

TEST(MODULE, rocketUnicodeCharacter) { // NOLINT(*-complexity)
  using namespace rocket::unicode;
  using type = Character<char>;

  // `Character` has no `operator==`, so its string is compared
  const auto scanChar = [](string_view input) -> optional<pair<string, i64>> {
    const auto result = scanOne<type>(input, "{}");
    if (not result) {
      return nullopt;
    }
    return pair<string, i64>(string(result->first), result->second);
  };
  const auto okChar = [](string_view str, i64 length) {
    return optional<pair<string, i64>>(pair<string, i64>(string(str), length));
  };

  EXPECT_EQ(scanChar("a"), okChar("a", 1));
  EXPECT_EQ(scanChar("€"), okChar("€", 3));
  EXPECT_EQ(scanChar("  ä "), okChar("ä", 4));
  EXPECT_EQ(scanChar("abc"), okChar("a", 1));
  EXPECT_EQ(scanChar("e\u0301x"), okChar("e\u0301", 3)); // e + COMBINING ACUTE ACCENT
  EXPECT_EQ(scanChar("👨‍👩‍👦!"), okChar("👨‍👩‍👦", 18)); // MAN, ZWJ, WOMAN, ZWJ, BOY
  EXPECT_EQ(scanChar(""), nullopt);
  EXPECT_EQ(scanChar("   "), nullopt);

  // Roundtrips
  for (const auto& c : { "a"_c, "€"_c, "e\u0301"_c, "👨‍👩‍👦"_c }) {
    const string input = fmt::format("{}", c);
    EXPECT_EQ(scanChar(input), okChar(input, static_cast<i64>(input.size()))) << input;
  }

  // Characters in sequence
  const auto result = scn::scan<type, type, i32>("ab 7", "{}{} {}");
  ASSERT_TRUE(result);
  const auto& [c1, c2, i] = result->values();
  EXPECT_EQ(string(c1), "a");
  EXPECT_EQ(string(c2), "b");
  EXPECT_EQ(i, 7);
}

TEST(MODULE, rocketUnicodeCodePoint) { // NOLINT(*-complexity)
  using namespace rocket::unicode;
  using type = CodePoint;

  EXPECT_EQ(scanOne<type>("U+0000", "{}"), ok('\0'_cp, 6));
  EXPECT_EQ(scanOne<type>("U+0041", "{}"), ok('A'_cp, 6));
  EXPECT_EQ(scanOne<type>("U+20AC", "{}"), ok(U'\u20AC'_cp, 6));
  EXPECT_EQ(scanOne<type>("U+20ac", "{}"), ok(U'\u20AC'_cp, 6));
  EXPECT_EQ(scanOne<type>("U+1ABCD", "{}"), ok(U'\U0001ABCD'_cp, 7));
  EXPECT_EQ(scanOne<type>("U+10FFFF", "{}"), ok(U'\U0010FFFF'_cp, 8));
  EXPECT_EQ(scanOne<type>("U+41", "{}"), ok('A'_cp, 4));
  EXPECT_EQ(scanOne<type>("  U+0041,", "{}"), ok('A'_cp, 8));

  // Roundtrips
  for (const auto cp : { '\0'_cp, 'z'_cp, U'\u00E4'_cp, U'\uFFFD'_cp, U'\U0001F600'_cp, U'\U0010FFFF'_cp }) {
    const string input = fmt::format("{}", cp);
    EXPECT_EQ(scanOne<type>(input, "{}"), ok(cp, static_cast<i64>(input.size()))) << input;
  }

  // Errors
  EXPECT_EQ(scanError<type>("", "{}"), scn::scan_error::end_of_input);
  EXPECT_EQ(scanError<type>("U+", "{}"), scn::scan_error::end_of_input);
  EXPECT_EQ(scanError<type>("U+G", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("u+0041", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("U0041", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("A", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("U+110000", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("U+D800", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("U+DFFF", "{}"), scn::scan_error::invalid_scanned_value);
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

TEST(MODULE, stdChronoDuration) { // NOLINT(*-complexity)
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

TEST(MODULE, stdChronoHhMmSs) { // NOLINT(*-complexity)
  using namespace std::chrono;

  // `hh_mm_ss` has no `operator==`, so its duration is compared
  const auto scanDuration = [](string_view input, auto type) -> optional<pair<decltype(type), i64>> {
    const auto result = scanOne<hh_mm_ss<decltype(type)>>(input, "{}");
    if (not result) {
      return nullopt;
    }
    return pair(result->first.to_duration(), result->second);
  };

  EXPECT_EQ(scanDuration("01:02:03", 0s), ok(1h + 2min + 3s, 8));
  EXPECT_EQ(scanDuration("  1:2:3", 0s), ok(1h + 2min + 3s, 7));
  EXPECT_EQ(scanDuration("01:02:03.005", 0ms), ok(1h + 2min + 3s + 5ms, 12));
  EXPECT_EQ(scanDuration("-111:02:03.123456789", 0ns), ok(-(111h + 2min + 3s + 123456789ns), 20));
  EXPECT_EQ(scanDuration("-00:02:00", 0s), ok(seconds(-2min), 9));
  EXPECT_EQ(scanDuration("2562046:59:59", 0s), ok(2562046h + 59min + 59s, 13));
  EXPECT_EQ(scanDuration("01:02:03,", 0s), ok(1h + 2min + 3s, 8));

  // Subseconds are truncated to the precision, and digits beyond nanoseconds are ignored
  EXPECT_EQ(scanDuration("01:02:03.999", 0s), ok(1h + 2min + 3s, 12));
  EXPECT_EQ(scanDuration("-01:02:03.999", 0ms), ok(-(1h + 2min + 3s + 999ms), 13));
  EXPECT_EQ(scanDuration("00:00:00.1234567891", 0ns), ok(123456789ns, 19));

  // Roundtrips
  const auto vals = initializer_list<nanoseconds> {
    0ns, 1h + 2min + 3s + 5ms, -(111h + 2min + 3s + 123456789ns), 2562046h + 1ns
  };
  for (const nanoseconds val : vals) {
    const string input = fmt::format("{}", hh_mm_ss { val });
    EXPECT_EQ(scanDuration(input, 0ns), ok(val, static_cast<i64>(input.size()))) << input;
  }

  // Errors
  using type = hh_mm_ss<seconds>;
  EXPECT_EQ(scanError<type>("", "{}"), scn::scan_error::end_of_input);
  EXPECT_EQ(scanError<type>("x", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("01:02", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("01:02:", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("01:02:03.", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("01:60:00", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("01:00:60", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("01:-1:00", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("2562047:00:00", "{}"), scn::scan_error::invalid_scanned_value);
  EXPECT_EQ(scanError<type>("--01:00:00", "{}"), scn::scan_error::invalid_scanned_value);
}

// The scanner for `std::chrono::time_point` used to call `std::mktime`, which takes the fields for local
// time, although a `system_clock` time point is UTC. `cmake/patches/scnlib.patch` makes it take the fields
// as UTC. Under a time zone other than UTC, these tests fail without the patch
TEST(MODULE, stdChronoTimePoint) { // NOLINT(*-complexity)
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
