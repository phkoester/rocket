/**
 * @file hash.h
 *
 * Hashing utiltities.
 */

#pragma once

#include "rocket/rocket.h"

#include <boost/functional/hash.hpp>

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

} // namespace rocket::hash

// EOF
