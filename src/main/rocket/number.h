/**
 * @file number.h
 *
 * Utilities around numbers.
 */

#pragma once

#include "Type.h"

#include <limits>
#include <stdexcept>
#include <type_traits>

namespace rocket {

// Functions ------------------------------------------------------------------------------------------------

template<typename Result, typename Control, typename Lhs, typename Rhs>
Result
add(Lhs lhs, Rhs rhs) {
  static_assert(std::is_signed_v<Control>);
  static_assert(sizeof(Control) >= 2 * sizeof(Lhs));
  static_assert(sizeof(Control) >= 2 * sizeof(Rhs));

  Control controlRet = static_cast<Control>(lhs) + static_cast<Control>(rhs);
  constexpr Control min = std::numeric_limits<Result>::min();
  constexpr Control max = std::numeric_limits<Result>::max();
  if (controlRet < min || controlRet > max) {
    throw std::overflow_error(fmt::format("`{}` overflow: {} + {}", Type::of<Result>(), lhs, rhs));
  }
  return static_cast<Result>(controlRet);
}

template<typename Result, typename Control, typename Lhs, typename Rhs>
Result
sub(Lhs lhs, Rhs rhs) {
  static_assert(std::is_signed_v<Control>);
  static_assert(sizeof(Control) >= 2 * sizeof(Lhs));
  static_assert(sizeof(Control) >= 2 * sizeof(Rhs));

  Control controlRet = static_cast<Control>(lhs) - static_cast<Control>(rhs);
  constexpr Control min = std::numeric_limits<Result>::min();
  constexpr Control max = std::numeric_limits<Result>::max();
  if (controlRet < min || controlRet > max) {
    throw std::overflow_error(fmt::format("`{}` overflow: {} - {}", Type::of<Result>(), lhs, rhs));
  }
  return static_cast<Result>(controlRet);
}

template<typename Result, typename Control, typename Lhs, typename Rhs>
std::optional<Result>
tryAdd(Lhs lhs, Rhs rhs) {
  static_assert(std::is_signed_v<Control>);
  static_assert(sizeof(Control) >= 2 * sizeof(Lhs));
  static_assert(sizeof(Control) >= 2 * sizeof(Rhs));

  Control controlRet = static_cast<Control>(lhs) + static_cast<Control>(rhs);
  constexpr Control min = std::numeric_limits<Result>::min();
  constexpr Control max = std::numeric_limits<Result>::max();
  if (controlRet < min || controlRet > max) {
    return std::nullopt;
  }
  return static_cast<Result>(controlRet);
}

template<typename Result, typename Control, typename Lhs, typename Rhs>
std::optional<Result>
trySub(Lhs lhs, Rhs rhs) {
  static_assert(std::is_signed_v<Control>);
  static_assert(sizeof(Control) >= 2 * sizeof(Lhs));
  static_assert(sizeof(Control) >= 2 * sizeof(Rhs));

  Control controlRet = static_cast<Control>(lhs) - static_cast<Control>(rhs);
  constexpr Control min = std::numeric_limits<Result>::min();
  constexpr Control max = std::numeric_limits<Result>::max();
  if (controlRet < min || controlRet > max) {
    return std::nullopt;
  }
  return static_cast<Result>(controlRet);
}

} // namespace rocket

// EOF
