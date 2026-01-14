/**
 * @file type-traits.h
 *
 * Template magic for types.
 */

#pragma once

#include "rocket/rocket.h"

#include <cstdint> // `int8_t`, `uint8_t`, ...
#include <type_traits>

namespace rocket {

// `Purge` --------------------------------------------------------------------------------------------------

/// An alias for `std::remove_cvref<T>`.
template<typename T>
using Purge = std::remove_cvref<T>;

/// An alias for `Purge<T>::type`.
template<typename T>
using PurgeType = Purge<T>::type;

static_assert(std::is_same_v<PurgeType<const volatile int>, int>);
static_assert(std::is_same_v<PurgeType<const std::true_type&>, std::true_type>);

// `Char` ---------------------------------------------------------------------------------------------------

template<int N> struct Char;

/**
 * 1-byte character: `char`.
 */
template<>
struct Char<1> {
  using Type = char; ///< @type_alias
};

/**
 * 4-byte character: `char32_t`.
 */
template<>
struct Char<4> {
  using Type = char32_t; ///< @type_alias
};

// `Int` ----------------------------------------------------------------------------------------------------

template<int N> struct Int;

/**
 * 1-byte signed integer: `int8_t`.
 */
template<>
struct Int<1> {
  using Type = int8_t; ///< @type_alias
};

/**
 * 2-byte signed integer: `int16_t`.
 */
template<>
struct Int<2> {
  using Type = int16_t; ///< @type_alias
};

/**
 * 4-byte signed integer: `int32_t`.
 */
template<>
struct Int<4> {
  using Type = int32_t; ///< @type_alias
};

/**
 * 8-byte signed integer: `int64_t`.
 */
template<>
struct Int<8> {
  using Type = int64_t; ///< @type_alias
};

/**
 * 16-byte signed integer: `int128_t`.
 */
template<>
struct Int<16> {
  using Type = int128_t; ///< @type_alias
};

// `Uint` ---------------------------------------------------------------------------------------------------

template<int N> struct Uint;

/**
 * 1-byte unsigned integer: `uint8_t`.
 */
template<>
struct Uint<1> {
  using Type = uint8_t; ///< @type_alias
};

/**
 * 2-byte unsigned integer: `uint16_t`.
 */
template<>
struct Uint<2> {
  using Type = uint16_t; ///< @type_alias
};

/**
 * 4-byte unsigned integer: `uint32_t`.
 */
template<>
struct Uint<4> {
  using Type = uint32_t; ///< @type_alias
};

/**
 * 8-byte unsigned integer: `uint64_t`.
 */
template<>
struct Uint<8> {
  using Type = uint64_t; ///< @type_alias
};

/**
 * 16-byte unsigned integer: `uint128_t`.
 */
template<>
struct Uint<16> {
  using Type = uint128_t; ///< @type_alias
};

// `Float` --------------------------------------------------------------------------------------------------

template<int N> struct Float;

/**
 * 4-byte floating point: `float`.
 *
 * @todo Find a portable way to express a four-byte floating point.
 */
template<>
struct Float<4> {
  using Type = float; ///< @type_alias
};

static_assert(sizeof(Float<4>::Type) == 4);

/**
 * 8-byte floating point: `double`.
 *
 * @todo Find a portable way to express an eight-byte floating point.
 */
template<>
struct Float<8> {
  using Type = double; ///< @type_alias
};

static_assert(sizeof(Float<8>::Type) == 8);

/**
 * 16-byte floating point: `long double`.
 *
 * @todo Find a portable way to express a sixteen-byte floating point.
 */
template<>
struct Float<16> {
  using Type = long double; ///< @type_alias
};

static_assert(sizeof(Float<16>::Type) == 16);

// Concepts -------------------------------------------------------------------------------------------------

template<typename T>
concept IsChar = std::is_same_v<PurgeType<T>, typename Char<sizeof(PurgeType<T>)>::Type>;

template<typename T>
concept IsInt = std::is_same_v<PurgeType<T>, typename Int<sizeof(PurgeType<T>)>::Type>;

template<typename T>
concept IsUint = std::is_same_v<PurgeType<T>, typename Uint<sizeof(PurgeType<T>)>::Type>;

template<typename T>
concept IsInteger = IsInt<T> || IsUint<T>;

template<typename T>
concept IsFloat = std::is_same_v<PurgeType<T>, typename Float<sizeof(PurgeType<T>)>::Type>;

// `LargestType` --------------------------------------------------------------------------------------------

/**
 * The `LargestType` template.
 *
 * Given a list of types, this template figures out the largest type.
 *
 * ## Examples
 *
 * ```
 * static_assert(std::is_same_v<LargestType<char, int>::Type, int>);
 * ```
 */
template <typename... Ts>
struct LargestType;

/// @spec{#rocket::LargestType, T}
template<typename T>
struct LargestType<T> {
  using Type = T; ///< @type_alias
};

/// @spec{#rocket::LargestType, T U Ts...}
template<typename T, typename U, typename... Ts>
struct LargestType<T, U, Ts...> {
  using Type = typename LargestType<
      typename std::conditional<(sizeof(T) >= sizeof(U)), T, U>::type, Ts...>::Type; ///< @type_alias
};

} // namespace rocket

// EOF
