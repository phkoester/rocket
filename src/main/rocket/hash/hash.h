/// @file hash.h
///
/// Hashing utilities.

#pragma once

#include "rocket/rocket.h"

#include <boost/container_hash/hash.hpp>

namespace rocket::hash {

// Functions ------------------------------------------------------------------------------------------------

/// Combines two hash values.
///
/// @param seed the seed value, which is modified
/// @param hash the hash value to combine
inline void
combine(u64& seed, u64 hash) {
  std::size_t s = seed;
  boost::hash_combine(s, hash);
  seed = s;
}

} // namespace rocket::hash

// EOF
