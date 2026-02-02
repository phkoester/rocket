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

namespace rocket::math::random {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Makes a new generator.
 *
 * @tparam Gen the generator type, defaults to #std::mt19937
 * @return a new generator
 */
template<typename Gen = std::mt19937>
inline Gen
gen() {
  std::random_device rd;
  return Gen(rd());
}

/**
 * Generates a random integer number in the closed interval @f$[lower,upper]@f$.
 *
 * @tparam Gen the generator type
 * @tparam I the integer type
 * @param gen the generator
 * @param lower the lower bound
 * @param upper the upper bound
 * @return a random integer number in the closed interval @f$[lower,upper]@f$.
 */
template<typename I, typename Gen> requires IsInteger<I>
inline I
uniformInteger(Gen& gen, I lower, I upper) {
  return std::uniform_int_distribution<I>(lower, upper)(gen);
}

/**
 * Returns a random character from the string @p chars.
 *
 * @tparam Gen the generator type
 * @param gen the generator
 * @param chars the characters to choose from
 * @return a random character from the string @p chars
 */
template<typename Gen>
char
charOf(Gen& gen, std::string_view chars) {
  ROCKET_CHECK(chars, chars.size() > 1);
  const u64 index = uniformInteger<u64>(gen, 0, chars.size() - 1);
  return chars[index];
}

/**
 * Returns a random string of length @p n, consisting of characters from the string @p chars.
 *
 * @tparam Gen the generator type
 * @param gen the generator
 * @param n the length of the string
 * @param chars the characters to choose from
 * @return a random string of length @p n, consisting of characters from the string @p chars
 */
template<typename Gen>
std::string
charsOf(Gen& gen, u64 n, std::string_view chars) {
  std::string ret;
  for (u64 i = 0; i < n; ++i) {
    ret.push_back(charOf(gen, chars));
  }
  return ret;
}

/**
 * Returns a random hexadecimal string of length @p n.
 *
 * @tparam Gen the generator type
 * @param gen the generator
 * @param n the length of the string
 * @return a random hexadecimal string of length @p n
 */
template<typename Gen>
std::string
hex(Gen& gen, u64 n) {
  return charsOf(gen, n, "0123456789abcdef");
}

} // namespace rocket::math::random

// EOF
