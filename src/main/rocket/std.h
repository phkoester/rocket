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
 * The `option` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param val a value; either of type @p T or `std::optional<T>`
 * @return an optional value. If the value is to be taken from a `std::optional<T>`, that optional is
 *    returned directly, otherwise a nonnull optional
 */
template<typename T>
constexpr std::optional<T>
option(const T& val) {
  return std::optional<T>(val);
}

/**
 * The `option` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param val a value; either of type @p T or `std::optional<T>`
 * @return an optional value. If the value is to be taken from a `std::optional<T>`, that optional is
 *    returned directly, otherwise a nonnull optional
 */
template<typename T>
constexpr std::optional<T>&
option(std::optional<T>& val) {
  return val;
}

/**
 * The `option` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param val a value; either of type @p T or `std::optional<T>`
 * @return an optional value. If the value is to be taken from a `std::optional<T>`, that optional is
 *    returned directly, otherwise a nonnull optional
 */
template<typename T>
constexpr const std::optional<T>&
option(const std::optional<T>& val) {
  return val;
}

/**
 * The `value` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param val a value; either of type @p T or `std::optional<T>`
 * @return a value. If the value is to be taken from a `std::optional<T>`, that optional is dereferenced,
 *   otherwise the value is returned directly
 */
template<typename T>
constexpr T&
value(T& val) {
  return val;
}

/**
 * The `value` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param val a value; either of type @p T or `std::optional<T>`
 * @return a value. If the value is to be taken from a `std::optional<T>`, that optional is dereferenced,
 *   otherwise the value is returned directly
 */
template<typename T>
constexpr const T&
value(const T& val) {
  return val;
}

/**
 * The `value` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param val a value; either of type @p T or `std::optional<T>`
 * @return a value. If the value is to be taken from a `std::optional<T>`, that optional is dereferenced,
 *   otherwise the value is returned directly
 */
template<typename T>
constexpr T&
value(std::optional<T>& val) {
  return *val;
}

/**
 * The `value` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param val a value; either of type @p T or `std::optional<T>`
 * @return a value. If the value is to be taken from a `std::optional<T>`, that optional is dereferenced,
 *   otherwise the value is returned directly
 */
template<typename T>
constexpr const T&
value(const std::optional<T>& val) {
  return *val;
}

} // namespace rocket

// EOF
