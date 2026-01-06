/**
 * @file rocket.h
 *
 * Rocket base header. Contains basic types and concepts.
 *
 * Size in bytes of basic types on all supported target systems:
 *
 * | Type          | `linux`
 * | :------------ | ------:
 * | `char`        |       1
 * | `std::byte`   |       1
 * | `short`       |       2
 * | `wchar_t`     |       4
 * | `char32_t`    |       4
 * | `int`         |       4
 * | `float`       |       4
 * | `long`        |       8
 * | `long long`   |       8
 * | `size_t`      |       8
 * | `double`      |       8
 * | `void*`       |       8
 * | `int128_t`    |      16
 * | `long double` |      16
 *
 * In Rocket, C strings of type `char*` and instances of `std::string` or `std::string_view` are assumed to
 * be UTF-8-encoded. This is already true at compile time: A string literal like `"ä"` must expand to
 * `"\xc3\xa4"`. Therefore, the `char8_t` from C++20 is never in use.
 *
 * The size of `wchar_t` is platform-dependent. On Linux, a `wchar_t` is usually 4 bytes wide, whereas on
 * Windows, it is only 2 bytes wide. So there is no guarantee a `wchar_t` can hold a Unicode code point. For
 * UTF-32 encoding, Rocket uses the `char32_t` type from C++11.
 *
 * The only Unicode encodings that Rocket supports are UTF-8 and UTF-32.
 */

#pragma once

// No Rocket includes allowed here!

#include <cmath> // `std::isnan`
#include <iosfwd>
#include <optional>

// Macros ---------------------------------------------------------------------------------------------------

#ifdef GAIA_CXX_TOOLCHAIN_GNU
  #define ROCKET_COMPILER_GNU 1
#endif

#ifdef GAIA_CXX_TOOLCHAIN_LLVM
  #define ROCKET_COMPILER_LLVM 1
#endif

#if not defined(ROCKET_COMPILER_GNU) && not defined(ROCKET_COMPILER_LLVM)
  #error Unknown compiler
#endif

// `int128_t` -----------------------------------------------------------------------------------------------

/**
 * A signed 128-bit integer type.
 *
 * - Minimum value: -170141183460469231731687303715884105728
 * - Maximum value:  170141183460469231731687303715884105727
 */
using int128_t = __int128;

/// @op_output{#int128_t}
std::ostream& operator<<(std::ostream& lhs, int128_t rhs);

/// @op_input{#int128_t}
std::istream& operator>>(std::istream& lhs, int128_t& rhs);

// `uint128_t` ----------------------------------------------------------------------------------------------

/**
 * An unsigned 128-bit integer type.
 *
 * - Maximum value: 340282366920938463463374607431768211455
 */
using uint128_t = unsigned __int128;

/// @op_input{#uint128_t}
std::istream& operator>>(std::istream& lhs, uint128_t& rhs);

/// @op_output{#uint128_t}
std::ostream& operator<<(std::ostream& lhs, uint128_t rhs);

namespace rocket {

// Constants ------------------------------------------------------------------------------------------------

/**
 * A constant for "not a position".
 */
constexpr size_t NPOS = -1L;

// Functions ------------------------------------------------------------------------------------------------

/**
 * A NOP function that helps to suppress warnings about unused variables.
 *
 * @tparam T the types of @p args
 * @param args the variables to suprress warnings for
 */
template<typename... T>
constexpr void nop(T&&... args) {}

} // namespace rocket

// EOF
