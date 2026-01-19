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

/**
 * Generates a 32-bit hash value for the given 32-bit value.
 *
 * Taken from https://stackoverflow.com/questions/664014/.
 *
 * @param val the value to hash
 * @return the hash value
 */
inline u32
hash32(u32 val) {
  val = ((val >> 16) ^ val) * UINT32_C(0x45d9f3b);
  val = ((val >> 16) ^ val) * UINT32_C(0x45d9f3b); // The same line—not a typo!
  val = (val >> 16) ^ val;
  return val;
}

/**
 * Generates a 64-bit hash value for the given 64-bit value.
 *
 * Taken from https://stackoverflow.com/questions/664014/.
 *
 * @param val the value to hash
 * @return the hash value
 */
inline u64
hash64(u64 val) {
  val = (val ^ (val >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
  val = (val ^ (val >> 27)) * UINT64_C(0x94d049bb133111eb);
  val = val ^ (val >> 31);
  return val;
}

/**
 * Reverses the hash of a 32-bit value.
 *
 * Taken from https://stackoverflow.com/questions/664014/.
 *
 * @param val the value to unhash
 * @return the unhashed value
 */
inline u32
unhash32(u32 val) {
  val = ((val >> 16) ^ val) * UINT32_C(0x119de1f3);
  val = ((val >> 16) ^ val) * UINT32_C(0x119de1f3); // The same line—not a typo!
  val = (val >> 16) ^ val;
  return val;
}

/**
 * Reverses the hash of a 64-bit value.
 *
 * Taken from https://stackoverflow.com/questions/664014/.
 *
 * @param val the value to unhash
 * @return the unhashed value
 */
inline u64
unhash64(u64 val) {
  val = (val ^ (val >> 31) ^ (val >> 62)) * UINT64_C(0x319642b2d24d8ec3);
  val = (val ^ (val >> 27) ^ (val >> 54)) * UINT64_C(0x96de1b173f119089);
  val = val ^ (val >> 30) ^ (val >> 60);
  return val;
}

} // namespace rocket

// EOF
