/**
 * @file rocket.h
 *
 * Rocket base header.
 *
 * Contains very basic types and declarations.
 *
 * OS-specific data types, sizes in bytes:
 *
 * | Type          | Linux | macOS   | Windows | Different?
 * | :------------ | ----: | ------: | ------: | :---------
 * | `bool`        |     1 |       1 |       1 |
 * | `wchar_t`     |     4 |       4 |       2 | Yes!
 * | `short`       |     2 |       2 |       2 |
 * | `int`         |     4 |       4 |       4 |
 * | `long`        |     8 |       8 |       4 | Yes!
 * | `long long`   |     8 |       8 |       8 |
 * | `i128`        |    16 |      16 |      16 |
 * | `float`       |     4 |       4 |       4 |
 * | `double`      |     8 |       8 |       8 |
 * | `long double` |    16 |       8 |       8 | Yes!
 * | `void*`       |     8 |       8 |       8 |
 *
 * Basic data types used in Rocket, consistent across all platforms:
 *
 * | Type             | Size
 * | :--------------- | ---:
 * | `bool`           |    1
 * | `char`           |    1
 * | `rocket::char32` |    4
 * | `rocket::i8`     |    1
 * | `rocket::u8`     |    1
 * | `rocket::i16`    |    2
 * | `rocket::u16`    |    2
 * | `rocket::i32`    |    4
 * | `rocket::u32`    |    4
 * | `rocket::i64`    |    8
 * | `rocket::u64`    |    8
 * | `rocket::i128`   |   16
 * | `rocket::u128`   |   16
 * | `rocket::f32`    |    4
 * | `rocket::f64`    |    8
 * | `void*`          |    8
 *
 * In Rocket, C strings of type `char*` and instances of #std::string or #std::string_view are assumed to
 * be UTF-8-encoded. This is already true at compile time: a string literal like `"ä"` must expand to
 * `"\xc3\xa4"`.
 *
 * The only Unicode encodings that Rocket supports are UTF-8 and UTF-32.
 */

#pragma once

#include "rocket/int128.h"

#include <bit> // `std::endian`
#include <cstddef> // `size_t`
#include <cstdint> // `std::int8_t`, `std::uint8_t`, ...
#include <concepts> // `std::same_as`
#include <limits>

// Check operating system and C/C++ compiler ----------------------------------------------------------------

#if not defined(ROCKET_OS_LINUX) && not defined(ROCKET_OS_MACOS) && not defined(ROCKET_OS_WINDOWS)
  #error Unknown OS
#endif

#if not defined(ROCKET_C_COMPILER_GNU) && \
  not defined(ROCKET_C_COMPILER_LLVM) && \
  not defined(ROCKET_C_COMPILER_MSVC)
  #error Unknown C compiler
#endif

#if not defined(ROCKET_CXX_COMPILER_GNU) && \
  not defined(ROCKET_CXX_COMPILER_LLVM) && \
  not defined(ROCKET_CXX_COMPILER_MSVC)
  #error Unknown C++ compiler
#endif

// Macros ---------------------------------------------------------------------------------------------------

#if defined(ROCKET_OS_WINDOWS) and defined(ROCKET_SHARED_LIBS)
  #ifdef ROCKET_EXPORTING__
    #define ROCKET_PUBLIC __declspec(dllexport) ///< Specifier for global data symbols.
  #else
    #define ROCKET_PUBLIC __declspec(dllimport) ///< Specifier for global data symbols.
  #endif
#else
  #define ROCKET_PUBLIC ///< Specifier for global data symbols.
#endif

namespace rocket {

// Basic Rocket types ---------------------------------------------------------------------------------------

/// @cond undocumented
using std_wchar_t = wchar_t;
using std_char32_t = char32_t;
using std_short = short;
using std_int = int;
using std_unsigned = unsigned;
using std_long = long;
using std_unsigned_long_long = unsigned long long;
using std_size_t = size_t;
using std_float = float;
using std_double = double;
using std_long_double = long double;
/// @endcond

using char32 = std_char32_t; ///< An unsigned 32-bit character.
using i8 = std::int8_t; ///< A signed 8-bit integer.
using u8 = std::uint8_t; ///< An unsigned 8-bit integer.
using i16 = std::int16_t; ///< A signed 16-bit integer.
using u16 = std::uint16_t; ///< An unsigned 16-bit integer.
using i32 = std::int32_t; ///< A signed 32-bit integer.
using u32 = std::uint32_t; ///< An unsigned 32-bit integer.
using i64 = std::int64_t; ///< A signed 64-bit integer.
using u64 = std::uint64_t; ///< An unsigned 64-bit integer.
using i128 = int128__; ///< A signed 128-bit integer.
using u128 = uint128__; ///< An unsigned 128-bit integer.
using f32 = std_float; ///< A 32-bit floating point.
using f64 = std_double; ///< A 64-bit floating point.

// Check the types ------------------------------------------------------------------------------------------

static_assert(sizeof(bool) == 1);
static_assert(sizeof(char) == 1);
static_assert(std::is_signed_v<char>);
static_assert(sizeof(char32) == 4);
static_assert(not std::numeric_limits<char32>::is_signed);
static_assert(sizeof(i8) == 1);
static_assert(sizeof(u8) == 1);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(i64) == 8);
static_assert(sizeof(u64) == 8);
static_assert(sizeof(u64) == sizeof(std_size_t));
static_assert(sizeof(i128) == 16);
static_assert(std::numeric_limits<i128>::is_signed);
static_assert(sizeof(u128) == 16);
static_assert(not std::numeric_limits<u128>::is_signed);
static_assert(sizeof(f32) == 4);
static_assert(std::same_as<decltype(1.0F), f32>);
static_assert(sizeof(f64) == 8);
static_assert(std::same_as<decltype(1.0), f64>);
static_assert(sizeof(void*) == 8);

// Detect endianness ----------------------------------------------------------------------------------------

/// Whether the native host architecture is little endian.
constexpr bool HAS_LITTLE_ENDIAN = std::endian::native == std::endian::little;
/// Whether the native host architecture is big endian.
constexpr bool HAS_BIG_ENDIAN = std::endian::native == std::endian::big;

// Mixed/middle endian is not supported
static_assert(
  HAS_LITTLE_ENDIAN || HAS_BIG_ENDIAN,
  "Only little-endian and big-endian architectures are supported");

// Constants ------------------------------------------------------------------------------------------------

/// A constant for "not a position".
constexpr u64 NPOS = -1;

} // namespace rocket

// EOF
