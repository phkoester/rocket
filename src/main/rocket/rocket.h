/**
 * @file rocket.h
 *
 * Rocket base header.
 *
 * Contains very basic types and declarations.
 *
 * OS-specific data types, sizes in bytes:
 *
 * | Type         | Linux | Windows
 * | :----------- | ----: | ------:
 * | `wchar_t`    |     4 |       2
 * | `long`       |     8 |       4
 * | `__int128`   |    16 |     N/A
 * | `long double`|    16 |       8
 *
 * Basic data types used in Rocket:
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
 * be UTF-8-encoded. This is already true at compile time: A string literal like `"ä"` must expand to
 * `"\xc3\xa4"`.
 *
 * The only Unicode encodings that Rocket supports are UTF-8 and UTF-32.
 */

#pragma once

#include <bit>
#include <cstdint> // `std::int8_t`, `std::uint8_t`, ...
#include <cstdio> // Make this generally availabe
#include <iosfwd>
#include <typeinfo> // Make this generally available

#ifdef ROCKET_OS_WINDOWS
#include <boost/int128.hpp>
#endif

// Check prerequisites --------------------------------------------------------------------------------------

#if not defined(ROCKET_OS_LINUX) && not defined(ROCKET_OS_WINDOWS)
  #error Unsupported OS
#endif

#if not defined(ROCKET_CXX_COMPILER_GNU) && \
    not defined(ROCKET_CXX_COMPILER_CLANG) && \
    not defined(ROCKET_CXX_COMPILER_MSVC)
  #error Unsupported compiler
#endif

// Detect endianness ----------------------------------------------------------------------------------------

// Mixed/middle endian is not supported
static_assert(
  std::endian::native == std::endian::little || std::endian::native == std::endian::big,
  "Only little-endian and big-endian architectures are supported");

/// Whether the native architecture is little-endian.
constexpr bool HAS_LITTLE_ENDIAN = std::endian::native == std::endian::little;

// Macros ---------------------------------------------------------------------------------------------------

#ifdef ROCKET_OS_WINDOWS
  #ifdef ROCKET_EXPORTING__
    #define ROCKET_PUBLIC __declspec(dllexport) ///< Specifier for global data symbols.
  #else
    #define ROCKET_PUBLIC  __declspec(dllimport) ///< Specifier for global data symbols.
  #endif
#else
  #define ROCKET_PUBLIC ///< Specifier for global data symbols.
#endif

// `std::type_info` for MSVC --------------------------------------------------------------------------------

#ifdef ROCKET_OS_WINDOWS

namespace std {

/// Windows `type_info` in the global namespace, so we need to alias it here.
using type_info = ::type_info;

} // namespace std

#endif // ROCKET_OS_WINDOWS

// Rocket type aliases --------------------------------------------------------------------------------------

/// @cond undocumented
using std_wchar_t = wchar_t;
using std_char32_t = char32_t;
using std_short = short;
using std_int = int;
using std_unsigned = unsigned;
using std_long = long;
using std_unsigned_long_long_int = unsigned long long int;
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
#ifdef ROCKET_OS_WINDOWS
  using i128 = boost::int128_t; ///< A signed 128-bit integer.
  using u128 = boost::uint128_t; ///< An unsigned 128-bit integer.
  #ifdef ROCKET_CXX_COMPILER_CLANG
    static_assert(std::is_same_v<i128, __int128>);
    static_assert(std::is_same_v<u128, unsigned __int128>);
  #endif
#else
  using i128 = __int128; ///< A signed 128-bit integer.
  using u128 = unsigned __int128; ///< An unsigned 128-bit integer.
#endif
using f32 = std_float; ///< A 32-bit floating point.
using f64 = std_double; ///< A 64-bit floating point.

// I/O stream support for 128-bit data types ----------------------------------------------------------------

/// @op_input{#i128}
std::istream& operator>>(std::istream& lhs, i128& rhs);

/// @op_output{#i128}
std::ostream& operator<<(std::ostream& lhs, i128 rhs);

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

} // namespace rocket

// EOF
