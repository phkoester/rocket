/**
 * @file functional.h
 *
 * Functional utilities.
 */

#pragma once

#include "rocket/rocket.h"

#include <boost/functional/hash.hpp>

#include <functional>

namespace rocket {

// #BoostHash -----------------------------------------------------------------------------------------------

/**
 * A hasher using `boost::hash`.
 */
struct BoostHash {
  /// @cond undocumented
  template<typename T>
  [[nodiscard]] u64
  operator()(const T& val) const {
    return boost::hash<T>()(val);
  }
  /// @endcond
};

// #StdEqualTo ----------------------------------------------------------------------------------------------

/**
 * A comparator using #std::equal_to.
 */
 struct StdEqualTo {
  /// @cond undocumented
  template<typename T>
  bool operator()(const T& lhs, const T& rhs) const {
    return std::equal_to<T>()(lhs, rhs);
  }
  /// @endcond
};

// #StdHash -------------------------------------------------------------------------------------------------

/**
 * A hasher using #std::hash.
 */
struct StdHash {
  /// @cond undocumented
  template<typename T>
  [[nodiscard]] u64
  operator()(const T& val) const {
    return std::hash<T>()(val);
  }
  /// @endcond
};

} // namespace rocket

// EOF
