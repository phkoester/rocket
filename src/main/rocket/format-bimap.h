/**
 * @file format-bimap.h
 *
 * Formatting support for Bimaps.
 */

#pragma once

#include "rocket/bimap.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <ostream>
#include <type_traits>

// `fmt::range_format_kind<boost::bimaps::bimap>` -----------------------------------------------------------

/// Keeps the range formatters of {fmt} away from Bimaps, which are formatted by the formatter below.
template<typename A, typename B, typename C>
struct fmt::range_format_kind<boost::bimaps::bimap<A, B>, C> :
  std::integral_constant<fmt::range_format, fmt::range_format::disabled> {};

// `fmt::formatter<boost::bimaps::bimap>` -------------------------------------------------------------------

/**
 * @spec_fmt_formatter{boost::bimaps::bimap}
 *
 * The formatter formats the left view of the bimap, just as {fmt} formats a #std::map or a
 * #std::unordered_map: as `{k1: v1, k2: v2}`, with keys and values in their debug format, so that strings
 * are quoted and escaped.
 *
 * - If the `n` format specifier is used, then the braces are omitted.
 * - Like for a #std::map, no other format specifiers are accepted, except for an empty underlying format
 *   specification after a colon, as in `{::}` or `{:n:}`.
 */
template<typename A, typename B, typename C>
struct fmt::formatter<boost::bimaps::bimap<A, B>, C> {
  /// @cond undocumented

  using Type = boost::bimaps::bimap<A, B>;
  using Key = std::remove_cvref_t<typename Type::left_key_type>;
  using Value = std::remove_cvref_t<typename Type::left_data_type>;

  constexpr const C*
  parse(parse_context<C>& ctx) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end) {
      if (*it == 'n') {
        noDelimiters_ = true;
        ++it;
      }
      if (it != end && *it != '}') {
        if (*it != ':') {
          report_error("invalid format specifier");
        }
        ++it;
      }
      ctx.advance_to(it);
    }
    keyFormatter_.parse(ctx);
    detail::maybe_set_debug_format(keyFormatter_, true);
    valueFormatter_.parse(ctx);
    detail::maybe_set_debug_format(valueFormatter_, true);
    return it;
  }

  template<typename FormatContext>
  FormatContext::iterator
  format(const Type& val, FormatContext& ctx) const {
    static constexpr C open[] = { C('{') };
    static constexpr C close[] = { C('}') };
    static constexpr C sep[] = { C(','), C(' ') };
    static constexpr C keySep[] = { C(':'), C(' ') };

    auto out = ctx.out();
    if (not noDelimiters_) {
      out = detail::copy<C>(basic_string_view<C>(open, 1), out);
    }
    bool first = true;
    for (const auto& elem : val.left) {
      if (not first) {
        out = detail::copy<C>(basic_string_view<C>(sep, 2), out);
      }
      first = false;
      ctx.advance_to(out);
      out = keyFormatter_.format(elem.first, ctx);
      out = detail::copy<C>(basic_string_view<C>(keySep, 2), out);
      ctx.advance_to(out);
      out = valueFormatter_.format(elem.second, ctx);
    }
    if (not noDelimiters_) {
      out = detail::copy<C>(basic_string_view<C>(close, 1), out);
    }
    return out;
  }

  /// @endcond

private:

  fmt::formatter<Key, C> keyFormatter_;
  fmt::formatter<Value, C> valueFormatter_;
  bool noDelimiters_ = false;
};

// `boost::bimaps` ------------------------------------------------------------------------------------------

namespace boost::bimaps {

/**
 * @fn_PrintTo{boost::bimaps::bimap}
 *
 * GoogleTest prints iterable types with its container printer before it considers `operator<<`. A bimap's
 * elements are Boost relations without a printer, so they would be dumped as raw bytes. A `PrintTo` found
 * by argument-dependent lookup (ADL) takes precedence over that. This function must live in `boost::bimaps`,
 * the namespace of boost::bimaps::bimap.
 */
template<typename A, typename B>
inline void
PrintTo(const bimap<A, B>& val, std::ostream* os) {
  *os << fmt::format("{}", val);
}

} // namespace boost::bimaps

// EOF
