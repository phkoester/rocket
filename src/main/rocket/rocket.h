/**
 * @file rocket.h
 *
 * Rocket base header.
 *
 * Contains very basic types and declarations.
 *
 * Basic data types used in Rocket:
 *
 * | Type             | Size in bytes | Availability
 * | :--------------- | ------------: | ------------
 * | `bool`           |             1 | Always
 * | `char`           |             1 | Always
 * | `rocket::char32` |             4 | Always
 * | `rocket::i8`     |             1 | Always
 * | `rocket::u8`     |             1 | Always
 * | `rocket::i16`    |             2 | Always
 * | `rocket::u16`    |             2 | Always
 * | `rocket::i32`    |             4 | Always
 * | `rocket::u32`    |             4 | Always
 * | `rocket::i64`    |             8 | Always
 * | `rocket::u64`    |             8 | Always
 * | `rocket::i128`   |            16 | Not with MSVC
 * | `rocket::u128`   |            16 | Not with MSVC
 * | `rocket::f32`    |             4 | Always
 * | `rocket::f64`    |             8 | Always
 * | `rocket::f128`   |            16 | Not with MSVC
 * | `void*`          |             8 | Always
 *
 * In Rocket, C strings of type `char*` and instances of `std::string` or `std::string_view` are assumed to
 * be UTF-8-encoded. This is already true at compile time: A string literal like `"ä"` must expand to
 * `"\xc3\xa4"`.
 *
 * The only Unicode encodings that Rocket supports are UTF-8 and UTF-32.
 */

#pragma once

#include <cstdint> // `int8_t`, `uint8_t`, ...
#include <cstdio> // Make this generally availabe
#include <iosfwd>
#include <typeinfo> // Make this generally available

// Check prerequisites --------------------------------------------------------------------------------------

#if not defined(ROCKET_OS_LINUX) && not defined(ROCKET_OS_WINDOWS)
  #error Unsupported OS
#endif

#if not defined(ROCKET_CXX_COMPILER_GNU) && \
    not defined(ROCKET_CXX_COMPILER_CLANG) && \
    not defined(ROCKET_CXX_COMPILER_MSVC)
  #error Unsupported compiler
#endif

#ifndef ROCKET_CXX_COMPILER_MSVC
  #define ROCKET_HAS_128 ///< Do we have 128-bit data types?
#endif

// Macros ---------------------------------------------------------------------------------------------------

#ifdef ROCKET_OS_WINDOWS
#define ROCKET_EXPORT __declspec(dllimport)
#else
#define ROCKET_EXPORT
#endif

#ifdef ROCKET_CXX_COMPILER_MSVC

#define STDIN_FILENO  0 ///< Standard input file number.
#define STDOUT_FILENO 1 ///< Standard output file number.
#define STDERR_FILENO 2 ///< Standard error file number.

#endif // ROCKET_CXX_COMPILER_MSVC

// `type_info` for MSVC -------------------------------------------------------------------------------------

#ifdef ROCKET_CXX_COMPILER_MSVC

namespace std {

/// Microsoft has `type_info` in the global namespace, so we need to alias it here.
using type_info = ::type_info;

} // namespace std

#endif // ROCKET_CXX_COMPILER_MSVC

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

using char32 = std_char32_t; ///< An unsigned 32-bit character.
using i8 = int8_t; ///< A signed 8-bit integer.
using u8 = uint8_t; ///< An unsigned 8-bit integer.
using i16 = int16_t; ///< A signed 16-bit integer.
using u16 = uint16_t; ///< An unsigned 16-bit integer.
using i32 = int32_t; ///< A signed 32-bit integer.
using u32 = uint32_t; ///< An unsigned 32-bit integer.
using i64 = int64_t; ///< A signed 64-bit integer.
using u64 = uint64_t; ///< An unsigned 64-bit integer.
#ifdef ROCKET_HAS_128
using i128 = __int128; ///< A signed 128-bit integer.
using u128 = unsigned __int128; ///< An unsigned 128-bit integer.
#endif
using f32 = std_float; ///< A 32-bit floating point.
using f64 = std_double; ///< A 64-bit floating point.
#ifdef ROCKET_HAS_128
using f128 = std_long_double; ///< A 128-bit floating point.
#endif

static_assert(sizeof(bool) == 1);
static_assert(sizeof(char) == 1);
static_assert(sizeof(char32) == 4);
static_assert(sizeof(i8) == 1);
static_assert(sizeof(u8) == 1);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(i64) == 8);
static_assert(sizeof(u64) == 8);
#ifdef ROCKET_HAS_128
static_assert(sizeof(i128) == 16);
static_assert(sizeof(i128) == 16);
#endif
static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);
#ifdef ROCKET_HAS_128
static_assert(sizeof(f128) == 16);
#endif
static_assert(sizeof(void*) == 8);

// I/O stream support for 128-bit data types ----------------------------------------------------------------

#ifdef ROCKET_HAS_128

/// @op_input{#i128}
std::istream& operator>>(std::istream& lhs, i128& rhs);

/// @op_output{#i128}
std::ostream& operator<<(std::ostream& lhs, i128 rhs);

/// @op_input{#u128}
std::istream& operator>>(std::istream& lhs, u128& rhs);

/// @op_output{#u128}
std::ostream& operator<<(std::ostream& lhs, u128 rhs);

#endif // ROCKET_HAS_128

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
 * @tparam T... the types of the arguments
 * @param ... the arguments to be ignored
 */
template<typename... T>
constexpr void nop(T&&...) {}

} // namespace rocket

// EOF
