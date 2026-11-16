/**
 * @file int128.h
 *
 * Provides 128-bit integer types.
 */

#pragma once

#include "rocket/configure.h"

// Boost.Int128 defines `std::numeric_limits`, so include `<limits>` here for the sake of symmetry
#include <limits>

#ifdef ROCKET_HAS_BOOST_INT128
  #include <boost/int128/int128.hpp>
#endif

namespace rocket {

// `i128` and `u128` ----------------------------------------------------------------------------------------

#ifdef ROCKET_HAS_INT128
using i128 = __int128; ///< A signed 128-bit integer type.
using u128 = unsigned __int128; ///< An unsigned 128-bit integer type.
#endif

#ifdef ROCKET_HAS_BOOST_INT128
using i128 = boost::int128::int128; ///< A signed 128-bit integer type.
using u128 = boost::int128::uint128; ///< An unsigned 128-bit integer type.
#endif

} // namespace rocket

// EOF
