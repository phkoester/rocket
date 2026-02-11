/**
 * @file hash.h
 *
 * Hashing.
 */

#pragma once

#include "rocket/rocket.h"

#include <boost/functional/hash.hpp>

#include <functional>

namespace rocket {

/**
 * Combines two hash values.
 *
 * Taken from `boost::hash_combine`.
 *
 * @tparam T the type of the value
 * @tparam Hash the hasher class
 * @param seed the seed value, which is modified
 * @param val the value to hash
 */
template<typename T, typename Hash = std::hash<T>>
void
combineHash(u64& seed, const T& val) {
  seed = boost::hash_detail::hash_mix(seed + 0x9e3779b9 + Hash()(val));
}

} // namespace rocket

// EOF
