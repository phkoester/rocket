/**
 * @file std.h
 *
 * Additional standard library support.
 */

#pragma once

#include "rocket/TypeTraits.h"
#include "rocket/hash.h"

#include <functional>
#include <optional>
#include <span>
#include <tuple>

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

/**
 * The `option` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or `std::optional<T>`
 * @return an optional value. If the value is to be taken from a `std::optional<T>`, that optional is
 *    returned directly, otherwise a nonnull optional
 */
template<typename T>
constexpr std::optional<T>
option(const T& v) {
  return std::optional<T>(v);
}

/**
 * The `option` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or `std::optional<T>`
 * @return an optional value. If the value is to be taken from a `std::optional<T>`, that optional is
 *    returned directly, otherwise a nonnull optional
 */
template<typename T>
constexpr std::optional<T>&
option(std::optional<T>& v) {
  return v;
}

/**
 * The `option` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or `std::optional<T>`
 * @return an optional value. If the value is to be taken from a `std::optional<T>`, that optional is
 *    returned directly, otherwise a nonnull optional
 */
template<typename T>
constexpr const std::optional<T>&
option(const std::optional<T>& v) {
  return v;
}

/**
 * The `value` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or `std::optional<T>`
 * @return a value. If the value is to be taken from a `std::optional<T>`, that optional is dereferenced,
 *     otherwise the value is returned directly
 */
template<typename T>
constexpr T&
value(T& v) {
  return v;
}

/**
 * The `value` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or `std::optional<T>`
 * @return a value. If the value is to be taken from a `std::optional<T>`, that optional is dereferenced,
 *     otherwise the value is returned directly
 */
template<typename T>
constexpr const T&
value(const T& v) {
  return v;
}

/**
 * The `value` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or `std::optional<T>`
 * @return a value. If the value is to be taken from a `std::optional<T>`, that optional is dereferenced,
 *     otherwise the value is returned directly
 */
template<typename T>
constexpr T&
value(std::optional<T>& v) {
  return *v;
}

/**
 * The `value` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or `std::optional<T>`
 * @return a value. If the value is to be taken from a `std::optional<T>`, that optional is dereferenced,
 *     otherwise the value is returned directly
 */
template<typename T>
constexpr const T&
value(const std::optional<T>& v) {
  return *v;
}

} // namespace rocket

// `std::hash<tuple>` ---------------------------------------------------------------------------------------

/// @spec_std_hash{`std::tuple`}
template<typename... T>
struct std::hash<std::tuple<T...>> {
  /// @cond undocumented

  size_t
  operator()(const tuple<T...>& v) const {
    using TupleType = rocket::PurgeType<decltype(v)>;
    size_t ret = tuple_size<TupleType>::value;
    apply([&](auto&&... arg) { (rocket::combineHash(ret, arg), ...); }, v);
    return ret;
  }

  /// @endcond
};

// EOF
