/**
 * @file object.h
 *
 * Utillities around generic objects.
 */

#pragma once

#include <span>

namespace rocket {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Makes a nonconst span for the value @p v of arbitrary type @p T.
 *
 * @tparam E the element type of the span
 * @tparam T the type of @p v
 * @param v an arbitrary value
 * @return a new span which serves as buffer on @p v
 */
template<typename E, typename T>
inline std::span<E>
asSpan(T& v) {
  static_assert(sizeof(T) % sizeof(E) == 0);
  return std::span<E>(reinterpret_cast<E*>(&v), sizeof(T) / sizeof(E));
}

/**
 * Makes a const span for the value @p v of arbitrary type @p T.
 *
 * @tparam E the element type of the span
 * @tparam T the type of @p v
 * @param v an arbitrary value
 * @return a new span which serves as buffer on @p v
 */
template<typename E, typename T>
inline std::span<const E>
asSpan(const T& v) {
  static_assert(sizeof(T) % sizeof(E) == 0);
  return std::span<const E>(reinterpret_cast<const E*>(&v), sizeof(T) / sizeof(E));
}

} // namespace rocket

// EOF
