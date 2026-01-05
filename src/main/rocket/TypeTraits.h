/**
 * @file TypeTraits.h
 *
 * Miscellaneous template magic.
 */

#pragma once

#include <type_traits>

namespace rocket {

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
