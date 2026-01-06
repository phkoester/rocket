/**
 * @file TypeTraits.h
 *
 * Template magic for types.
 */

#pragma once

#include "rocket/rocket.h"

#include <cstdint> // `uint8_t`, ...
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

template<int N> struct Char : std::false_type {};

/**
 * 1-byte character: `char`.
 */
template<>
struct Char<1> : std::true_type {
  using Type = char; ///< @type_alias
};

/**
 * 4-byte character: `char32_t`.
 */
template<>
struct Char<4> : std::true_type {
  using Type = char32_t; ///< @type_alias
};

// `Int` ----------------------------------------------------------------------------------------------------

template<int N> struct Int : std::false_type {};

/**
 * 1-byte signed integer: `int8_t`.
 */
template<>
struct Int<1> : std::true_type {
  using Type = int8_t; ///< @type_alias
};

/**
 * 2-byte signed integer: `int16_t`.
 */
template<>
struct Int<2> : std::true_type {
  using Type = int16_t; ///< @type_alias
};

/**
 * 4-byte signed integer: `int32_t`.
 */
template<>
struct Int<4> : std::true_type {
  using Type = int32_t; ///< @type_alias
};

/**
 * 8-byte signed integer: `int64_t`.
 */
template<>
struct Int<8> : std::true_type {
  using Type = int64_t; ///< @type_alias
};

/**
 * 16-byte signed integer: `int128_t`.
 */
template<>
struct Int<16> : std::true_type {
  using Type = int128_t; ///< @type_alias
};

// `Uint` ---------------------------------------------------------------------------------------------------

template<int N> struct Uint : std::false_type {};

/**
 * 1-byte unsigned integer: `uint8_t`.
 */
template<>
struct Uint<1> : std::true_type {
  using Type = uint8_t; ///< @type_alias
};

/**
 * 2-byte unsigned integer: `uint16_t`.
 */
template<>
struct Uint<2> : std::true_type {
  using Type = uint16_t; ///< @type_alias
};

/**
 * 4-byte unsigned integer: `uint32_t`.
 */
template<>
struct Uint<4> : std::true_type {
  using Type = uint32_t; ///< @type_alias
};

/**
 * 8-byte unsigned integer: `uint64_t`.
 */
template<>
struct Uint<8> : std::true_type {
  using Type = uint64_t; ///< @type_alias
};

/**
 * 16-byte unsigned integer: `uint128_t`.
 */
template<>
struct Uint<16> : std::true_type {
  using Type = uint128_t; ///< @type_alias
};

// `Float` --------------------------------------------------------------------------------------------------

template<int N> struct Float : std::false_type {};

/**
 * 4-byte floating point: `float`.
 *
 * @todo Find a portable way to express a four-byte floating point.
 */
template<>
struct Float<4> : std::true_type {
  using Type = float; ///< @type_alias
};

static_assert(sizeof(Float<4>::Type) == 4);

/**
 * 8-byte floating point: `double`.
 *
 * @todo Find a portable way to express an eight-byte floating point.
 */
template<>
struct Float<8> : std::true_type {
  using Type = double; ///< @type_alias
};

static_assert(sizeof(Float<8>::Type) == 8);

/**
 * 16-byte floating point: `long double`.
 *
 * @todo Find a portable way to express a sixteen-byte floating point.
 */
template<>
struct Float<16> : std::true_type {
  using Type = long double; ///< @type_alias
};

static_assert(sizeof(Float<16>::Type) == 16);

// Concepts -------------------------------------------------------------------------------------------------

template<typename T>
concept Character = Char<sizeof(PurgeType<T>)>::value;

template<typename T>
concept SignedInteger = Int<sizeof(PurgeType<T>)>::value;

template<typename T>
concept UnsignedInteger = Uint<sizeof(PurgeType<T>)>::value;

template<typename T>
concept Integer = SignedInteger<T> || UnsignedInteger<T>;

template<typename T>
concept FloatingPoint = Float<sizeof(PurgeType<T>)>::value;

// `LargestType` --------------------------------------------------------------------------------------------

/// The `LargestType` template.
template <typename... Ts>
struct LargestType;

/// @spec{#rocket::LargestType, `T`}
template<typename T>
struct LargestType<T> {
  using Type = T; ///< @type_alias
};

/// @spec{#rocket::LargestType, `T U Ts...`}
template<typename T, typename U, typename... Ts>
struct LargestType<T, U, Ts...> {
  using Type = typename LargestType<
      typename std::conditional<(sizeof(T) >= sizeof(U)), T, U>::type, Ts...>::Type; ///< @type_alias
};

} // namespace rocket

// EOF
