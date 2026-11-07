/**
 * @file int128.h
 *
 * Provides 128-bit integer types.
 * The only Unicode encodings that Rocket supports are UTF-8 and UTF-32.
 */

#pragma once

#ifdef ROCKET_OS_WINDOWS
  #ifdef __SIZEOF_INT128__
    #error This C++ version supports 128-bit integers
  #endif
#else
  #if __SIZEOF_INT128__ != 16
    #error This C++ version does not support 128-bit integers
  #endif
  #define ROCKET_HAS_INT128
#endif

#ifndef ROCKET_HAS_INT128
  #include <boost/int128/int128.hpp>
#endif

// `int128__` and `uint128__` ------------------------------------------------------------------------------

#ifdef ROCKET_HAS_INT128

using int128__ = __int128;
using uint128__ = unsigned __int128;

#else

using int128__ = boost::int128::int128;
using uint128__ = boost::int128::uint128;

#endif // ROCKET_HAS_INT128

// EOF
