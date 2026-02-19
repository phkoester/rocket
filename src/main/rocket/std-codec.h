/**
 * @file std-codec.h
 *
 * Additional codec support for the standard library.
 */

#pragma once

#include "rocket/codec/CompareEncoder.h"
#include "rocket/codec/EqualToEncoder.h"

namespace std {

/// @op_eq{#std::span}
template<typename T, u64 Extent>
inline bool
operator==(span<T, Extent> lhs, span<T, Extent> rhs) {
  return rocket::codec::EqualToEncoder<>().encode(lhs, rhs);
}

/// @op_cmp{#std::span}
template<typename T, u64 Extent>
inline auto
operator<=>(span<T, Extent> lhs, span<T, Extent> rhs) {
  return rocket::codec::CompareEncoder<>().encode(lhs, rhs);
}

} // namespace std

// EOF
