/**
 * @file type-traits.h
 *
 * Compile-time type information, template magic.
 */

#pragma once

#include "rocket/rocket.h"

#include <array>
#include <forward_list>
#include <optional>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

namespace rocket {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

template<typename T>
struct IsArrayImpl : std::false_type {};

template<typename T, u64 N>
struct IsArrayImpl<std::array<T, N>> : std::true_type {};

template<typename T>
struct IsForwardListImpl : std::false_type {};

template<typename T, typename Alloc>
struct IsForwardListImpl<std::forward_list<T, Alloc>> : std::true_type {};

template<typename T>
struct IsOptionalImpl : std::false_type {};

template<typename T>
struct IsOptionalImpl<std::optional<T>> : std::true_type {};

template<typename T, typename = void>
struct IsUnorderedImpl : std::false_type {};

template<typename T>
struct IsUnorderedImpl<T, std::void_t<typename T::hasher>> : std::true_type {};

template<typename T>
struct IsVectorImpl : std::false_type {};

template<typename T, typename Alloc>
struct IsVectorImpl<std::vector<T, Alloc>> : std::true_type {};

template<typename T>
struct IsViewImpl : std::false_type {};

template<typename T, u64 Extent>
struct IsViewImpl<std::span<T, Extent>> : std::true_type {};

template<typename C, typename Traits>
struct IsViewImpl<std::basic_string_view<C, Traits>> : std::true_type {};

template<typename... T>
struct LargestImpl;

template<typename T>
struct LargestImpl<T> {
  using Type = T;
};

template<typename T, typename U, typename... Ts>
struct LargestImpl<T, U, Ts...> {
  using Type = LargestImpl<typename std::conditional_t<(sizeof(T) >= sizeof(U)), T, U>, Ts...>::Type;
};

template<typename T>
using PurgeImpl = std::remove_cvref<T>;

template<typename T>
struct ViewImpl {
  using Type = T;
};

template<typename T, u64 N>
struct ViewImpl<std::array<T, N>> {
  using Type = std::span<const T>;
};

template<typename C, typename Traits, typename Alloc>
struct ViewImpl<std::basic_string<C, Traits, Alloc>> {
  using Type = std::basic_string_view<C, Traits>;
};

template<typename T, u64 Extent>
struct ViewImpl<std::span<T, Extent>> {
  using Type = std::span<const T, Extent>;
};

template<typename T, typename Alloc>
struct ViewImpl<std::vector<T, Alloc>> {
  using Type = std::span<const T>;
};

} // namespace internal

// `Ordering`, `CommonOrdering` -----------------------------------------------------------------------------

/**
 * Yields the ordering of the given type @p T.
 *
 * @tparam T the type to get the ordering of
 */
template<typename T>
using Ordering = decltype(std::declval<T>() <=> std::declval<T>());

/**
 * Yields the common ordering of the given types @p T.
 *
 * @tparam T the types to get the common ordering of
 */
template<typename... T>
using CommonOrdering = Ordering<std::tuple<T...>>;

// `Largest` ------------------------------------------------------------------------------------------------

/**
 * Yields the largest type from the given types @p T.
 *
 * @tparam T the types to compare
 */
template<typename... T>
using Largest = typename internal::LargestImpl<T...>::Type; ///< @type_alias

// `Purge` --------------------------------------------------------------------------------------------------

/**
 * Removes `const`, `volatile`, and reference from the type @p T.
 *
 * @tparam T the type to purge
 */
template<typename T>
using Purge = internal::PurgeImpl<T>::type;

// `View` ---------------------------------------------------------------------------------------------------

/**
 * Yields a view type for the given type @p T, or @p T itself if there is no defined view type.
 *
 * @tparam T the type to get a view type for
 */
template<typename T>
using View = typename internal::ViewImpl<T>::Type;

// `Char` ---------------------------------------------------------------------------------------------------

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

// `Int` ----------------------------------------------------------------------------------------------------

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

// `Uint` ---------------------------------------------------------------------------------------------------

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

// `Float` --------------------------------------------------------------------------------------------------

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
concept IsChar = std::is_same_v<Purge<T>, typename Char<sizeof(Purge<T>)>::Type>;

template<typename T>
concept IsInt = std::is_same_v<Purge<T>, typename Int<sizeof(Purge<T>)>::Type>;

template<typename T>
concept IsUint = std::is_same_v<Purge<T>, typename Uint<sizeof(Purge<T>)>::Type>;

template<typename T>
concept IsInteger = IsInt<T> || IsUint<T>;

template<typename T>
concept IsFloat = std::is_same_v<Purge<T>, typename Float<sizeof(Purge<T>)>::Type>;

// Miscellaneous concepts -----------------------------------------------------------------------------------

template<typename T> concept IsArray = internal::IsArrayImpl<T>::value;

template<typename T> concept IsForwardList = internal::IsForwardListImpl<T>::value;

template<typename T> concept IsOptional = internal::IsOptionalImpl<T>::value;

template<typename T> concept IsUnordered = internal::IsUnorderedImpl<T>::value;

template<typename T> concept IsVector = internal::IsVectorImpl<T>::value;

template<typename T> concept IsView = internal::IsViewImpl<T>::value;

} // namespace rocket

// EOF
