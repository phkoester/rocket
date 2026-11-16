/**
 * @file Interval-codec.h
 *
 * Codec support for mathematical intervals.
 */

#pragma once

#include "rocket/format.h"
#include "rocket/codec/FormattedCodec.h"
#include "rocket/format/CodecFormatter.h"
#include "rocket/math/Interval.h"

#include <ostream>

namespace rocket::math {

// Functions ------------------------------------------------------------------------------------------------

/// @op_output{#rocket::math::Interval}
template<typename Left, typename Right>
std::ostream&
operator<<(std::ostream& lhs, const Interval<Left, Right>& rhs) {
  return lhs << fmt::format("{}", rhs);
}

} // namespace rocket::math

// `fmt::formatter<rocket::math::Interval>` -----------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::math::Interval}
 *
 * - If the `i` format specifier is used, then the output is indented.
 */
template<typename Left, typename Right, typename C> requires rocket::IsChar<C>
struct fmt::formatter<rocket::math::Interval<Left, Right>, C> : rocket::internal::format::CodecFormatter<C> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(const rocket::math::Interval<Left, Right>& val, FormatContext& ctx) const {
    return this->formatEncoded(val, ctx);
  }

  /// @endcond
};

// EOF
