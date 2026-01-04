/**
 * @file numeric.h
 *
 * Utilities around native numerics.
 */

#pragma once

#include "Type.h"

#include <limits>
#include <stdexcept>
#include <type_traits>

namespace rocket {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Adds two values and returns the result, checking the `Result` type for overflow.
 *
 * @tparam Result the result type
 * @tparam Control the control type
 * @tparam Lhs the left-hand side type
 * @tparam Rhs the right-hand side type
 * @param lhs the left-hand side value
 * @param rhs the right-hand side value
 * @return the result
 * @throws `std::overflow_error` if the result is out of range
 */
template<typename Result, typename Control, typename Lhs, typename Rhs>
constexpr Result
add(Lhs lhs, Rhs rhs) {
  static_assert(std::is_signed_v<Control>);
  static_assert(sizeof(Control) > sizeof(Lhs));
  static_assert(sizeof(Control) > sizeof(Rhs));

  Control controlRet = static_cast<Control>(lhs) + static_cast<Control>(rhs);
  constexpr Control min = std::numeric_limits<Result>::min();
  constexpr Control max = std::numeric_limits<Result>::max();
  if (controlRet < min || controlRet > max) {
    throw std::overflow_error(fmt::format("`{}` overflow: {} + {}", Type::of<Result>(), lhs, rhs));
  }
  return static_cast<Result>(controlRet);
}

/**
 * Subtracts two values and returns the result, checking the `Result` type for overflow.
 *
 * @tparam Result the result type
 * @tparam Control the control type
 * @tparam Lhs the left-hand side type
 * @tparam Rhs the right-hand side type
 * @param lhs the left-hand side value
 * @param rhs the right-hand side value
 * @return the result
 * @throws `std::overflow_error` if the result is out of range
 */
template<typename Result, typename Control, typename Lhs, typename Rhs>
constexpr Result
sub(Lhs lhs, Rhs rhs) {
  static_assert(std::is_signed_v<Control>);
  static_assert(sizeof(Control) > sizeof(Lhs));
  static_assert(sizeof(Control) > sizeof(Rhs));

  Control controlRet = static_cast<Control>(lhs) - static_cast<Control>(rhs);
  constexpr Control min = std::numeric_limits<Result>::min();
  constexpr Control max = std::numeric_limits<Result>::max();
  if (controlRet < min || controlRet > max) {
    throw std::overflow_error(fmt::format("`{}` overflow: {} - {}", Type::of<Result>(), lhs, rhs));
  }
  return static_cast<Result>(controlRet);
}

/**
 * Tries to add two values and return the result, returning null if the result is out of range.
 *
 * @tparam Result the result type
 * @tparam Control the control type
 * @tparam Lhs the left-hand side type
 * @tparam Rhs the right-hand side type
 * @param lhs the left-hand side value
 * @param rhs the right-hand side value
 * @return the result, or null if the result is out of range
 */
template<typename Result, typename Control, typename Lhs, typename Rhs>
constexpr std::optional<Result>
tryAdd(Lhs lhs, Rhs rhs) {
  static_assert(std::is_signed_v<Control>);
  static_assert(sizeof(Control) > sizeof(Lhs));
  static_assert(sizeof(Control) > sizeof(Rhs));

  Control controlRet = static_cast<Control>(lhs) + static_cast<Control>(rhs);
  constexpr Control min = std::numeric_limits<Result>::min();
  constexpr Control max = std::numeric_limits<Result>::max();
  if (controlRet < min || controlRet > max) {
    return std::nullopt;
  }
  return static_cast<Result>(controlRet);
}

/**
 * Tries to subtract two values and return the result, returning null if the result is out of range.
 *
 * @tparam Result the result type
 * @tparam Control the control type
 * @tparam Lhs the left-hand side type
 * @tparam Rhs the right-hand side type
 * @param lhs the left-hand side value
 * @param rhs the right-hand side value
 * @return the result, or null if the result is out of range
 */
template<typename Result, typename Control, typename Lhs, typename Rhs>
constexpr std::optional<Result>
trySub(Lhs lhs, Rhs rhs) {
  static_assert(std::is_signed_v<Control>);
  static_assert(sizeof(Control) > sizeof(Lhs));
  static_assert(sizeof(Control) > sizeof(Rhs));

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
