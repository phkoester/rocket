/**
 * @file format.h
 */

#pragma once

#include "rocket/type-traits.h"
#include "rocket/chrono/chrono.h"
#include "rocket/io/io.h"
#include "rocket/unicode/ConvertTo.h"

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/std.h>
#include <fmt/xchar.h>

#ifndef ROCKET_HAS_INT128_FMT_FORMATTER

// `fmt::formatter<rocket::i128>` ---------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::i128}
 *
 * This formatter uses the same format specifiers as the underlying string formatter.
 */
template<typename C> requires rocket::IsChar<C>
struct fmt::formatter<rocket::i128, C> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(rocket::i128 val, FormatContext& ctx) const {
    std::ostringstream os;
    os << val;
    auto str = os.str();
    return underlying_.format(rocket::unicode::ConvertTo<C>::apply(str), ctx);
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return underlying_.parse(ctx);
  }

  constexpr void
  set_debug_format(bool val = true) {
    underlying_.set_debug_format(val);
  }

  /// @endcond

private:

  fmt::formatter<basic_string_view<C>, C> underlying_;
};

// `fmt::formatter<rocket::u128>` ---------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::u128}
 *
 * This formatter uses the same format specifiers as the underlying string formatter.
 */
template<typename C> requires rocket::IsChar<C>
struct fmt::formatter<rocket::u128, C> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(rocket::u128 val, FormatContext& ctx) const {
    std::ostringstream os;
    os << val;
    auto str = os.str();
    return underlying_.format(rocket::unicode::ConvertTo<C>::apply(str), ctx);
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return underlying_.parse(ctx);
  }

  constexpr void
  set_debug_format(bool val = true) {
    underlying_.set_debug_format(val);
  }

  /// @endcond

private:

  fmt::formatter<basic_string_view<C>, C> underlying_;
};

#endif

#ifndef ROCKET_HAS_STD_CHRONO_TIME_ZONE

// `std::formatter<rocket::chrono::zoned_time>` -------------------------------------------------------------

/**
 * Formats a #rocket::chrono::zoned_time, like #std::chrono::zoned_time is formatted.
 *
 * The format specification is that of #std::chrono::local_time, plus `%Z` (the abbreviation of the time
 * zone), `%z` (the UTC offset as `+hhmm`), and `%Ez` and `%Oz` (the UTC offset as `+hh:mm`). Without
 * conversion specifications, the format is `%F %T %Z`.
 *
 * @tparam Duration the duration of the time point
 * @tparam TimeZonePtr the time-zone pointer
 * @tparam CharT the character type
 */
template<typename Duration, typename TimeZonePtr, typename CharT>
struct std::formatter<rocket::chrono::zoned_time<Duration, TimeZonePtr>, CharT> {
  /// @cond undocumented

  template<typename ParseContext>
  constexpr ParseContext::iterator
  parse(ParseContext& ctx) {
    // Chrono specifications cannot contain braces, so the specification extends to the closing brace. It is
    // only stored here; the formatter of `local_time` parses it in `format`, after substituting `%Z` and `%z`
    auto it = ctx.begin();
    while (it != ctx.end() && *it != '}') {
      ++it;
    }
    spec_ = std::basic_string_view<CharT>(ctx.begin(), it);
    return it;
  }

  template<typename FormatContext>
  FormatContext::iterator
  format(const rocket::chrono::zoned_time<Duration, TimeZonePtr>& val, FormatContext& ctx) const {
    using LocalTime = std::chrono::local_time<typename rocket::chrono::zoned_time<Duration, TimeZonePtr>::duration>;

    const rocket::chrono::sys_info info = val.get_info();
    std::basic_string<CharT> spec;
    bool hasConversion = false;

    for (std::size_t i = 0; i < spec_.size(); ++i) {
      const CharT c = spec_[i];
      if (c != '%' || i + 1 == spec_.size()) {
        spec.push_back(c);
        continue;
      }
      hasConversion = true;
      const CharT next = spec_[i + 1];
      if (next == 'Z') {
        appendAbbrev(spec, info);
        i += 1;
      } else if (next == 'z') {
        appendOffset(spec, info, false);
        i += 1;
      } else if ((next == 'E' || next == 'O') && i + 2 < spec_.size() && spec_[i + 2] == 'z') {
        appendOffset(spec, info, true);
        i += 2;
      } else {
        spec.push_back(c);
        spec.push_back(next);
        i += 1;
      }
    }
    if (!hasConversion) {
      for (const char c : std::string_view("%F %T ")) {
        spec.push_back(static_cast<CharT>(c));
      }
      appendAbbrev(spec, info);
    }

    std::formatter<LocalTime, CharT> formatter;
    std::basic_format_parse_context<CharT> parseCtx(spec);
    formatter.parse(parseCtx);
    return formatter.format(val.get_local_time(), ctx);
  }

  /// @endcond

private:

  std::basic_string_view<CharT> spec_;

  static void
  appendAbbrev(std::basic_string<CharT>& spec, const rocket::chrono::sys_info& info) {
    for (const char c : info.abbrev) {
      spec.push_back(static_cast<CharT>(c));
    }
  }

  static void
  appendOffset(std::basic_string<CharT>& spec, const rocket::chrono::sys_info& info, bool colon) {
    const auto offset = info.offset;
    const auto abs = offset < std::chrono::seconds(0) ? -offset : offset;
    const auto h = std::chrono::duration_cast<std::chrono::hours>(abs).count();
    const auto m = std::chrono::duration_cast<std::chrono::minutes>(abs % std::chrono::hours(1)).count();
    const std::string str = colon
      ? std::format("{}{:02}:{:02}", offset < std::chrono::seconds(0) ? '-' : '+', h, m)
      : std::format("{}{:02}{:02}", offset < std::chrono::seconds(0) ? '-' : '+', h, m);
    for (const char c : str) {
      spec.push_back(static_cast<CharT>(c));
    }
  }
};

// `fmt::formatter<std::chrono::year_month_day>` -----------------------------------------------------------

/**
 * Formats a #std::chrono::year_month_day with {fmt}.
 *
 * {fmt} only provides this formatter if `__cpp_lib_chrono >= 201907L`; otherwise it formats its own
 * `fmt::year_month_day`. This is a copy of {fmt}'s formatter from `fmt/chrono.h`. Without a format
 * specification, the format is `%F`.
 *
 * @tparam C the character type
 */
template<typename C> requires rocket::IsChar<C>
struct fmt::formatter<std::chrono::year_month_day, C> : private fmt::formatter<std::tm, C> {
  /// @cond undocumented

  constexpr auto
  parse(fmt::parse_context<C>& ctx) -> const C* {
    auto it = ctx.begin();
    auto end = ctx.end();
    useTmFormatter_ = it != end && *it != '}';
    return useTmFormatter_ ? fmt::formatter<std::tm, C>::parse(ctx) : it;
  }

  template<typename FormatContext>
  auto
  format(std::chrono::year_month_day val, FormatContext& ctx) const -> decltype(ctx.out()) {
    auto time = std::tm();
    time.tm_year = static_cast<int>(val.year()) - 1900;
    time.tm_mon = static_cast<int>(static_cast<unsigned>(val.month())) - 1;
    time.tm_mday = static_cast<int>(static_cast<unsigned>(val.day()));
    if (useTmFormatter_) {
      return fmt::formatter<std::tm, C>::format(time, ctx);
    }
    fmt::detail::get_locale loc(true, ctx.locale());
    auto w = fmt::detail::tm_writer<decltype(ctx.out()), C>(loc, ctx.out(), time);
    w.on_iso_date();
    return w.out();
  }

  /// @endcond

private:

  bool useTmFormatter_ = false;
};

#endif // ROCKET_HAS_STD_CHRONO_TIME_ZONE

// EOF
