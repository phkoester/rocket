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
 * Windows, it is only 2 bytes wide. So there is no guarantee a `wchar_t` can hold any Unicode code point.
 * For UTF-32 encoding, Rocket uses the `char32_t` type from C++11.
 *
 * The only Unicode encodings that Rocket supports are UTF-8 and UTF-32.
 */

#pragma once

// No Rocket includes allowed here!

#include <cstdint> // `int8_t`, `uint8_t`, ...
#include <iosfwd>

// Check prerequisites --------------------------------------------------------------------------------------

#if not defined(ROCKET_OS_LINUX) && not defined(ROCKET_OS_WINDOWS)
  #error Unsupported OS
#endif

#if not defined(ROCKET_CXX_COMPILER_GNU) && \
    not defined(ROCKET_CXX_COMPILER_CLANG) && \
    not defined(ROCKET_CXX_COMPILER_MSVC)
  #error Unsupported compiler
#endif

// Rocket type aliases --------------------------------------------------------------------------------------

/// @cond undocumented

using std_char32_t = char32_t;
using std_float = float;
using std_double = double;
using std_long = long;
using std_long_double = long double;
using std_size_t = size_t;
using std_wchar_t = wchar_t;

/// @endcond

/// An unsigned 32-bit character.
using char32 = std_char32_t;
/// A signed 8-bit integer.

using i8 = int8_t;
/// An unsigned 8-bit integer.
using u8 = uint8_t;
/// A signed 16-bit integer.
using i16 = int16_t;
/// An unsigned 16-bit integer.
using u16 = uint16_t;
/// A signed 32-bit integer.
using i32 = int32_t;
/// An unsigned 32-bit integer.
using u32 = uint32_t;
/// A signed 64-bit integer.
using i64 = int64_t;
/// An unsigned 64-bit integer.
using u64 = uint64_t;
/// A signed 128-bit integer.
using i128 = __int128;
/// An unsigned 128-bit integer.
using u128 = unsigned __int128;

/**
 * A 32-bit floating point.
 *
 * @todo Find a portable way to define this.
 */
using f32 = std_float;
static_assert(sizeof(f32) == 4);
/**
 * A 64-bit floating point.
 *
 * @todo Find a portable way to define this.
 */
using f64 = std_double;
static_assert(sizeof(f64) == 8);
/**
 * A 128-bit floating point.
 *
 * @todo Find a portable way to define this.
 */
using f128 = std_long_double;
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
 * @tparam T... the types of @p args
 */
template<typename... T>
constexpr void nop(T&&...) {}

} // namespace rocket

// EOF
