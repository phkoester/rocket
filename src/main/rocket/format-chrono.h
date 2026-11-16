/**
 * @file format-chrono.h
 *
 * {fmt} formatters for chrono types.
 */

#pragma once

#include "rocket/type-traits.h"
#include "rocket/chrono/chrono.h"

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <chrono>
#include <ctime>
#include <string>
#include <string_view>

// `fmt::formatter<rocket::chrono::zoned_time>` -------------------------------------------------------------

/**
 * Formats a #rocket::chrono::zoned_time.
 *
 * The format specification is that of #std::chrono::local_time, plus `%Z` (the abbreviation of the time
 * zone), `%z` (the UTC offset as `+hhmm`), and `%Ez` and `%Oz` (the UTC offset as `+hh:mm`). Without
 * conversion specifications, the format is `%F %T %Z`.
 *
 * @tparam Duration the duration of the time point
 * @tparam TimeZonePtr the time-zone pointer
 * @tparam C the character type
 */
template<typename Duration, typename TimeZonePtr, typename C> requires rocket::IsChar<C>
struct fmt::formatter<rocket::chrono::zoned_time<Duration, TimeZonePtr>, C> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(const rocket::chrono::zoned_time<Duration, TimeZonePtr>& val, FormatContext& ctx) const {
    using namespace rocket;

    static constexpr C defaultSpec[] = { '%', 'F', ' ', '%', 'T', ' ', '%', 'Z' };

    // The specification consists of a standard part (fill, alignment, width, and `L`) and a chrono part,
    // which starts at the first `%`. Without a chrono part, the format is `%F %T %Z`
    const u64 pos = spec_.find('%');
    std::basic_string_view<C> stdSpec = spec_.substr(0, pos);
    const std::basic_string_view<C> chronoSpec =
      pos == std::basic_string_view<C>::npos ? std::basic_string_view<C>(defaultSpec, 8) : spec_.substr(pos);
    const bool localized = not stdSpec.empty() && stdSpec.back() == 'L';
    if (localized) {
      stdSpec.remove_suffix(1);
    }

    // `%Z` and `%z` are written directly. All other conversions are formatted by {fmt}, in runs that start
    // with a conversion, because {fmt} does not accept chrono specifications that start with literal text.
    // Literal text before such a run is written directly as well
    const rocket::chrono::sys_info info = val.get_info();
    const SysTime time(val.get_local_time().time_since_epoch());
    fmt::basic_memory_buffer<C> buf;
    std::basic_string<C> run;

    const auto flush = [&] {
      if (not run.empty()) {
        formatTime(buf, run, localized, time, ctx.locale());
        run.clear();
      }
    };

    for (u64 i = 0; i < chronoSpec.size(); ++i) {
      const C c = chronoSpec[i];
      if (c != '%') {
        if (run.empty()) {
          buf.push_back(c);
        } else {
          run.push_back(c);
        }
        continue;
      }
      if (i + 1 == chronoSpec.size()) {
        throw fmt::format_error("invalid format");
      }
      const C next = chronoSpec[i + 1];
      if (next == 'Z') {
        flush();
        appendAbbrev(buf, info);
        i += 1;
      } else if (next == 'z') {
        flush();
        appendOffset(buf, info, false);
        i += 1;
      } else if ((next == 'E' || next == 'O') && i + 2 < chronoSpec.size() && chronoSpec[i + 2] == 'z') {
        flush();
        appendOffset(buf, info, true);
        i += 2;
      } else {
        run.push_back(c);
        run.push_back(next);
        i += 1;
      }
    }
    flush();

    // Fill, alignment, and width apply to the whole result
    fmt::formatter<std::basic_string_view<C>, C> formatter;
    fmt::parse_context<C> parseCtx(stdSpec);
    if (formatter.parse(parseCtx) != parseCtx.end()) {
      throw fmt::format_error("invalid format");
    }
    return formatter.format(std::basic_string_view<C>(buf.data(), buf.size()), ctx);
  }

  template<typename ParseContext>
  constexpr ParseContext::iterator
  parse(ParseContext& ctx) {
    // Chrono specifications cannot contain braces, so the specification extends to the closing brace. It is
    // only stored here and interpreted in `format`
    auto it = ctx.begin();
    while (it != ctx.end() && *it != '}') {
      ++it;
    }
    spec_ = std::basic_string_view<C>(ctx.begin(), it);
    return it;
  }

  /// @endcond

private:

  // The local time is deliberately formatted as a `sys_time`. {fmt} only provides a formatter for
  // `std::chrono::local_time` if `__cpp_lib_chrono >= 201907L`, and that formatter prints wrong subseconds
  // for times before the epoch. The time zone does not matter, as `%Z` and `%z` are never passed to {fmt}
  using SysTime = fmt::sys_time<typename rocket::chrono::zoned_time<Duration, TimeZonePtr>::duration>;

  std::basic_string_view<C> spec_;

  static void
  formatTime(
    fmt::basic_memory_buffer<C>& buf,
    std::basic_string_view<C> spec,
    bool localized,
    const SysTime& time,
    fmt::locale_ref loc) {
    std::basic_string<C> fullSpec;
    if (localized) {
      fullSpec.push_back('L');
    }
    fullSpec.append(spec);
    fmt::formatter<SysTime, C> formatter;
    fmt::parse_context<C> parseCtx(fullSpec);
    if (formatter.parse(parseCtx) != parseCtx.end()) {
      throw fmt::format_error("invalid format");
    }
    fmt::buffered_context<C> ctx(fmt::basic_appender<C>(buf), {}, loc);
    formatter.format(time, ctx);
  }

  static void
  appendAbbrev(fmt::basic_memory_buffer<C>& buf, const rocket::chrono::sys_info& info) {
    for (const char c : info.abbrev) {
      buf.push_back(static_cast<C>(c));
    }
  }

  static void
  appendOffset(fmt::basic_memory_buffer<C>& buf, const rocket::chrono::sys_info& info, bool colon) {
    const auto offset = info.offset;
    const auto abs = offset < std::chrono::seconds(0) ? -offset : offset;
    const auto h = std::chrono::duration_cast<std::chrono::hours>(abs).count();
    const auto m = std::chrono::duration_cast<std::chrono::minutes>(abs % std::chrono::hours(1)).count();
    const std::string str = colon ?
      fmt::format("{}{:02}:{:02}", offset < std::chrono::seconds(0) ? '-' : '+', h, m) :
      fmt::format("{}{:02}{:02}", offset < std::chrono::seconds(0) ? '-' : '+', h, m);
    for (const char c : str) {
      buf.push_back(static_cast<C>(c));
    }
  }
};

#ifndef ROCKET_HAS_FMT_FORMATTER_STD_CHRONO_YEAR_MONTH_DAY

// `fmt::formatter<std::chrono::year_month_day>` ------------------------------------------------------------

/**
 * Formats a #std::chrono::year_month_day with {fmt}.
 *
 * {fmt} only provides this formatter if `__cpp_lib_chrono >= 201907L`; otherwise it formats its own
 * `fmt::year_month_day`. This is a copy of {fmt}’s formatter from `fmt/chrono.h`. Without a format
 * specification, the format is `%F`.
 *
 * @tparam C the character type
 */
template<typename C> requires rocket::IsChar<C>
struct fmt::formatter<std::chrono::year_month_day, C> : private fmt::formatter<std::tm, C> {
  /// @cond undocumented

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

  constexpr auto
  parse(fmt::parse_context<C>& ctx) -> const C* {
    auto it = ctx.begin();
    auto end = ctx.end();
    useTmFormatter_ = it != end && *it != '}';
    return useTmFormatter_ ? fmt::formatter<std::tm, C>::parse(ctx) : it;
  }

  /// @endcond

private:

  bool useTmFormatter_ = false;
};

#endif // ROCKET_HAS_FMT_FORMATTER_STD_CHRONO_YEAR_MONTH_DAY

// EOF
