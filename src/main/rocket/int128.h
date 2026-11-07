/**
 * @file int128.h
 *
 * Provides 128-bit integer types.
 */

#pragma once

#if defined(ROCKET_OS_WINDOWS)
  // Windows: Boost.Int128
  #define ROCKET_HAS_BOOST_INT128
  #define ROCKET_HAS_INT128_LIMITS
  #define ROCKET_HAS_INT128_STD_HASH
#else
  #ifndef __SIZEOF_INT128__
    #error This C++ version does not support 128-bit integers
  #endif
  #define ROCKET_HAS_INT128
  #if defined(ROCKET_LINUX) || defined(ROCKET_MACOS)
    // Linux, macOS: native __i128
    #define ROCKET_HAS_INT128_FMT_FORMATTER
    #define ROCKET_HAS_INT128_LIMITS
    #define ROCKET_HAS_INT128_SCN_SCANNER
    #define ROCKET_HAS_INT128_STD_HASH
  #endif
#endif // ROCKET_OS_WINDOWS

#include <limits>

#ifndef ROCKET_HAS_INT128
  #include <boost/int128/int128.hpp>
#endif

namespace rocket {

// `int128__` and `uint128__` -------------------------------------------------------------------------------

#ifdef ROCKET_HAS_INT128

using i128 = __int128;
using u128 = unsigned __int128;

#else

using i128 = boost::int128::int128;
using u128 = boost::int128::uint128;

#endif // ROCKET_HAS_INT128

} // namespace rocket

#ifndef ROCKET_HAS_INT128_LIMITS

// `std::numeric_limits<rocket::i128>` ----------------------------------------------------------------------

template<>
struct std::numeric_limits<rocket::i128> {
  static constexpr bool is_signed = true;

  static constexpr rocket::i128 max() noexcept { return ~(rocket::i128(1) << 127); }

  static constexpr rocket::i128 min() noexcept { return rocket::i128(1) << 127; }
};

// `std::numeric_limits<rocket::u128>` ----------------------------------------------------------------------

template<>
struct std::numeric_limits<rocket::u128> {
  static constexpr bool is_signed = false;

  static constexpr rocket::u128 max() noexcept { return ~(rocket::u128(0)); }

  static constexpr rocket::u128 min() noexcept { return rocket::u128(0); }
};

#endif // ROCKET_HAS_INT128_LIMITS

// EOF
