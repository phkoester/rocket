/**
 * @file rocket.h
 *
 * Rocket base header. Contains basic types and other declarations.
 *
 * Size in bytes of basic types on all supported target systems:
 *
 * | Type          | `linux` | Rocket alias
 * | :------------ | ------: | :-----------
 * | `char`        |       1 | --
 * | `wchar_t`     |       4 | --
 * | `char32_t`    |       4 | `char32`
 * | `short`       |       2 | `i16`
 * | `int`         |       4 | `i32`
 * | `long`        |       8 | `i64`
 * | `long long`   |       8 | `i64`
 * | `size_t`      |       8 | `u64`
 * | `__int128`    |      16 | `i128`
 * | `float`       |       4 | `f32`
 * | `double`      |       8 | `f64`
 * | `long double` |      16 | `f128`
 * | `void*`       |       8 | --
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

#include <cstdint> // `int8_t`, `uint8_t`, ...
#include <iosfwd>

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

// Rocket type aliases --------------------------------------------------------------------------------------

using char32 = char32_t;
using i8 = int8_t;
using u8 = uint8_t;
using i16 = int16_t;
using u16 = uint16_t;
using i32 = int;
using u32 = unsigned int;
using i64 = int64_t;
using u64 = uint64_t;
using i128 = __int128;
using u128 = unsigned __int128;
using f32 = _Float32;
using f64 = _Float64;
using f128 = long double; // @todo Find a portable way to define this
static_assert(sizeof(f128) == 16);

// `i128` ---------------------------------------------------------------------------------------------------

/// @op_input{#i128}
std::istream& operator>>(std::istream& lhs, i128& rhs);

/// @op_output{#i128}
std::ostream& operator<<(std::ostream& lhs, i128 rhs);

// `u128` ---------------------------------------------------------------------------------------------------

/// @op_input{#u128}
std::istream& operator>>(std::istream& lhs, u128& rhs);

/// @op_output{#u128}
std::ostream& operator<<(std::ostream& lhs, u128 rhs);

namespace rocket {

// Constants ------------------------------------------------------------------------------------------------

/**
 * A constant for "not a position".
 */
constexpr u64 NPOS = -1;

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
