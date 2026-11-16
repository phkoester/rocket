/**
 * @file Bimap-codec.h
 *
 * Codec support for Bimaps.
 */

#pragma once

#include "rocket/format.h"
#include "rocket/codec/CompareEncoder.h"
#include "rocket/codec/EqualToEncoder.h"
#include "rocket/codec/FormattedCodec.h"
#include "rocket/nio/nio.h"
#include "rocket/unicode/ConvertTo.h"

// `boost::bimaps` ------------------------------------------------------------------------------------------

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
  rocket::nio::StringSink buf;
  rocket::codec::FormattedCodec().encode(rhs, buf);
  return lhs << buf.str();
}

/**
 * @fn_PrintTo{`boost::bimaps::bimap`}
 *
 * @todo Why do we need this? For some reason, GoogleTest doesn’t find `operator<<` ...
 */
template<typename A, typename B>
inline void
PrintTo(const bimap<A, B>& val, std::ostream* os) {
  *os << val;
}

} // namespace boost::bimaps

// EOF
