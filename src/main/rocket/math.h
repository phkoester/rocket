/**
 * @file math.h
 *
 * Mathematical utilities.
 */

#pragma once

#include "base.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <type_traits>

namespace rocket::math {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

// `BoundTraits` ............................................................................................

template<typename T, typename BoundType_, bool Left_, char Symbol_>
struct BoundTraits {
  using Type = T;
  using BoundType = BoundType_;

  static constexpr bool Left = Left_;
  static constexpr bool Closed = std::is_same_v<BoundType, T>;
  static constexpr char Symbol = Symbol_;

  static constexpr bool
  matches(BoundType bound, Type v) {
    if constexpr (Left) {
      // Left
      if constexpr (Closed)
        return bound <= v;
      else
        return not bound ? true : *bound < v;
    } else {
      // Right
     if constexpr (Closed)
       return bound >= v;
     else
       return not bound ? true : *bound > v;
    }
  }

  static constexpr BoundType
  max(BoundType a, BoundType b) {
    if constexpr (Closed)
      return std::max(a, b);
    else {
      if (a && b)
        return std::max(*a, *b);
      else if (a)
        return b;
      else
        return std::nullopt;
    }
  }

  static constexpr BoundType
  min(BoundType a, BoundType b) {
    if constexpr (Closed)
      return std::min(a, b);
    else {
      if (a && b)
        return std::min(*a, *b);
      else if (a)
        return b;
      else
        return std::nullopt;
    }
  }
};

template<typename T> using LeftClosed = BoundTraits<T, T, true, '['>;
template<typename T> using LeftOpen = BoundTraits<T, std::optional<T>, true, '('>;
template<typename T> using RightClosed = BoundTraits<T, T, false, ']'>;
template<typename T> using RightOpen = BoundTraits<T, std::optional<T>, false, ')'>;

template<typename Left, typename Right>
constexpr std::pair<typename Left::BoundType, typename Right::BoundType>
intersectionImpl(
    typename Left::BoundType lhsLower, typename Right::BoundType lhsUpper,
    typename Left::BoundType rhsLower, typename Right::BoundType rhsUpper) {
  return { Left::max(lhsLower, rhsLower), Right::min(lhsUpper, rhsUpper) };
}

template<typename Left, typename Right>
constexpr std::pair<typename Left::BoundType, typename Right::BoundType>
unionImpl(
    typename Left::BoundType lhsLower, typename Right::BoundType lhsUpper,
    typename Left::BoundType rhsLower, typename Right::BoundType rhsUpper) {
  return { Left::min(lhsLower, rhsLower), Right::max(lhsUpper, rhsUpper) };
}

// `IntervalTraits` .........................................................................................

template<typename T, typename Left, typename Right>
struct IntervalTraits;

// Interval (closed interval)
template<typename T>
struct IntervalTraits<T, LeftClosed<T>, RightClosed<T>> {
  using LeftType = LeftClosed<T>;
  using RightType = RightClosed<T>;

  using SizeType = T;

  static constexpr bool
  empty(LeftType::BoundType lower, RightType::BoundType upper) {
    return upper < lower;
  }

  static constexpr SizeType
  size(LeftType::BoundType lower, RightType::BoundType upper) {
    if (empty(lower, upper))
      return 0;
    return upper - lower;
  }
};

// Open interval
template<typename T>
struct IntervalTraits<T, LeftOpen<T>, RightOpen<T>> {
  using LeftType = LeftOpen<T>;
  using RightType = RightOpen<T>;

  using SizeType = std::optional<T>;

  static constexpr bool
  empty(LeftType::BoundType lower, RightType::BoundType upper) {
    if (not lower || not upper)
      return false;
    if (*upper < *lower)
      return true;
    if constexpr (std::is_integral_v<T>)
      return *upper - *lower < 2;
    else
      return *upper == *lower;
  }

  static constexpr SizeType
  size(LeftType::BoundType lower, RightType::BoundType upper) {
    if (empty(lower, upper))
      return 0;
    if (not upper || not lower)
      return std::nullopt;
    return *upper - *lower;
  }
};

// Left-open interval
template<typename T>
struct IntervalTraits<T, LeftOpen<T>, RightClosed<T>> {
  using LeftType = LeftOpen<T>;
  using RightType = RightClosed<T>;

  using SizeType = std::optional<T>;

  static constexpr bool
  empty(LeftType::BoundType lower, RightType::BoundType upper) {
    if (not lower)
      return false;
    if (upper < *lower)
      return true;
    if constexpr (std::is_integral_v<T>)
      return upper - *lower < 1;
    else
      return upper == *lower;
  }

  static constexpr SizeType
  size(LeftType::BoundType lower, RightType::BoundType upper) {
    if (empty(lower, upper))
      return 0;
    if (not lower)
      return std::nullopt;
    return upper - *lower;
  }
};

// Right-open interval
template<typename T>
struct IntervalTraits<T, LeftClosed<T>, RightOpen<T>> {
  using LeftType = LeftClosed<T>;
  using RightType = RightOpen<T>;

  using SizeType = std::optional<T>;

  static constexpr bool
  empty(LeftType::BoundType lower, RightType::BoundType upper) {
    if (not upper)
      return false;
    if (*upper < lower)
      return true;
    if constexpr (std::is_integral_v<T>)
      return *upper - lower < 1;
    else
      return *upper <= lower;
  }

  static constexpr SizeType
  size(LeftType::BoundType lower, RightType::BoundType upper) {
    if (empty(lower, upper))
      return 0;
    if (not upper)
      return std::nullopt;
    return *upper - lower;
  }
};

} // namespace internal

// `IntervalImpl` -------------------------------------------------------------------------------------------

/**
 * A mathematical interval for either integer or noninteger types.
 *
 * @tparam T the element type
 * @tparam Left the type of the lower-bound traits
 * @tparam Right the type of the upper-bound traits
 */
template<typename T, typename Left, typename Right>
struct IntervalImpl {
  static_assert(
       std::is_same_v<Left, internal::LeftClosed<T>> || std::is_same_v<Left, internal::LeftOpen<T>>);
  static_assert(
       std::is_same_v<Right, internal::RightClosed<T>> || std::is_same_v<Right, internal::RightOpen<T>>);

  /// The element type.
  using Type = T;

  /// The type of the lower-bound traits.
  using LeftType = Left;
  /// The type of the upper-bound traits.
  using RightType = Right;

  /// The type of the lower bound.
  using LowerType = Left::BoundType;
  /// The type of the upper bound.
  using UpperType = Right::BoundType;

  /// The interval traits.
  using Traits = internal::IntervalTraits<T, Left, Right>;

  /**
    * The interval's lower bound.
    *
    * If this is null, then the interval has no lower bound and no size and is nonempty.
    */
  LowerType lower;
  /**
    * The interval's upper bound.
    *
    * If this is null, then the interval has no upper bound and no size and is nonempty.
    */
  UpperType upper;

  /**
   * @ctor_default
   *
   * Makes an empty interval.
   */
  consteval IntervalImpl() : lower(static_cast<T>(1)), upper(static_cast<T>(0)) {}

  /**
   * @ctor
   *
   * Makes an interval.
   *
   * @param lower the lower bound. If null, then there is no lower bound
   * @param upper the upper bound. If null, then there is no upper bound
   */
  constexpr IntervalImpl(LowerType lower, UpperType upper) : lower(lower), upper(upper) {}

  /**
   * @member_op_cast{`bool`}
   *
   * @return `true` if this interval is nonempty
   */
  inline operator bool() const { return not empty(); }

  /// @member_op_eq
  bool
  operator==(const IntervalImpl& rhs) const {
    if (empty() && rhs.empty())
      return true;
    return lower == rhs.lower && upper == rhs.upper;
  }

  /// @member_op_ne
  inline bool operator!=(const IntervalImpl& rhs) const { return not operator==(rhs); }

  /**
   * Tests if @p v is contained in this interval.
   *
   * @param v a value of type @p T
   * @return `true` if @p v is contained in this interval
   */
  constexpr bool contains(T v) const { return Left::matches(lower, v) && Right::matches(upper, v); }

  /**
   * Tests if this interval is empty.
   *
   * If either #lower or #upper are null, meaning "infinite", then this interval is nonempty.
   *
   * @return `true` if this interval is empty
   */
  constexpr bool empty() const { return Traits::empty(lower, upper); }

  /**
   * Returns the size of this interval.
   *
   * If either #lower or #upper are null, then the size of the interval is null, meaning "infinite".
   * Otherwise, the size is calculated as #upper - #lower.
   *
   * @attention A size of 0 doesn't necessarily mean an interval is empty. For instance, the closed interval
   * [2,2] has a size of 0 and is nonempty. On the other hand, an empty interval always has a size of 0. To
   * check if an interval is empty, use the #empty member function.
   *
   * @return the size of this interval
   */
  constexpr Traits::SizeType size() const { return Traits::size(lower, upper); }
};

/// @op_output{#rocket::math::IntervalImpl}
template<typename T, typename Left, typename Right>
std::ostream&
operator<<(std::ostream& lhs, const IntervalImpl<T, Left, Right>& rhs) {
  if (rhs.empty()) {
    // Use a neat mathematical symbol
    return lhs << "∅";
  } else {
    lhs << Left::Symbol;
    auto opt = option(rhs.lower);
    if (not opt)
      lhs << "-∞";
    else
      lhs << *opt;
    lhs << ',';
    opt = option(rhs.upper);
    if (not opt)
      lhs << "+∞"; // In interval notation, we prefer "+∞" over "∞"
    else
      lhs << *opt;
    return lhs << Right::Symbol;
  }
}

/**
 * Returns the intersection of the intervals @p lhs and @p rhs.
 *
 * @tparam T the element type
 * @tparam Left the type of the lower-bound traits
 * @tparam Right the type of the upper-bound traits
 * @param_lhs
 * @param_rhs
 * @return a #rocket::math::IntervalImpl representing the intersection of @p lhs and @p rhs if such
 *     intersection exists, otherwise an empty interval
 */
template<typename T, typename Left, typename Right>
IntervalImpl<T, Left, Right>
operator&(const IntervalImpl<T, Left, Right>& lhs, const IntervalImpl<T, Left, Right>& rhs) {
  auto pair = internal::intersectionImpl<Left, Right>(lhs.lower, lhs.upper, rhs.lower, rhs.upper);
  return IntervalImpl<T, Left, Right>(pair.first, pair.second);
}

/**
 * `operator&=` for type #rocket::math::IntervalImpl.
 *
 * @tparam T the element type
 * @tparam Left the type of the lower-bound traits
 * @tparam Right the type of the upper-bound traits
 * @param_lhs
 * @param_rhs
 * @return @p lhs
 */
template<typename T, typename Left, typename Right>
inline IntervalImpl<T, Left, Right>&
operator&=(const IntervalImpl<T, Left, Right>& lhs, const IntervalImpl<T, Left, Right>& rhs) {
  return lhs = lhs & rhs;
}

/**
 * Returns the union of the intervals @p lhs and @p rhs.
 *
 * @tparam T the element type
 * @tparam Left the type of the lower-bound traits
 * @tparam Right the type of the upper-bound traits
 * @param_lhs
 * @param_rhs
 * @return a #rocket::math::IntervalImpl representing the union of @p lhs and @p rhs if such union exists,
 *     otherwise an empty interval
 */
template<typename T, typename Left, typename Right>
IntervalImpl<T, Left, Right>
operator|(const IntervalImpl<T, Left, Right>& lhs, const IntervalImpl<T, Left, Right>& rhs) {
  auto pair = internal::unionImpl<Left, Right>(lhs.lower, lhs.upper, rhs.lower, rhs.upper);
  return IntervalImpl<T, Left, Right>(pair.first, pair.second);
}

/**
 * `operator|=` for type #rocket::math::IntervalImpl.
 *
 * @tparam T the element type
 * @tparam Left the type of the lower-bound traits
 * @tparam Right the type of the upper-bound traits
 * @param_lhs
 * @param_rhs
 * @return @p lhs
 */
template<typename T, typename Left, typename Right>
inline IntervalImpl<T, Left, Right>&
operator|=(const IntervalImpl<T, Left, Right>& lhs, const IntervalImpl<T, Left, Right>& rhs) {
  return lhs = lhs | rhs;
}

// Interval types -------------------------------------------------------------------------------------------

/**
 * A closed interval [*lower*,*upper*] contains all elements *x* such that *lower* <= *x* <= *upper*.
 *
 * @tparam T the element type
 */
template<typename T>
using Interval = IntervalImpl<T, internal::LeftClosed<T>, internal::RightClosed<T>>;

/**
 * An open interval (*lower*,*upper*) contains all elements *x* such that *lower* < *x* < *upper*.
 *
 * @tparam T the element type
 */
template<typename T>
using OpenInterval = IntervalImpl<T, internal::LeftOpen<T>, internal::RightOpen<T>>;

/**
 * A left-open interval (*lower*,*upper*] contains all elements *x* such that *lower* < *x* <= *upper*.
 *
 * @tparam T the element type
 */
template<typename T>
using LeftOpenInterval = IntervalImpl<T, internal::LeftOpen<T>, internal::RightClosed<T>>;

/**
 * A right-open interval [*lower*,*upper*) contains all elements *x* such that *lower* <= *x* < *upper*.
 *
 * @tparam T the element type
 */
template<typename T>
using RightOpenInterval = IntervalImpl<T, internal::LeftClosed<T>, internal::RightOpen<T>>;

} // namespace rocket::math

// EOF
