/**
 * @file hash.h
 *
 * Hashing.
 */

#pragma once

#include "rocket/rocket.h"

#include <boost/functional/hash.hpp>

#include <functional>

namespace rocket::hash {

// #BoostHash -----------------------------------------------------------------------------------------------

/**
 * A hasher using `boost::hash`.
 */
 struct BoostHash {
  /**
    * Hash function.
    *
    * @tparam T the type of the value to hash
    * @param val the value to hash
    * @return the hash value
    */
  template<typename T>
  [[nodiscard]] u64
  operator()(const T& val) const {
    return boost::hash<T>()(val);
  }
};

// #StdHash -------------------------------------------------------------------------------------------------

/**
 * A hasher using #std::hash.
 */
struct StdHash {
  /**
   * Hash function.
   *
   * @tparam T the type of the value to hash
   * @param val the value to hash
   * @return the hash value
   */
  template<typename T>
  [[nodiscard]] u64
  operator()(const T& val) const {
    return std::hash<T>()(val);
  }
};

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
 * Combines two hash values.
 *
 * Taken from `boost::hash_combine`.
 *
 * @tparam T the type of the value to hash
 * @tparam Hash the hasher class
 * @param seed the seed value, which is modified
 * @param val the value to hash
 */
template<typename T, typename Hash = StdHash>
void
combineHash(u64& seed, const T& val) {
  combine(seed, Hash()(val));
}

} // namespace rocket::hash

// EOF
