/**
 * @file spans.h
 *
 * @c std::span utilities.
 *
 */

#pragma once

#include <span>

namespace rocket::spans {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Makes a nonconst byte span for the value @p v of arbitrary type @p T.
 *
 * @tparam T the type of @p v
 * @param v an arbitrary value
 * @return a new span which serves as a byte view on @p v
 */
template<typename T>
inline std::span<std::byte>
bytes(T& v) {
  return std::span<std::byte>(reinterpret_cast<std::byte*>(&v), sizeof(T));
}

/**
 * Makes a const byte span for the value @p v of arbitrary type @p T.
 *
 * @tparam T the type of @p v
 * @param v an arbitrary value
 * @return a new span which serves as a byte view on @p v
 */
template<typename T>
inline std::span<const std::byte>
bytes(const T& v) {
  return std::span<const std::byte>(reinterpret_cast<const std::byte*>(&v), sizeof(T));
}

} // namespace rocket::span

// EOF
