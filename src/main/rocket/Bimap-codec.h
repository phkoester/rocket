/**
 * @file Bimap-codec.h
 *
 * Codec support for Bimaps.
 */

#pragma once

#include "rocket/codec/CompareEncoder.h"
#include "rocket/codec/EqualToEncoder.h"
#include "rocket/codec/FormattedCodec.h"
#include "rocket/nio/nio.h"
#include "rocket/unicode/ConvertTo.h"

#include <fmt/format.h>

// #boost::bimaps -------------------------------------------------------------------------------------------

namespace boost::bimaps {

/// @op_eq{`boost::bimaps::bimap`}
template<typename A, typename B>
inline bool
operator==(const bimap<A, B>& lhs, const bimap<A, B>& rhs) {
  return rocket::codec::EqualToEncoder<>().encode(lhs, rhs);
}

/// @op_cmp{`boost::bimaps::bimap`}
template<typename A, typename B> requires (not rocket::IsUnordered<bimap<A, B>>)
inline auto
operator<=>(const bimap<A, B>& lhs, const bimap<A, B>& rhs) {
  return rocket::codec::CompareEncoder<>().encode(lhs, rhs);
}

/// @op_output{`boost::bimaps::bimap`}
template<typename A, typename B>
inline std::ostream&
operator<<(std::ostream& lhs, const bimap<A, B>& rhs) {
  return lhs << fmt::format("{}", rhs);
}

/**
 * @fn_PrintTo{`boost::bimaps::bimap`}
 *
 * @todo Why do we need this? For some reason, GoogleTest doesn find `operator<<` ...
 */
template<typename A, typename B>
inline void
PrintTo(const bimap<A, B>& val, std::ostream* os) {
  *os << val;
}

} // namespace boost::bimaps

// #fmt::formatter<#boost::bimaps::bimap>--------------------------------------------------------------------

/**
 * @spec_fmt_formatter{`boost::bimaps::bimap`}
 *
 * - If the `i` format specifier is used, then the output is indented.
 */
template<typename A, typename B, typename C>
struct fmt::formatter<boost::bimaps::bimap<A, B>, C> {
  /// @cond undocumented

  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const boost::bimaps::bimap<A, B>& val, FormatContext& ctx) const {
    rocket::codec::FormattedCodec codec;
    rocket::nio::StringSink buf;
    codec.encode(val, buf, { .indent=indent_ });
    std::basic_string<C> str(rocket::unicode::ConvertTo<C>::apply(buf.str()));
    auto out = ctx.out();
    return detail::write<C>(out, str);
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    auto it = ctx.begin();
    auto end = ctx.end();
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
