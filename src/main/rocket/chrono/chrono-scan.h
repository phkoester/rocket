/**
 * @file chrono-scan.h
 *
 * scnlib scanners for chrono types.
 */

#pragma once

#include "rocket/type-traits.h"
#include "rocket/chrono/chrono.h"

#include <scn/chrono.h>

#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <ratio>
#include <string>
#include <string_view>
#include <type_traits>

namespace rocket::chrono::internal::scan {

// Utilities ------------------------------------------------------------------------------------------------

/// Whether a code unit is whitespace in the classic locale.
template<typename C>
constexpr bool
isSpace(C c) {
  return c == ' ' || (c >= '\t' && c <= '\r');
}

/// Encodes text of code points below U+10000 as code units of `C`.
template<typename C>
std::basic_string<C>
encode(std::u32string_view str) {
  std::basic_string<C> result;
  for (const char32_t cp : str) {
    if constexpr (sizeof(C) == 1) {
      if (cp < 0x80) {
        result.push_back(static_cast<C>(cp));
      } else if (cp < 0x800) {
        result.push_back(static_cast<C>(0xc0 | (cp >> 6)));
        result.push_back(static_cast<C>(0x80 | (cp & 0x3f)));
      } else {
        result.push_back(static_cast<C>(0xe0 | (cp >> 12)));
        result.push_back(static_cast<C>(0x80 | ((cp >> 6) & 0x3f)));
        result.push_back(static_cast<C>(0x80 | (cp & 0x3f)));
      }
    } else {
      result.push_back(static_cast<C>(cp));
    }
  }
  return result;
}

/// Matches `str` at `it`, advancing `it` on success.
template<typename It, typename Sent, typename C>
bool
match(It& it, Sent end, std::basic_string_view<C> str) {
  auto p = it;
  for (const C c : str) {
    if (p == end || *p != c) {
      return false;
    }
    ++p;
  }
  it = p;
  return true;
}

inline scn::unexpected<scn::scan_error>
invalidValue(const char* msg) {
  return scn::unexpected(scn::scan_error(scn::scan_error::invalid_scanned_value, msg));
}

/// A unit suffix of {fmt}, with its period in seconds.
struct Unit {
  std::u32string_view suffix;
  std::intmax_t num;
  std::intmax_t den;
};

/// The unit suffixes {fmt} writes for durations, plus `us` for microseconds.
inline constexpr auto units = std::to_array<Unit>({
  { U"as", 1, std::atto::den },
  { U"fs", 1, std::femto::den },
  { U"ps", 1, std::pico::den },
  { U"ns", 1, std::nano::den },
  { U"µs", 1, std::micro::den },
  { U"us", 1, std::micro::den },
  { U"ms", 1, std::milli::den },
  { U"cs", 1, std::centi::den },
  { U"ds", 1, std::deci::den },
  { U"s", 1, 1 },
  { U"das", std::deca::num, 1 },
  { U"hs", std::hecto::num, 1 },
  { U"ks", std::kilo::num, 1 },
  { U"Ms", std::mega::num, 1 },
  { U"Gs", std::giga::num, 1 },
  { U"Ts", std::tera::num, 1 },
  { U"Ps", std::peta::num, 1 },
  { U"Es", std::exa::num, 1 },
  { U"min", 60, 1 },
  { U"h", 3600, 1 },
  { U"d", 86400, 1 },
});

/// Reads a positive number for the unit `[num]s` or `[num/den]s`.
template<typename It, typename Sent>
bool
readUnitNumber(It& it, Sent end, std::intmax_t& val) {
  if (it == end || *it < '0' || *it > '9') {
    return false;
  }
  // The first character is a digit, as checked above
  val = 0;
  while (it != end && *it >= '0' && *it <= '9') {
    const auto digit = static_cast<std::intmax_t>(*it - '0');
    if (val > (std::numeric_limits<std::intmax_t>::max() - digit) / 10) {
      return false;
    }
    val = val * 10 + digit;
    ++it;
  }
  return val != 0;
}

/**
 * Reads a unit suffix as {fmt} writes it, for example `ms` or `[604800]s`, and returns its period in
 * seconds.
 */
template<typename C, typename Context>
scn::scan_expected<void>
readUnit(Context& ctx, std::intmax_t& num, std::intmax_t& den) {
  auto it = ctx.begin();
  const auto end = ctx.end();

  if (it != end && *it == '[') {
    ++it;
    num = 0;
    den = 1;
    if (not readUnitNumber(it, end, num)) {
      return invalidValue("Invalid duration unit");
    }
    if (it != end && *it == '/') {
      ++it;
      if (not readUnitNumber(it, end, den)) {
        return invalidValue("Invalid duration unit");
      }
    }
    static constexpr auto close = std::to_array<C>({ ']', 's' });
    if (not match(it, end, std::basic_string_view<C>(close.data(), close.size()))) {
      return invalidValue("Invalid duration unit");
    }
    ctx.advance_to(it);
    return {};
  }

  // The longest suffix that matches, so that `das` is not read as `d`
  const Unit* best = nullptr;
  auto bestIt = it;
  u64 bestSize = 0;
  for (const Unit& unit : units) {
    const std::basic_string<C> suffix = encode<C>(unit.suffix);
    auto p = it;
    if (suffix.size() > bestSize && match(p, end, std::basic_string_view<C>(suffix))) {
      best = &unit;
      bestIt = p;
      bestSize = suffix.size();
    }
  }
  if (best == nullptr) {
    return invalidValue("Invalid duration unit");
  }
  num = best->num;
  den = best->den;
  ctx.advance_to(bestIt);
  return {};
}

/**
 * Converts a count of `num/den` seconds exactly to a count of `Period`.
 *
 * Floating-point counts are converted arithmetically. Integral counts fail if the result is not an integer
 * or does not fit into `Rep`.
 */
template<typename Rep, typename Period>
scn::scan_expected<Rep>
convertCount(Rep count, std::intmax_t num, std::intmax_t den) {
  if constexpr (std::floating_point<Rep>) {
    const long double factor = static_cast<long double>(num) * static_cast<long double>(Period::den) /
      (static_cast<long double>(den) * static_cast<long double>(Period::num));
    return static_cast<Rep>(static_cast<long double>(count) * factor);
  } else {
    // The factor is `n/d`. Both fit into `u128`, as `num`, `den`, and the parts of `Period` fit into
    // `intmax_t`
    u128 n = static_cast<u128>(static_cast<std::uintmax_t>(num)) * static_cast<u128>(Period::den);
    u128 d = static_cast<u128>(static_cast<std::uintmax_t>(den)) * static_cast<u128>(Period::num);
    u128 a = n;
    u128 b = d;
    while (b != static_cast<u128>(0)) {
      const u128 t = a % b;
      a = b;
      b = t;
    }
    n /= a;
    d /= a;

    bool negative = false;
    u128 magnitude {};
    if constexpr (std::is_signed_v<Rep>) {
      negative = count < 0;
      magnitude =
        negative ? static_cast<u128>(-(count + 1)) + static_cast<u128>(1) : static_cast<u128>(count);
    } else {
      magnitude = static_cast<u128>(count);
    }

    const auto overflow = [negative] {
      return scn::unexpected(scn::scan_error(
        negative ? scn::scan_error::value_negative_overflow : scn::scan_error::value_positive_overflow,
        "Duration overflow"));
    };
    if (magnitude != static_cast<u128>(0) && magnitude > std::numeric_limits<u128>::max() / n) {
      return overflow();
    }
    const u128 product = magnitude * n;
    if (product % d != static_cast<u128>(0)) {
      return invalidValue("Duration not representable in the target type");
    }
    const u128 result = product / d;

    u128 limit {};
    if (negative) {
      const Rep min = std::numeric_limits<Rep>::min();
      limit = static_cast<u128>(-(min + 1)) + static_cast<u128>(1);
    } else {
      limit = static_cast<u128>(std::numeric_limits<Rep>::max());
    }
    if (result > limit) {
      return overflow();
    }
    return negative ? static_cast<Rep>(static_cast<u128>(0) - result) : static_cast<Rep>(result);
  }
}

} // namespace rocket::chrono::internal::scan

// `scn::scanner<std::chrono::duration>` --------------------------------------------------------------------

/**
 * Scans a #std::chrono::duration.
 *
 * scnlib does not provide this scanner. The format specification consists of the conversions `%Q` (the
 * count, required exactly once), `%q` (the unit suffix), `%%` (a `%`), `%n` (one whitespace character), and
 * `%t` (an optional whitespace character). Other whitespace matches any amount of whitespace, and other
 * characters match themselves. Without a format specification, the format is `%Q%q`, so the output of {fmt},
 * such as `42ms`, `1min`, or `1[604800]s`, can be scanned.
 *
 * The count is scanned like a value of the type `Rep`. The unit suffix is any that {fmt} writes, plus `us`
 * for microseconds; the count is converted to `Period`, failing if it is not representable exactly (unless
 * `Rep` is a floating-point type) or overflows. Without `%q`, the count is in units of `Period`.
 *
 * @tparam Rep the representation of the count
 * @tparam Period the period of the duration
 * @tparam C the character type
 */
template<typename Rep, typename Period, typename C>
  requires rocket::IsChar<C> && (std::integral<Rep> || std::floating_point<Rep>)
struct scn::scanner<std::chrono::duration<Rep, Period>, C> {
  /// @cond undocumented

  template<typename ParseCtx>
  constexpr ParseCtx::iterator
  parse(ParseCtx& pctx) {
    const auto begin = pctx.begin();
    auto it = begin;
    while (it != pctx.end() && *it != '}') {
      ++it;
    }
    spec_ = std::basic_string_view<C>(scn::detail::to_address(begin), static_cast<rocket::u64>(it - begin));

    int counts = 0;
    for (rocket::u64 i = 0; i < spec_.size(); ++i) {
      if (spec_[i] != '%') {
        continue;
      }
      if (i + 1 == spec_.size()) {
        pctx.on_error("Unexpected end of chrono format string");
        return it;
      }
      const C next = spec_[++i];
      if (next == 'Q') {
        ++counts;
      } else if (next != 'q' && next != '%' && next != 'n' && next != 't') {
        pctx.on_error("Invalid format specifier for a duration");
        return it;
      }
    }
    if (not spec_.empty() && counts != 1) {
      pctx.on_error("A duration format string requires `%Q` exactly once");
    }
    return it;
  }

  template<typename Context>
  scn::scan_expected<typename Context::iterator>
  scan(std::chrono::duration<Rep, Period>& val, Context& ctx) const { // NOLINT(*-complexity)
    using namespace rocket::chrono::internal::scan;

    static constexpr auto defaultSpec = std::to_array<C>({ '%', 'Q', '%', 'q' });
    const std::basic_string_view<C> spec =
      spec_.empty() ? std::basic_string_view<C>(defaultSpec.data(), defaultSpec.size()) : spec_;

    Rep count {};
    bool hasUnit = false;
    std::intmax_t num = 1;
    std::intmax_t den = 1;

    for (rocket::u64 i = 0; i < spec.size(); ++i) {
      const C c = spec[i];
      auto it = ctx.begin();
      const auto end = ctx.end();

      if (c == '%') {
        const C next = spec[++i];
        if (next == 'Q') {
          const scn::scanner<Rep, C> scanner {};
          auto result = scanner.scan(count, ctx);
          if (not result) {
            return scn::unexpected(result.error());
          }
          ctx.advance_to(*result);
        } else if (next == 'q') {
          if (auto result = readUnit<C>(ctx, num, den); not result) {
            return scn::unexpected(result.error());
          }
          hasUnit = true;
        } else if (next == '%') {
          if (it == end || *it != '%') {
            return invalidValue("Expected `%`");
          }
          ctx.advance_to(++it);
        } else if (next == 'n') {
          if (it == end || not isSpace(*it)) {
            return invalidValue("Expected whitespace");
          }
          ctx.advance_to(++it);
        } else if (it != end && isSpace(*it)) { // `%t`
          ctx.advance_to(++it);
        }
      } else if (isSpace(c)) {
        while (it != end && isSpace(*it)) {
          ++it;
        }
        ctx.advance_to(it);
      } else {
        if (it == end || *it != c) {
          return invalidValue("Literal character in the format string does not match");
        }
        ctx.advance_to(++it);
      }
    }

    if (hasUnit) {
      auto converted = convertCount<Rep, Period>(count, num, den);
      if (not converted) {
        return scn::unexpected(converted.error());
      }
      count = *converted;
    }
    val = std::chrono::duration<Rep, Period>(count);
    return ctx.begin();
  }

  /// @endcond

private:

  std::basic_string_view<C> spec_;
};

// `scn::scanner<rocket::chrono::zoned_time>` ---------------------------------------------------------------

namespace rocket::chrono::internal::scan {

/**
 * Splits a chrono format specification into segments, calling `f(segment, isOffset)` for each nonempty one.
 *
 * The UTC offsets `%z`, `%Ez`, and `%Oz`, with the literal text after them, form segments of their own,
 * because scnlib does not accept them together with the time-zone name `%Z` in one specification.
 */
template<typename C, typename F>
constexpr void
forEachSegment(std::basic_string_view<C> spec, F f) {
  u64 begin = 0;
  u64 i = 0;
  while (i < spec.size()) {
    if (spec[i] != '%' || i + 1 == spec.size()) {
      ++i;
      continue;
    }
    const C next = spec[i + 1];
    u64 length = 0;
    if (next == 'z') {
      length = 2;
    } else if ((next == 'E' || next == 'O') && i + 2 < spec.size() && spec[i + 2] == 'z') {
      length = 3;
    }
    if (length == 0) {
      i += 2;
      continue;
    }
    if (i > begin) {
      f(spec.substr(begin, i - begin), false);
    }
    // The offset takes the literal text up to the next conversion, as scnlib requires each specification to
    // start with a conversion
    u64 end = i + length;
    while (end < spec.size() && spec[end] != '%') {
      ++end;
    }
    f(spec.substr(i, end - i), true);
    i = end;
    begin = i;
  }
  if (begin < spec.size()) {
    f(spec.substr(begin), false);
  }
}

/// Copies the fields that are set in `src` to `dst`.
inline void
merge(scn::datetime_components& dst, const scn::datetime_components& src) {
  const auto copy = [](auto& to, const auto& from) {
    if (from) {
      to = from;
    }
  };
  copy(dst.subsec, src.subsec);
  copy(dst.sec, src.sec);
  copy(dst.min, src.min);
  copy(dst.hour, src.hour);
  copy(dst.mday, src.mday);
  copy(dst.mon, src.mon);
  copy(dst.year, src.year);
  copy(dst.wday, src.wday);
  copy(dst.yday, src.yday);
  copy(dst.tz_offset, src.tz_offset);
  copy(dst.tz_name, src.tz_name);
}

} // namespace rocket::chrono::internal::scan

/**
 * Scans a #rocket::chrono::zoned_time.
 *
 * scnlib does not provide this scanner. The format specification is that of scnlib for date and time, which
 * must contain a date and `%Z`. Unlike when formatting, `%Z` is the name of the time zone in the time-zone
 * database, such as `Europe/Berlin` or `UTC`, not its abbreviation; abbreviations such as `CST` are
 * ambiguous. Legacy names that look like abbreviations, such as `CET` or `EST`, are rejected, see
 * #rocket::chrono::isUnambiguousZoneName. The date and time are the local time in that time zone. Unlike in
 * scnlib, `%Z` may be combined with the UTC offset `%z` (or `%Ez` or `%Oz`), which then selects between the
 * two meanings of an ambiguous local time, and must match the time zone; without it, an ambiguous or
 * nonexistent local time fails.
 *
 * Without a format specification, the format is `%F %T %Z`, or `%F %H:%M:%.S %Z` for a duration finer than
 * seconds, so the output of the formatter with the default format can be scanned if it uses a time-zone
 * name.
 *
 * @tparam Duration the duration of the time point
 * @tparam TimeZonePtr the time-zone pointer
 * @tparam C the character type
 */
template<typename Duration, typename TimeZonePtr, typename C>
  requires rocket::IsChar<C> && requires(std::string_view name) {
    { rocket::chrono::zoned_traits<TimeZonePtr>::locate_zone(name) } -> std::convertible_to<TimeZonePtr>;
  }
class scn::scanner<rocket::chrono::zoned_time<Duration, TimeZonePtr>, C> {
  using ZonedTime = rocket::chrono::zoned_time<Duration, TimeZonePtr>;

public:

  /// @cond undocumented

  template<typename ParseCtx>
  constexpr ParseCtx::iterator
  parse(ParseCtx& pctx) {
    using rocket::chrono::internal::scan::forEachSegment;

    const auto begin = pctx.begin();
    auto it = begin;
    while (it != pctx.end() && *it != '}') {
      ++it;
    }
    if (it == begin) {
      if constexpr (std::ratio_less_v<typename ZonedTime::duration::period, std::ratio<1>>) {
        spec_ = std::basic_string_view<C>(subsecondSpec.data(), subsecondSpec.size());
      } else {
        spec_ = std::basic_string_view<C>(defaultSpec.data(), defaultSpec.size());
      }
      return it;
    }
    spec_ = std::basic_string_view<C>(scn::detail::to_address(begin), static_cast<rocket::u64>(it - begin));

    // Check the segments as scnlib checks a specification
    bool hasZone = false;
    int offsets = 0;
    const char* error = nullptr;
    forEachSegment(spec_, [&](std::basic_string_view<C> segment, bool isOffset) {
      if (isOffset) {
        ++offsets;
        return;
      }
      auto checker = scn::detail::tm_format_checker<scn::datetime_components> {};
      scn::detail::parse_chrono_format_specs(segment.data(), segment.data() + segment.size(), checker);
      if (const auto result = checker.get_error(); not result && error == nullptr) {
        error = result.error().msg();
      }
      for (rocket::u64 i = 0; i + 1 < segment.size(); ++i) {
        if (segment[i] == '%') {
          hasZone = hasZone || segment[i + 1] == 'Z';
          ++i;
        }
      }
    });
    if (error != nullptr) {
      pctx.on_error(error);
    } else if (offsets > 1) {
      pctx.on_error("A zoned time allows only one UTC offset (`%z`)");
    } else if (not hasZone) {
      pctx.on_error("A zoned time requires the time-zone name `%Z`");
    }
    return it;
  }

  template<typename Context>
  scn::scan_expected<typename Context::iterator>
  scan(ZonedTime& val, Context& ctx) const { // NOLINT(*-complexity)
    using namespace std::chrono;
    using rocket::chrono::internal::scan::forEachSegment;
    using rocket::chrono::internal::scan::invalidValue;
    using rocket::chrono::internal::scan::merge;
    using D = ZonedTime::duration;

    // Scan the segments with scnlib
    scn::datetime_components dt {};
    scn::scan_expected<void> scanned;
    forEachSegment(spec_, [&](std::basic_string_view<C> segment, bool) {
      if (not scanned) {
        return;
      }
      scn::datetime_components part {};
      auto result = scn::detail::chrono_scan_impl(segment, part, ctx);
      if (not result) {
        scanned = scn::unexpected(result.error());
        return;
      }
      ctx.advance_to(*result);
      merge(dt, part);
    });
    if (not scanned) {
      return scn::unexpected(scanned.error());
    }

    // Date
    if (not dt.year || not dt.mon || not dt.mday) {
      return invalidValue("A zoned time requires a date");
    }
    // Use `std::chrono::year_month_day` and siblings to avoid ambiguity with scnlib’s
    const std::chrono::year_month_day ymd(
      std::chrono::year(*dt.year),
      std::chrono::month(static_cast<unsigned>(*dt.mon)),
      std::chrono::day(static_cast<unsigned>(*dt.mday)));
    if (not ymd.ok()) {
      return invalidValue("Invalid date");
    }
    const std::chrono::sys_days days(ymd);
    if (dt.wday && std::chrono::weekday(dt.wday->c_encoding()) != std::chrono::weekday(days)) {
      return invalidValue("The weekday does not match the date");
    }
    if (dt.yday && *dt.yday !=
       (days - std::chrono::sys_days(ymd.year() / std::chrono::January / 1)).count()) {
      return invalidValue("The day of the year does not match the date");
    }

    // Local time
    local_time<D> lt = local_days(days.time_since_epoch()) + hours(dt.hour.value_or(0)) +
      minutes(dt.min.value_or(0)) + seconds(dt.sec.value_or(0));
    if (dt.subsec) {
      lt += duration_cast<D>(nanoseconds(std::llround(*dt.subsec * static_cast<double>(std::nano::den))));
    }

    // Time zone. scnlib reads an empty name if no name character follows
    if (not dt.tz_name || dt.tz_name->empty()) {
      return invalidValue("Expected a time-zone name");
    }
    TimeZonePtr zone;
    try {
      zone = rocket::chrono::zoned_traits<TimeZonePtr>::locate_zone(*dt.tz_name);
    } catch (const std::exception&) {
      return invalidValue("Unknown time zone");
    }
    if (not rocket::chrono::isUnambiguousZoneName(*dt.tz_name)) {
      return invalidValue("Ambiguous time-zone name");
    }

    // UTC offset
    const auto info = zone->get_info(lt);
    using Info = std::remove_cvref_t<decltype(info)>;
    seconds offset;
    if (dt.tz_offset) {
      offset = duration_cast<seconds>(*dt.tz_offset);
      const bool matches = (info.result != Info::nonexistent && info.first.offset == offset) ||
        (info.result == Info::ambiguous && info.second.offset == offset);
      if (not matches) {
        return invalidValue("The UTC offset does not match the time zone");
      }
    } else if (info.result == Info::nonexistent) {
      return invalidValue("The local time does not exist in the time zone");
    } else if (info.result == Info::ambiguous) {
      return invalidValue("The local time is ambiguous in the time zone");
    } else {
      offset = info.first.offset;
    }

    const sys_time<D> st(lt.time_since_epoch() - offset);
    if constexpr (std::is_same_v<D, Duration>) {
      val = ZonedTime(zone, st);
    } else {
      val = ZonedTime(zone, floor<Duration>(st));
    }
    return ctx.begin();
  }

  /// @endcond

private:

  static constexpr auto defaultSpec = std::to_array<C>({ '%', 'F', ' ', '%', 'T', ' ', '%', 'Z' });
  static constexpr auto subsecondSpec = std::to_array<C>({
    '%', 'F', ' ', '%', 'H', ':', '%', 'M', ':', '%', '.', 'S', ' ', '%', 'Z'
  });

  std::basic_string_view<C> spec_;
};

// `scn::scanner<std::chrono::hh_mm_ss>` --------------------------------------------------------------------

namespace rocket::chrono::internal::scan {

/// Reads decimal digits into `val`, failing if there are none or if `val` would exceed `max`.
template<typename It, typename Sent>
scn::scan_expected<void>
readDigits(It& it, Sent end, std::int64_t max, std::int64_t& val) {
  if (it == end || *it < '0' || *it > '9') {
    return invalidValue("Expected hour, minute, and second");
  }
  // The first character is a digit, as checked above
  val = 0;
  while (it != end && *it >= '0' && *it <= '9') {
    val = val * 10 + static_cast<std::int64_t>(*it - '0');
    if (val > max) {
      return invalidValue("Invalid hour, minute, or second");
    }
    ++it;
  }
  return {};
}

} // namespace rocket::chrono::internal::scan

/**
 * Scans a #std::chrono::hh_mm_ss.
 *
 * scnlib does not provide this scanner. It reads what the formatter in `rocket/chrono/chrono-format.h`
 * writes: `[-]H:M:S`, optionally followed by a dot and subseconds, e.g., `01:02:03`, `01:02:03.005`, or
 * `-111:02:03.123456789`. Whitespace before the value is skipped.
 *
 * The hours are not limited to 0 through 23, but the whole value must fit into #std::chrono::nanoseconds.
 * The minutes and seconds must be less than 60. Subseconds finer than nanoseconds are ignored, and the
 * value is truncated toward zero to the precision of @p Duration.
 *
 * - No format specifiers are accepted.
 *
 * @tparam Duration the duration of the #std::chrono::hh_mm_ss
 * @tparam C the character type
 */
template<typename Duration, typename C> requires rocket::IsChar<C>
struct scn::scanner<std::chrono::hh_mm_ss<Duration>, C> {
  /// @cond undocumented

  template<typename ParseCtx>
  constexpr ParseCtx::iterator
  parse(ParseCtx& pctx) {
    return pctx.begin();
  }

  template<typename Context>
  scn::scan_expected<typename Context::iterator>
  scan(std::chrono::hh_mm_ss<Duration>& val, Context& ctx) const {
    using namespace std::chrono;
    using namespace rocket::chrono::internal::scan;

    auto skipped = scn::detail::internal_skip_classic_whitespace(ctx.range(), false);
    if (not skipped) {
      return scn::unexpected(skipped.error());
    }
    auto it = *skipped;
    const auto end = ctx.end();

    const bool negative = it != end && *it == '-';
    if (negative) {
      ++it;
    }

    // Hours, so that the whole value fits into nanoseconds
    constexpr std::int64_t maxHours = duration_cast<hours>(nanoseconds::max()).count() - 1;
    std::int64_t h = 0;
    std::int64_t m = 0;
    std::int64_t s = 0;
    if (auto result = readDigits(it, end, maxHours, h); not result) {
      return scn::unexpected(result.error());
    }
    for (std::int64_t* field : { &m, &s }) {
      if (it == end || *it != ':') {
        return invalidValue("Expected hour, minute, and second");
      }
      ++it;
      if (auto result = readDigits(it, end, 59, *field); not result) {
        return scn::unexpected(result.error());
      }
    }

    // Subseconds, as nanoseconds
    std::int64_t ns = 0;
    if (it != end && *it == '.') {
      ++it;
      if (it == end || *it < '0' || *it > '9') {
        return invalidValue("Expected subseconds");
      }
      int digits = 0;
      for (; it != end && *it >= '0' && *it <= '9'; ++it) {
        if (digits < 9) {
          ns = ns * 10 + static_cast<std::int64_t>(*it - '0');
          ++digits;
        }
      }
      for (; digits < 9; ++digits) {
        ns *= 10;
      }
    }

    auto duration = duration_cast<Duration>(hours(h) + minutes(m) + seconds(s) + nanoseconds(ns));
    if (negative) {
      duration = -duration;
    }
    val = std::chrono::hh_mm_ss<Duration>(duration);
    return it;
  }

  /// @endcond
};

// `scn::scanner<std::chrono::year_month_day>` --------------------------------------------------------------

/**
 * Scans a #std::chrono::year_month_day with scnlib.
 *
 * scnlib only provides this scanner if `__cpp_lib_chrono >= 201907L`; otherwise it scans its own
 * `scn::year_month_day`. This is a copy of scnlib’s scanner from `scn/chrono.h`. Like there, there is no
 * default format, so a format specification such as `%F` is required.
 *
 * If scnlib provides its own scanner, the two do not collide: both are partial specializations for the same
 * arguments, but this one is constrained by #rocket::IsChar, so partial ordering makes it the more
 * specialized one, and it is always picked.
 *
 * @tparam C the character type
 */
template<typename C> requires rocket::IsChar<C>
struct scn::scanner<std::chrono::year_month_day, C> : scn::detail::chrono_component_scanner<C> {
  /// @cond undocumented

  template<typename Context>
  scn::scan_expected<typename Context::iterator>
  scan(std::chrono::year_month_day& val, Context& ctx) const {
    scn::datetime_components dt {};
    auto result = scn::detail::chrono_component_scanner<C>::scan(dt, ctx);
    if (not result) {
      return scn::unexpected(result.error());
    }
    assert(dt.year);
    assert(dt.mon);
    assert(dt.mday);
    val = std::chrono::year_month_day(
      std::chrono::year(*dt.year),
      std::chrono::month(static_cast<unsigned>(*dt.mon)),
      std::chrono::day(static_cast<unsigned>(*dt.mday)));
    return *result;
  }

  /// @endcond
};

// EOF
