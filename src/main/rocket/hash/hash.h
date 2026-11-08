/**
 * @file hash.h
 *
 * Hashing utiltities.
 */

#pragma once

#include "rocket/rocket.h"

#include <boost/functional/hash.hpp>

#ifdef ROCKET_HAS_BOOST_INT128
  #include <boost/int128/hash.hpp>
#endif

namespace rocket::hash {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Combines two hash values.
 *
 * Taken from `boost::hash_combine`.
 *
 * @param seed the seed value, which is modified
 * @param hash the hash value to combine
 */
inline void
combine(u64& seed, u64 hash) {
  seed = boost::hash_detail::hash_mix(seed + 0x9e3779b9 + hash);
}

/**
 * Computes a hash value for an unsigned 128-bit integer by combining its low and high 64-bit halves.
 *
 * @param val the value to hash
 * @return the hash value
 */
inline u64
hash128(u128 val) noexcept {
  auto seed = static_cast<u64>(val);
  combine(seed, static_cast<u64>(val >> 64));
  return seed;
}

} // namespace rocket::hash

#ifndef ROCKET_HAS_INT128_STD_HASH

// `std::hash<rocket::i128>`, `std::hash<rocket::u128>` -----------------------------------------------------

/// @spec_std_hash{#rocket::i128}
template<>
struct std::hash<rocket::i128> {
  /// @cond undocumented
  rocket::std_size_t
  operator()(rocket::i128 val) const noexcept {
    return rocket::hash::hash128(static_cast<rocket::u128>(val));
  }
  /// @endcond
};

/// @spec_std_hash{#rocket::u128}
template<>
struct std::hash<rocket::u128> {
  /// @cond undocumented
  rocket::std_size_t
  operator()(rocket::u128 val) const noexcept {
    return rocket::hash::hash128(val);
  }
  /// @endcond
};

#endif // ROCKET_HAS_INT128_STD_HASH

// EOF
