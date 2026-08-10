/**
 * @file std.h
 *
 * Additional support for the standard library.
 */

#pragma once

#include <optional>
#include <span>

namespace rocket {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Makes a nonconst span for the value @p val of arbitrary type @p T.
 *
 * @tparam E the element type of the span
 * @tparam T the type of @p val
 * @param val an arbitrary value
 * @return a new span which serves as buffer on @p val
 */
template<typename E, typename T>
inline std::span<E>
asSpan(T& val) {
  static_assert(sizeof(T) % sizeof(E) == 0);
  return std::span<E>(reinterpret_cast<E*>(&val), sizeof(T) / sizeof(E));
}

/**
 * Makes a const span for the value @p val of arbitrary type @p T.
 *
 * @tparam E the element type of the span
 * @tparam T the type of @p val
 * @param val an arbitrary value
 * @return a new span which serves as buffer on @p val
 */
template<typename E, typename T>
inline std::span<const E>
asSpan(const T& val) {
  static_assert(sizeof(T) % sizeof(E) == 0);
  return std::span<const E>(reinterpret_cast<const E*>(&val), sizeof(T) / sizeof(E));
}

/**
 * The `optionalOf` function has two overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param val a value; either of type @p T or `std::optional<T>`
 * @return an optional value. If the value is to be taken from a `std::optional<T>`, that optional is
 *    returned directly, otherwise a nonnull optional
 */
template<typename T>
constexpr std::optional<T>
optionalOf(const T& val) {
  return val;
}

/**
 * The `optionalOf` function has two overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param val a value; either of type @p T or `std::optional<T>`
 * @return an optional value. If the value is to be taken from a `std::optional<T>`, that optional is
 *    returned directly, otherwise a nonnull optional
 */
template<typename T>
constexpr const std::optional<T>&
optionalOf(const std::optional<T>& val) {
  return val;
}

/**
 * The `valueOf` function has two overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param val a value; either of type @p T or `std::optional<T>`
 * @return a value. If the value is to be taken from a `std::optional<T>`, that optional is dereferenced,
 *   otherwise the value is returned directly
 */
template<typename T>
constexpr const T&
valueOf(const T& val) {
  return val;
}

/**
 * The `valueOf` function has two overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param val a value; either of type @p T or `std::optional<T>`
 * @return a value. If the value is to be taken from a `std::optional<T>`, that optional is dereferenced,
 *   otherwise the value is returned directly
 * @throw std::bad_optional_access if the value is not present
 */
template<typename T>
constexpr const T&
valueOf(const std::optional<T>& val) {
  return val.value();
}

} // namespace rocket

// EOF
