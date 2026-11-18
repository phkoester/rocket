/**
 * @file Interval-format.h
 *
 * Formatting support for mathematical intervals.
 */

#pragma once

#include "rocket/type-traits.h"
#include "rocket/format/StringFormatter.h"
#include "rocket/math/Interval.h"

#include <fmt/format.h>

#include <iterator>
#include <string>
#include <string_view>

// Internal -------------------------------------------------------------------------------------------------

namespace rocket::math::internal::interval {

/**
 * Appends the bound @p val to the UTF-8 string @p out. Infinite floating-point values are written as `-∞` and
 * `∞`.
 *
 * @tparam T the element type
 * @param out the output string
 * @param val the bound
 */
template<typename T>
void
appendBound(std::string& out, T val) {
  if constexpr (IsFloat<T>) {
    if (val == -std::numeric_limits<T>::infinity()) {
      out += "-∞";
      return;
    }
    if (val == std::numeric_limits<T>::infinity()) {
      out += "∞";
      return;
    }
  }
  fmt::format_to(std::back_inserter(out), "{}", val);
}

/**
 * Appends the bound @p val to the UTF-8 string @p out. A missing bound is written as @p unbounded.
 *
 * @tparam Traits the bound traits
 * @param out the output string
 * @param val the bound
 * @param unbounded the notation of a missing bound
 */
template<typename Traits>
void
appendBound(std::string& out, const typename Traits::BoundType& val, std::string_view unbounded) {
  if constexpr (Traits::IsClosed) {
    appendBound(out, val);
  } else if (val) {
    appendBound(out, *val);
  } else {
    out += unbounded;
  }
}

/**
 * Returns the UTF-8 notation of the interval @p val, e.g., `∅`, `[1,2]`, `(-∞,2]`, or `[1.5,∞)`.
 *
 * @tparam Left the type of the lower-bound traits
 * @tparam Right the type of the upper-bound traits
 * @param val the interval
 * @return the notation of @p val
 */
template<typename Left, typename Right>
std::string
toUtf8(const Interval<Left, Right>& val) {
  if (val.empty()) {
    return "∅";
  }
  std::string ret;
  ret += Left::Symbol;
  appendBound<Left>(ret, val.a, "-∞");
  ret += ',';
  appendBound<Right>(ret, val.b, "∞");
  ret += Right::Symbol;
  return ret;
}

} // namespace rocket::math::internal::interval

// `fmt::formatter<rocket::math::Interval>` -----------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::math::Interval}
 *
 * An empty interval is written as `∅`. A nonempty interval is written with its bounds separated by a comma
 * and no space, enclosed in `[` or `]` for a closed bound and `(` or `)` for an open bound. A missing lower
 * bound is written as `-∞`, a missing upper bound as `∞`. Example values: `[1,2]`, `(-1.5,3)`, `(-∞,12]`,
 * `[12,∞)`.
 *
 * No format specifiers are accepted.
 */
template<typename Left, typename Right, typename C> requires rocket::IsChar<C>
struct fmt::formatter<rocket::math::Interval<Left, Right>, C> {
  /// @cond undocumented

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return rocket::format::internal::parseFlags(ctx, [](C) { return false; });
  }

  template<typename FormatContext>
  FormatContext::iterator
  format(const rocket::math::Interval<Left, Right>& val, FormatContext& ctx) const {
    return rocket::format::internal::writeUtf8<C>(ctx.out(), rocket::math::internal::interval::toUtf8(val));
  }

  /// @endcond
};

// EOF
