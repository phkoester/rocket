/**
 * @file configure.h
 *
 * Detect C++ configuration, define macros describing what is available and what not.
 */

#pragma once

#include <version>

#ifdef ROCKET_OS_LINUX
// Linux ----------------------------------------------------------------------------------------------------

// 128-bit integers
#ifndef __SIZEOF_INT128__
  #error This C++ version does not support 128-bit integers
#endif
#define ROCKET_HAS_INT128
#define ROCKET_HAS_FMT_FORMATTER_INT128
#define ROCKET_HAS_SCN_SCANNER_INT128

// `std::chrono::time_zone`
#if __cpp_lib_chrono < 201907L
  #error This C++ version does not support `std::chrono` time zones
#endif
#define ROCKET_HAS_STD_CHRONO_TIME_ZONE
#define ROCKET_HAS_FMT_FORMATTER_STD_CHRONO_YEAR_MONTH_DAY
#define ROCKET_HAS_SCN_SCANNER_STD_CHRONO_YEAR_MONTH_DAY

#elifdef ROCKET_OS_MACOS
// macOS ----------------------------------------------------------------------------------------------------

#ifndef __SIZEOF_INT128__
  #error This C++ version does not support 128-bit integers
#endif
#define ROCKET_HAS_INT128
#define ROCKET_HAS_FMT_FORMATTER_INT128
#define ROCKET_HAS_SCN_SCANNER_INT128

// With the LLVM toolchain, there is no support for `std::chrono::time_zone` in the standard library. With
// the GNU toolchain, there is
#ifdef ROCKET_CXX_COMPILER_LLVM
  // macOS, LLVM
  #if __cpp_lib_chrono >= 201907L
    #error This C++ version supports `std::chrono` time zones
  #endif
#else
  // macOS, GNU
  #if __cpp_lib_chrono < 201907L
    #error This C++ version does not support `std::chrono` time zones
  #endif
  #define ROCKET_HAS_STD_CHRONO_TIME_ZONE
  #define ROCKET_HAS_FMT_FORMATTER_STD_CHRONO_YEAR_MONTH_DAY
  #define ROCKET_HAS_SCN_SCANNER_STD_CHRONO_YEAR_MONTH_DAY
#endif

#else
// Windows --------------------------------------------------------------------------------------------------

// Do not check for `__SIZEOF_INT128__` here, use Boost.Int128 at any rate. Even though `clang-cl` does have
// a builtin `__int128` type, it will not work with {fmt}
#define ROCKET_HAS_BOOST_INT128

#if __cpp_lib_chrono < 201907L
  #error This C++ version does not support `std::chrono` time zones
#endif
#define ROCKET_HAS_STD_CHRONO_TIME_ZONE
#define ROCKET_HAS_FMT_FORMATTER_STD_CHRONO_YEAR_MONTH_DAY
#define ROCKET_HAS_SCN_SCANNER_STD_CHRONO_YEAR_MONTH_DAY

#endif // ROCKET_OS_LINUX

// EOF
