/**
 * @file type-traits.h
 *
 * Template magic for types.
 */

#pragma once

#include "rocket/rocket.h"

#include <optional>
#include <type_traits>
#include <vector>
namespace rocket {

// #Purge ---------------------------------------------------------------------------------------------------

/// An alias for #std::remove_cvref.
template<typename T>
using Purge = std::remove_cvref<T>;

/// An alias for `Purge<T>::type`.
template<typename T>
using PurgeType = Purge<T>::type;

static_assert(std::is_same_v<PurgeType<const volatile i32>, i32>);
static_assert(std::is_same_v<PurgeType<const std::true_type&>, std::true_type>);

// #Char ----------------------------------------------------------------------------------------------------

template<u32 N> struct Char;

/**
 * 1-byte character: `char`.
 */
template<>
struct Char<1> {
  using Type = char; ///< @type_alias
};

/**
 * 4-byte character: `char32`.
 */
template<>
struct Char<4> {
  using Type = char32; ///< @type_alias
};

// #Int -----------------------------------------------------------------------------------------------------

template<u32 N> struct Int;

/**
 * 1-byte signed integer: #i8.
 */
template<>
struct Int<1> {
  using Type = i8; ///< @type_alias
};

/**
 * 2-byte signed integer: `i16`.
 */
template<>
struct Int<2> {
  using Type = i16; ///< @type_alias
};

/**
 * 4-byte signed integer: `i32`.
 */
template<>
struct Int<4> {
  using Type = i32; ///< @type_alias
};

/**
 * 8-byte signed integer: `i64`.
 */
template<>
struct Int<8> {
  using Type = i64; ///< @type_alias
};

#ifdef ROCKET_HAS_128
/**
 * 16-byte signed integer: `i128`.
 */
template<>
struct Int<16> {
  using Type = i128; ///< @type_alias
};
#endif

// #Uint ----------------------------------------------------------------------------------------------------

template<u32 N> struct Uint;

/**
 * 1-byte unsigned integer: `u8`.
 */
template<>
struct Uint<1> {
  using Type = u8; ///< @type_alias
};

/**
 * 2-byte unsigned integer: `u16`.
 */
template<>
struct Uint<2> {
  using Type = u16; ///< @type_alias
};

/**
 * 4-byte unsigned integer: `u32`.
 */
template<>
struct Uint<4> {
  using Type = u32; ///< @type_alias
};

/**
 * 8-byte unsigned integer: `u64`.
 */
template<>
struct Uint<8> {
  using Type = u64; ///< @type_alias
};

#ifdef ROCKET_HAS_128
/**
 * 16-byte unsigned integer: `u128`.
 */
template<>
struct Uint<16> {
  using Type = u128; ///< @type_alias
};
#endif

// #Float ---------------------------------------------------------------------------------------------------

template<u32 N> struct Float;

/**
 * 4-byte floating point: `f32`.
 */
template<>
struct Float<4> {
  using Type = f32; ///< @type_alias
};

/**
 * 8-byte floating point: `f64`.
 */
template<>
struct Float<8> {
  using Type = f64; ///< @type_alias
};

#ifdef ROCKET_HAS_128
/**
 * 16-byte floating point: `f128`.
 */
template<>
struct Float<16> {
  using Type = f128; ///< @type_alias
};
#endif

// Concepts for basic data types ----------------------------------------------------------------------------

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

// #IsOptional ----------------------------------------------------------------------------------------------


template<typename T>
struct Optional : std::false_type {};

template<typename T>
struct Optional<std::optional<T>> : std::true_type {};

template<typename T>
concept IsOptional = Optional<T>::value;

// #IsVector ------------------------------------------------------------------------------------------------

template<typename T>
struct Vector : std::false_type {};

template<typename T>
struct Vector<std::vector<T>> : std::true_type {};

template<typename T>
concept IsVector = Vector<T>::value;

// #LargestType ---------------------------------------------------------------------------------------------

/**
 * The #rocket::LargestType template.
 *
 * Given a list of types, this template figures out the largest type.
 *
 * ## Examples
 *
 * ```
 * static_assert(std::is_same_v<LargestType<char, i32>::Type, i32>);
 * ```
 */
template <typename... Ts>
struct LargestType;

/// @spec{#rocket::LargestType, T}
template<typename T>
struct LargestType<T> {
  using Type = T; ///< @type_alias
};

/// @spec{#rocket::LargestType, ...}
template<typename T, typename U, typename... Ts>
struct LargestType<T, U, Ts...> {
  using Type = typename LargestType<
      typename std::conditional_t<(sizeof(T) >= sizeof(U)), T, U>, Ts...>::Type; ///< @type_alias
};

} // namespace rocket

// EOF
