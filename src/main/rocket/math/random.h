/**
 * @file random.h
 *
 * Random numbers.
 */

#pragma once

#include "rocket/assert.h"
#include "rocket/type-traits.h"

#include <random>
#include <string_view>

namespace rocket::math {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Makes a new default random number generator.
 *
 * @return a new default random number generator
 */
inline std::mt19937
gen() {
  std::random_device rd;
  return std::mt19937(rd());
}

/**
 * Generates a random integer number in the closed interval @f$[lower,upper]@f$.
 *
 * @tparam Gen the generator type
 * @tparam I the value type
 * @param gen the generator
 * @param lower the lower bound
 * @param upper the upper bound
 * @return a random integer number in the closed interval @f$[lower,upper]@f$.
 */
template<typename Gen, typename I> requires IsInteger<I>
I
random(Gen& gen, I lower, I upper) {
  std::uniform_int_distribution<I> distrib(lower, upper);
  return distrib(gen);
}

/**
 * Returns a random character from the string @p chars.
 *
 * @tparam Generator the generator type
 * @param gen the generator
 * @param chars the characters to choose from
 * @return a random character from the string @p chars
 */
template<typename Generator>
char
randomChar(Generator& gen, std::string_view chars) {
  ROCKET_CHECK(chars, chars.size() > 1);
  u64 index = random<u64>(gen, 0, chars.size() - 1);
  return chars[index];
}

/**
 * Returns a random string of length @p n, consisting of characters from the string @p chars.
 *
 * @tparam Generator the generator type
 * @param gen the generator
 * @param n the length of the string
 * @param chars the characters to choose from
 * @return a random string of length @p n, consisting of characters from the string @p chars
 */
template<typename Generator>
std::string
randomChars(Generator& gen, u64 n, std::string_view chars) {
  std::string ret;
  for (u64 i = 0; i < n; ++i) {
    ret.push_back(randomChar(gen, chars));
  }
  return ret;
}

/**
 * Returns a random hexadecimal string of length @p n.
 *
 * @tparam Generator the generator type
 * @param gen the generator
 * @param n the length of the string
 * @return a random hexadecimal string of length @p n
 */
template<typename Generator>
std::string
randomHex(Generator& gen, u64 n) {
  return randomChars(gen, n, "0123456789abcdef");
}

} // namespace rocket::math

// EOF
