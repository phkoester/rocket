/**
 * @file numeric.h
 *
 * Utilities around native numerics.
 */

#pragma once

#include "rocket/Exception.h"
#include "rocket/Type.h"
#include "rocket/rocket.h"
#include "rocket/type-traits.h"

#include <limits>
#include <type_traits>

namespace rocket {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

template<typename T> struct Control;

template<> struct Control<char> { using Type = i32; };
template<> struct Control<char32> { using Type = i64; };

template<> struct Control<i16> { using Type = i32; };
template<> struct Control<u16> { using Type = i32; };
template<> struct Control<i32> { using Type = i64; };
template<> struct Control<u32> { using Type = i64; };
template<> struct Control<i64> { using Type = i128; };
template<> struct Control<u64> { using Type = i128; };
template<> struct Control<i128> { using Type = i128; };
template<> struct Control<u128> { using Type = u128; };

} // namespace internal

// `NumericTraits` ------------------------------------------------------------------------------------------

/// The `NumericTraits` template.
template<typename T>
struct NumericTraits;

/// @spec{#rocket::NumericTraits, `char`}
template<>
struct NumericTraits<char> {
  /// @f$2^{7}@f$ = 128.
  static constexpr int negativeMin = static_cast<int>(std::numeric_limits<char>::max()) + 1;
};

/// @spec{#rocket::NumericTraits, #rocket::i16}
template<>
struct NumericTraits<i16> {
  /// @f$2^{15}@f$ = 32,768.
  static constexpr i32 negativeMin = static_cast<i32>(std::numeric_limits<i16>::max()) + 1;
};

/// @spec{#rocket::NumericTraits, #rocket::i32}
template<>
struct NumericTraits<i32> {
  /// @f$2^{31}@f$ = 2,147,483,648.
  static constexpr i64 negativeMin = static_cast<i64>(std::numeric_limits<i32>::max()) + 1;
};

/// @spec{#rocket::NumericTraits, #rocket::i64}
template<>
struct NumericTraits<i64> {
  /// @f$2^{63}@f$ = 9,223,372,036,854,775,808.
  static constexpr i128 negativeMin = static_cast<i128>(std::numeric_limits<i64>::max()) + 1;
};

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
    if ((controlRhs > 0 && controlRet <= controlLhs) || (controlRhs < 0 && controlRet >= controlLhs)) {
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
 * Checks if @p v is a quiet not-a-number.
 *
 * @tparam F the floating-point type
 * @param v a floating-point value
 * @return whether @p v is a quiet not-a-number
 */
template<typename F> requires IsFloat<F>
constexpr bool
quietNan(F v) {
  return std::isnan(v) && not issignaling(v);
}

/**
 * Checks if @p v is a signaling not-a-number.
 *
 * @tparam F the floating-point type
 * @param v a floating-point value
 * @return whether @p v is a signaling not-a-number
 */
template<typename F> requires IsFloat<F>
constexpr bool
signalingNan(F v) {
  return std::isnan(v) && issignaling(v);
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
    if ((controlRhs > 0 && controlRet >= controlLhs) || (controlRhs < 0 && controlRet <= controlLhs)) {
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
    if ((controlRhs > 0 && controlRet <= controlLhs) || (controlRhs < 0 && controlRet >= controlLhs)) {
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
    if ((controlRhs > 0 && controlRet >= controlLhs) || (controlRhs < 0 && controlRet <= controlLhs)) {
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
