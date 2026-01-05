/**
 * @file numeric.h
 *
 * Utilities around native numerics.
 */

#pragma once

#include "rocket/Exception.h"
#include "rocket/Type.h"
#include "rocket/TypeTraits.h"
#include "rocket/rocket.h"

#include <limits>
#include <type_traits>

namespace rocket {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

template<typename T> struct Control;

template<> struct Control<signed char> { using Type = int; };
template<> struct Control<unsigned char> { using Type = int; };
template<> struct Control<short> { using Type = int; };
template<> struct Control<unsigned short> { using Type = int; };
template<> struct Control<int> { using Type = long; };
template<> struct Control<unsigned int> { using Type = long; };
template<> struct Control<long> { using Type = int128_t; };
template<> struct Control<unsigned long> { using Type = int128_t; };
template<> struct Control<int128_t> { using Type = int128_t; };
template<> struct Control<uint128_t> { using Type = uint128_t; };

} // namespace internal

// Constants ------------------------------------------------------------------------------------------------

/// 2^7 = 128.
constexpr int CHAR_NEG_MIN = static_cast<int>(std::numeric_limits<char>::max()) + 1;
/// 2^15 = 32,768.
constexpr int SHORT_NEG_MIN = static_cast<int>(std::numeric_limits<short>::max()) + 1;
/// 2^31 = 2,147,483,648.
constexpr long INT_NEG_MIN = static_cast<long>(std::numeric_limits<int>::max()) + 1;
/// 2^63 = 9,223,372,036,854,775,808.
constexpr int128_t LONG_NEG_MIN = static_cast<int128_t>(std::numeric_limits<long>::max()) + 1;

// Functions ------------------------------------------------------------------------------------------------

/**
 * Adds two values and returns the result, checking the @p Result type for overflow.
 *
 * @tparam Result the result type
 * @tparam Lhs the left-hand side type
 * @tparam Rhs the right-hand side type
 * @param lhs the left-hand side value
 * @param rhs the right-hand side value
 * @return the result
 * @throws #rocket::Overflow if the result is out of range
 */
template<typename Result, typename Lhs, typename Rhs>
constexpr Result
add(Lhs lhs, Rhs rhs) {
  using Control = internal::Control<typename LargestType<Result, Lhs, Rhs>::Type>::Type;
  Control controlLhs = static_cast<Control>(lhs);
  Control controlRhs = static_cast<Control>(rhs);
  Control controlRet = controlLhs + controlRhs;

  if constexpr (sizeof(Control) == sizeof(Result)) {
    // Mixing `int128_t` and `uint128_t` is not implemented yet
    static_assert(std::is_signed_v<Control> == std::is_signed_v<Result>);
    if ((controlRhs > 0 && controlRet < controlLhs) || (controlRhs < 0 && controlRet > controlLhs)) {
      throw Overflow(Type::of<Result>(), fmt::format("{} + {}", controlLhs, controlRhs));
    }
  } else {
    constexpr Control min = std::numeric_limits<Result>::min();
    constexpr Control max = std::numeric_limits<Result>::max();
    if (controlRet < min || controlRet > max) {
      throw Overflow(Type::of<Result>(), fmt::format("{} + {}", controlLhs, controlRhs));
    }
  }

  return static_cast<Result>(controlRet);
}

/**
 * Subtracts two values and returns the result, checking the @p Result type for overflow.
 *
 * @tparam Result the result type
 * @tparam Lhs the left-hand side type
 * @tparam Rhs the right-hand side type
 * @param lhs the left-hand side value
 * @param rhs the right-hand side value
 * @return the result
 * @throws #rocket::Overflow if the result is out of range
 */
template<typename Result, typename Lhs, typename Rhs>
constexpr Result
sub(Lhs lhs, Rhs rhs) {
  using Control = internal::Control<typename LargestType<Result, Lhs, Rhs>::Type>::Type;
  Control controlLhs = static_cast<Control>(lhs);
  Control controlRhs = static_cast<Control>(rhs);
  Control controlRet = controlLhs - controlRhs;

  if constexpr (sizeof(Control) == sizeof(Result)) {
    // Mixing `int128_t` and `uint128_t` is not implemented yet
    static_assert(std::is_signed_v<Control> == std::is_signed_v<Result>);
    if ((controlRhs > 0 && controlRet > controlLhs) || (controlRhs < 0 && controlRet < controlLhs)) {
      throw Overflow(Type::of<Result>(), fmt::format("{} - {}", controlLhs, controlRhs));
    }
  } else {
    constexpr Control min = std::numeric_limits<Result>::min();
    constexpr Control max = std::numeric_limits<Result>::max();
    if (controlRet < min || controlRet > max) {
      throw Overflow(Type::of<Result>(), fmt::format("{} - {}", controlLhs, controlRhs));
    }
  }

  return static_cast<Result>(controlRet);
}

/**
 * Converts a value to another type, checking the @p Result type for overflow.
 *
 * @tparam Result the result type
 * @tparam T the value type
 * @param v the value to convert
 * @return the result
 * @throws #rocket::Overflow if the result is out of range
 */
template<typename Result, typename T>
constexpr Result
to(T v) {
  using Control = internal::Control<typename LargestType<Result, T>::Type>::Type;
  Control controlRet = static_cast<Control>(v);

  if constexpr (sizeof(Control) > sizeof(Result)) {
    constexpr Control min = std::numeric_limits<Result>::min();
    constexpr Control max = std::numeric_limits<Result>::max();
    if (controlRet < min || controlRet > max) {
      throw Overflow(Type::of<Result>(), fmt::format("{}", controlRet));
    }
  }

  if (v < 0 && std::is_unsigned_v<Result>) {
    throw Overflow(Type::of<Result>(), fmt::format("{}", v));
  }
  return static_cast<Result>(controlRet);
}

/**
 * Tries to add two values and return the result, returning null if the result is out of range.
 *
 * @tparam Result the result type
 * @tparam Lhs the left-hand side type
 * @tparam Rhs the right-hand side type
 * @param lhs the left-hand side value
 * @param rhs the right-hand side value
 * @return the result, or null if the result is out of range
 */
template<typename Result, typename Lhs, typename Rhs>
constexpr std::optional<Result>
tryAdd(Lhs lhs, Rhs rhs) {
  using Control = internal::Control<typename LargestType<Result, Lhs, Rhs>::Type>::Type;
  Control controlLhs = static_cast<Control>(lhs);
  Control controlRhs = static_cast<Control>(rhs);
  Control controlRet = controlLhs + controlRhs;

  if constexpr (sizeof(Control) == sizeof(Result)) {
    // Mixing `int128_t` and `uint128_t` is not implemented yet
    static_assert(std::is_signed_v<Control> == std::is_signed_v<Result>);
    if ((controlRhs > 0 && controlRet < controlLhs) || (controlRhs < 0 && controlRet > controlLhs)) {
      return std::nullopt;
    }
  } else {
    constexpr Control min = std::numeric_limits<Result>::min();
    constexpr Control max = std::numeric_limits<Result>::max();
    if (controlRet < min || controlRet > max) {
      return std::nullopt;
    }
  }

  return static_cast<Result>(controlRet);
}

/**
 * Tries to subtract two values and return the result, returning null if the result is out of range.
 *
 * @tparam Result the result type
 * @tparam Lhs the left-hand side type
 * @tparam Rhs the right-hand side type
 * @param lhs the left-hand side value
 * @param rhs the right-hand side value
 * @return the result, or null if the result is out of range
 */
template<typename Result, typename Lhs, typename Rhs>
constexpr std::optional<Result>
trySub(Lhs lhs, Rhs rhs) {
  using Control = internal::Control<typename LargestType<Result, Lhs, Rhs>::Type>::Type;
  Control controlLhs = static_cast<Control>(lhs);
  Control controlRhs = static_cast<Control>(rhs);
  Control controlRet = controlLhs - controlRhs;

  if constexpr (sizeof(Control) == sizeof(Result)) {
    // Mixing `int128_t` and `uint128_t` is not implemented yet
    static_assert(std::is_signed_v<Control> == std::is_signed_v<Result>);
    if ((controlRhs > 0 && controlRet > controlLhs) || (controlRhs < 0 && controlRet < controlLhs)) {
      return std::nullopt;
    }
  } else {
    constexpr Control min = std::numeric_limits<Result>::min();
    constexpr Control max = std::numeric_limits<Result>::max();
    if (controlRet < min || controlRet > max) {
      return std::nullopt;
    }
  }

  return static_cast<Result>(controlRet);
}

/**
 * Tries to convert a value to another type, returning null if the result is out of range.
 *
 * @tparam Result the result type
 * @tparam T the value type
 * @param v the value to convert
 * @return the result, or null if the result is out of range
 */
template<typename Result, typename T>
constexpr std::optional<Result>
tryTo(T v) {
  using Control = internal::Control<typename LargestType<Result, T>::Type>::Type;
  Control controlRet = static_cast<Control>(v);

  if constexpr (sizeof(Control) > sizeof(Result)) {
    constexpr Control min = std::numeric_limits<Result>::min();
    constexpr Control max = std::numeric_limits<Result>::max();
    if (controlRet < min || controlRet > max) {
      return std::nullopt;
    }
  }

  if (v < 0 && std::is_unsigned_v<Result>) {
    return std::nullopt;
  }
  return static_cast<Result>(controlRet);
}

} // namespace rocket

// EOF
