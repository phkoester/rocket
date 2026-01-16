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
 * @param v the value to hash
 */
 template<typename T, typename Hash = std::hash<T>>
 void
 combineHash(u64& seed, const T& v) {
   seed = boost::hash_detail::hash_mix(seed + 0x9e3779b9 + Hash()(v));
 }

/**
 * Generates a 32-bit hash value for the given 32-bit value.
 *
 * Taken from https://stackoverflow.com/questions/664014/.
 *
 * @param v the value to hash
 * @return the hash value
 */
inline u32
hash32(u32 v) {
  v = ((v >> 16) ^ v) * UINT32_C(0x45d9f3b);
  v = ((v >> 16) ^ v) * UINT32_C(0x45d9f3b); // The same line—not a typo!
  v = (v >> 16) ^ v;
  return v;
}

/**
 * Generates a 64-bit hash value for the given 64-bit value.
 *
 * Taken from https://stackoverflow.com/questions/664014/.
 *
 * @param v the value to hash
 * @return the hash value
 */
inline u64
hash64(u64 v) {
  v = (v ^ (v >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
  v = (v ^ (v >> 27)) * UINT64_C(0x94d049bb133111eb);
  v = v ^ (v >> 31);
  return v;
}

/**
 * Reverses the hash of a 32-bit value.
 *
 * Taken from https://stackoverflow.com/questions/664014/.
 *
 * @param v the value to unhash
 * @return the unhashed value
 */
inline u32
unhash32(u32 v) {
  v = ((v >> 16) ^ v) * UINT32_C(0x119de1f3);
  v = ((v >> 16) ^ v) * UINT32_C(0x119de1f3); // The same line—not a typo!
  v = (v >> 16) ^ v;
  return v;
}

/**
 * Reverses the hash of a 64-bit value.
 *
 * Taken from https://stackoverflow.com/questions/664014/.
 *
 * @param v the value to unhash
 * @return the unhashed value
 */
inline u64
unhash64(u64 v) {
  v = (v ^ (v >> 31) ^ (v >> 62)) * UINT64_C(0x319642b2d24d8ec3);
  v = (v ^ (v >> 27) ^ (v >> 54)) * UINT64_C(0x96de1b173f119089);
  v = v ^ (v >> 30) ^ (v >> 60);
  return v;
}

} // namespace rocket

// EOF
