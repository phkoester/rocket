/**
 * @file int128.h
 *
 * 128-bit integer support.
 */

#pragma once

#include <iosfwd>

namespace rocket {

#if defined(ROCKET_OS_WINDOWS) && not defined(ROCKET_CXX_COMPILER_CLANG)
  #define ROCKET_HAS_BOOST_INT128

  // With Boost 1.93, this will be `<boost/int128.hpp>`
  #include <boost/multiprecision/cpp_int.hpp>

  using int128 = boost::multiprecision::int128_t;
  using uint128 = boost::multiprecision::uint128_t;
#else
  using int128 = __int128;
  using uint128 = unsigned __int128;
#endif

} // namespace rocket

// I/O stream support for 128-bit data types ----------------------------------------------------------------

/// @op_input{#i128}
std::istream& operator>>(std::istream& lhs, rocket::int128& rhs);

/// @op_output{#i128}
std::ostream& operator<<(std::ostream& lhs, rocket::int128 rhs);

/// @op_input{#u128}
std::istream& operator>>(std::istream& lhs, rocket::uint128& rhs);

/// @op_output{#u128}
std::ostream& operator<<(std::ostream& lhs, rocket::uint128 rhs);

// EOF
