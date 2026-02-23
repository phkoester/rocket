/**
 * @file Interval-codec.h
 *
 * Codec support for mathematical intervals.
 */

#pragma once

#include "rocket/codec/FormattedCodec.h"
#include "rocket/math/Interval.h"

#include <fmt/format.h>

#include <ostream>

namespace rocket::math {

// Functions ------------------------------------------------------------------------------------------------

// @op_output{#rocket::math::Interval}
template<typename Left, typename Right>
std::ostream&
operator<<(std::ostream& os, const Interval<Left, Right>& val) {
  return os << fmt::format("{}", val);
}

} // namespace rocket::math

// #fmt::formatter<#rocket::math::Interval> -----------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::math::Interval}
 *
 * - If the `i` format specifier is used, then the output is indented.
 */
template<typename Left, typename Right, typename C>
struct fmt::formatter<rocket::math::Interval<Left, Right>, C> {
  /// @cond undocumented

  using Type = rocket::math::Interval<Left, Right>;

  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const Type& val, FormatContext& ctx) const {
    auto out = ctx.out();
    const rocket::codec::FormattedCodec codec;
    rocket::nio::StringSink buf;
    codec.encode(val, buf, { .indent=indent_ });
    out = detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(buf.str()));
    return out;
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it == 'i') {
      indent_ = true;
      ++it;
    }
    return it;
  }

  /// @endcond

private:

  bool indent_ = false;
};

// EOF
