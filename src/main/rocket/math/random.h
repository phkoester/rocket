/**
 * @file random.h
 *
 * Random numbers.
 */

#pragma once

#include "rocket/assert.h"

#include <random>

namespace rocket::math {

// Functions ------------------------------------------------------------------------------------------------

inline std::mt19937
gen() {
  std::random_device rd;
  return std::mt19937(rd());
}

template<typename Gen, typename T>
T
random(Gen& gen, T lower, T upper) {
  std::uniform_int_distribution<T> distrib(lower, upper);
  return distrib(gen);
}

template<typename Generator>
char
randomChar(Generator& gen, std::string_view chars) {
  ROCKET_CHECK(chars, chars.size() > 1);
  size_t index = random(gen, 0UL, chars.size() - 1);
  return chars[index];
}

template<typename Generator>
std::string
randomChars(Generator& gen, size_t n, std::string_view chars) {
  std::string ret;
  for (size_t i = 0; i < n; ++i) {
    ret.push_back(randomChar(gen, chars));
  }
  return ret;
}

template<typename Generator>
std::string
randomHex(Generator& gen, size_t n) {
  return randomChars(gen, n, "0123456789abcdef");
}

} // namespace rocket::math

// EOF
