/**
 * @file Interval.h
 *
 * Mathematical intervals: closed, open, left-open, and right-open.
 */

#pragma once

#include "rocket/type-traits.h"

#include <algorithm>
#include <optional>
#include <type_traits>

namespace rocket::math {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

// `BoundTraits` ............................................................................................

template<typename T, typename _BoundType, bool _IsLeft, char _Symbol>
struct BoundTraits {
  using Type = T;
  using BoundType = _BoundType; ///< Either `T` or `std::optional<T>`

  static constexpr bool IsClosed = std::is_same_v<BoundType, Type>;
  static constexpr bool IsLeft = _IsLeft;
  static constexpr char Symbol = _Symbol;

  static constexpr bool
  matches(BoundType bound, Type val) {
    if constexpr (IsLeft) {
      // Left
      if constexpr (IsClosed) {
        return bound <= val;
      } else {
        return not bound ? true : *bound < val;
      }
    } else {
      // Right
     if constexpr (IsClosed) {
       return bound >= val;
     } else {
       return not bound ? true : *bound > val;
     }
    }
  }

  static constexpr BoundType
  leftMax(BoundType lhs, BoundType rhs) {
    if constexpr (IsClosed) {
      return std::max(lhs, rhs);
    } else {
      // null < anything
      if (not lhs) {
        return rhs;
      }
      if (not rhs) {
        return lhs;
      }
      return std::max(*lhs, *rhs);
    }
  }

  static constexpr BoundType
  leftMin(BoundType lhs, BoundType rhs) {
    if constexpr (IsClosed) {
      return std::min(lhs, rhs);
    } else {
      // null < anything
      if (not lhs) {
        return lhs;
      }
      if (not rhs) {
        return rhs;
      }
      return std::min(*lhs, *rhs);
    }
  }

  static constexpr BoundType
  rightMax(BoundType lhs, BoundType rhs) {
    if constexpr (IsClosed) {
      return std::max(lhs, rhs);
    } else {
      // null > anything
      if (not lhs) {
        return lhs;
      }
      if (not rhs) {
        return rhs;
      }
      return std::max(*lhs, *rhs);
    }
  }

  static constexpr BoundType
  rightMin(BoundType lhs, BoundType rhs) {
    if constexpr (IsClosed) {
      return std::min(lhs, rhs);
    } else {
      // null > anything
      if (not lhs) {
        return rhs;
      }
      if (not rhs) {
        return lhs;
      }
      return std::min(*lhs, *rhs);
    }
  }
};

template<typename T> using LeftClosed = BoundTraits<T, T, true, '['>;
template<typename T> using LeftOpen = BoundTraits<T, std::optional<T>, true, '('>;
template<typename T> using RightClosed = BoundTraits<T, T, false, ']'>;
template<typename T> using RightOpen = BoundTraits<T, std::optional<T>, false, ')'>;

// `Cardinality` ............................................................................................

template<typename T, bool LeftClosed, bool RightClosed>
struct Cardinality;

template<typename I> requires IsInteger<I>
struct Cardinality<I, false, false> { using Type = std::optional<typename Uint<sizeof(I)>::Type>; };

template<typename I> requires IsInteger<I>
struct Cardinality<I, false, true> { using Type = std::optional<typename Uint<sizeof(I)>::Type>; };

template<typename I> requires IsInteger<I>
struct Cardinality<I, true, false> { using Type = std::optional<typename Uint<sizeof(I)>::Type>; };

template<typename I> requires IsInteger<I>
struct Cardinality<I, true, true> { using Type = typename Uint<sizeof(I)>::Type; };

template<typename F> requires IsFloat<F>
struct Cardinality<F, false, false> { using Type = std::optional<u32>; };

template<typename F> requires IsFloat<F>
struct Cardinality<F, false, true> { using Type = std::optional<u32>; };

template<typename F> requires IsFloat<F>
struct Cardinality<F, true, false> { using Type = std::optional<u32>; };

template<typename F> requires IsFloat<F>
struct Cardinality<F, true, true> { using Type = std::optional<u32>; };

// `IntervalSymbols` ........................................................................................

template<typename C> requires IsChar<C>
struct IntervalSymbols;

template<>
struct IntervalSymbols<char> {
  static constexpr auto Empty = "∅";
  static constexpr auto NegativeInfinity = "-∞";
  static constexpr auto PositiveInfinity = "∞";
};

template<>
struct IntervalSymbols<char32> {
  static constexpr auto Empty = U"∅";
  static constexpr auto NegativeInfinity = U"-∞";
  static constexpr auto PositiveInfinity = U"∞";
};

// `IntervalTraits` .........................................................................................

template<typename Left, typename Right>
struct IntervalTraits;

// Closed interval
template<typename T>
struct IntervalTraits<LeftClosed<T>, RightClosed<T>> {
  using LeftType = LeftClosed<T>;
  using RightType = RightClosed<T>;

  using CardinalityType = Cardinality<T, LeftType::IsClosed, RightType::IsClosed>::Type;
  using SizeType = T;

  static constexpr CardinalityType
  cardinality(LeftType::BoundType a, RightType::BoundType b) {
    if (empty(a, b)) {
      return 0;
    }
    if constexpr (IsInteger<T>) {
      return b - a + 1;
    } else {
      // This is the only case where a floating-point interval has a cardinality of 1
      if (b == a) {
        return 1;
      }
      return std::nullopt;
    }
  }

  static constexpr bool
  empty(LeftType::BoundType a, RightType::BoundType b) {
    return b < a;
  }

  static constexpr SizeType
  size(LeftType::BoundType a, RightType::BoundType b) {
    if (empty(a, b)) {
      return 0;
    }
    return b - a;
  }
};

// Open interval
template<typename T>
struct IntervalTraits<LeftOpen<T>, RightOpen<T>> {
  using LeftType = LeftOpen<T>;
  using RightType = RightOpen<T>;

  using CardinalityType = Cardinality<T, LeftType::IsClosed, RightType::IsClosed>::Type;
  using SizeType = std::optional<T>;

  static constexpr CardinalityType
  cardinality(LeftType::BoundType a, RightType::BoundType b) {
    if (empty(a, b)) {
      return 0;
    }
    if (not a || not b) {
      return std::nullopt;
    }
    if constexpr (IsInteger<T>) {
      return *b - *a - 1;
    } else {
      return std::nullopt;
    }
  }

  static constexpr bool
  empty(LeftType::BoundType a, RightType::BoundType b) {
    if (not a || not b) {
      return false;
    }
    if (*b < *a) {
      return true;
    }
    if constexpr (IsInteger<T>) {
      return *b - *a < 2;
    } else {
      return *b == *a;
    }
  }

  static constexpr SizeType
  size(LeftType::BoundType a, RightType::BoundType b) {
    if (empty(a, b)) {
      return 0;
    }
    if (not b || not a) {
      return std::nullopt;
    }
    return *b - *a;
  }
};

// Left-open interval
template<typename T>
struct IntervalTraits<LeftOpen<T>, RightClosed<T>> {
  using LeftType = LeftOpen<T>;
  using RightType = RightClosed<T>;

  using CardinalityType = Cardinality<T, LeftType::IsClosed, RightType::IsClosed>::Type;
  using SizeType = std::optional<T>;

  static constexpr CardinalityType
  cardinality(LeftType::BoundType a, RightType::BoundType b) {
    if (empty(a, b)) {
      return 0;
    }
    if (not a) {
      return std::nullopt;
    }
    if constexpr (IsInteger<T>) {
      return b - *a;
    } else {
      return std::nullopt;
    }
  }

  static constexpr bool
  empty(LeftType::BoundType a, RightType::BoundType b) {
    if (not a) {
      return false;
    }
    if (b < *a) {
      return true;
    }
    if constexpr (IsInteger<T>) {
      return b - *a < 1;
    } else {
      return b == *a;
    }
  }

  static constexpr SizeType
  size(LeftType::BoundType a, RightType::BoundType b) {
    if (empty(a, b)) {
      return 0;
    }
    if (not a) {
      return std::nullopt;
    }
    return b - *a;
  }
};

// Right-open interval
template<typename T>
struct IntervalTraits<LeftClosed<T>, RightOpen<T>> {
  using LeftType = LeftClosed<T>;
  using RightType = RightOpen<T>;

  using CardinalityType = Cardinality<T, LeftType::IsClosed, RightType::IsClosed>::Type;
  using SizeType = std::optional<T>;

  static constexpr CardinalityType
  cardinality(LeftType::BoundType a, RightType::BoundType b) {
    if (empty(a, b)) {
      return 0;
    }
    if (not b) {
      return std::nullopt;
    }
    if constexpr (IsInteger<T>) {
      return *b - a;
    } else {
      return std::nullopt;
    }
  }

  static constexpr bool
  empty(LeftType::BoundType a, RightType::BoundType b) {
    if (not b) {
      return false;
    }
    if (*b < a) {
      return true;
    }
    if constexpr (IsInteger<T>) {
      return *b - a < 1;
    } else {
      return *b <= a;
    }
  }

  static constexpr SizeType
  size(LeftType::BoundType a, RightType::BoundType b) {
    if (empty(a, b)) {
      return 0;
    }
    if (not b) {
      return std::nullopt;
    }
    return *b - a;
  }
};

} // namespace internal

// `Interval` -----------------------------------------------------------------------------------------------

/**
 * A mathematical interval for either integer or noninteger types.
 *
 * @tparam Left the type of the lower-bound traits
 * @tparam Right the type of the upper-bound traits
 */
template<typename Left, typename Right>
struct Interval {
  static_assert(std::is_same_v<typename Left::Type, typename Right::Type>);
  /// The element type.
  using Type = typename Left::Type;

  static_assert(
       std::is_same_v<Left, internal::LeftClosed<Type>> || std::is_same_v<Left, internal::LeftOpen<Type>>);
  static_assert(
       std::is_same_v<Right, internal::RightClosed<Type>> || std::is_same_v<Right, internal::RightOpen<Type>>);

  /// The interval traits.
  using Traits = internal::IntervalTraits<Left, Right>;

  /// The type of the lower-bound traits.
  using LeftType = Left;
  /// The type of the upper-bound traits.
  using RightType = Right;

  /// The type of the lower bound.
  using A = Left::BoundType;
  /// The type of the upper bound.
  using B = Right::BoundType;

  /**
   * The interval's lower bound.
   *
   * If this is null, then the interval has no lower bound and no size and is nonempty.
   */
  A a;
  /**
   * The interval's upper bound.
   *
   * If this is null, then the interval has no upper bound and no size and is nonempty.
   */
  B b;

  /**
   * @ctor_default
   *
   * Constructs an empty interval.
   */
  constexpr Interval() : a(static_cast<Type>(1)), b(static_cast<Type>(0)) {}

  /**
   * @ctor
   *
   * Constructs an interval.
   *
   * If @p b is less than @p a, then the interval is empty.
   *
   * @param a the lower bound. If null, then there is no lower bound
   * @param b the upper bound. If null, then there is no upper bound
   */
  constexpr Interval(A a, B b) : a(a), b(b) {}

  /**
   * Returns the cardinality of the interval.
   *
   * If either #a or #b are null, then the cardinality is null, meaning "infinite". Otherwise, the
   * cardinality is calculated as appropriate.
   *
   * Empty intervals have a cardinality of 0.
   *
   * A floating-point interval has a cardinality of 1 if, and only if, it is a closed interval and #a equals
   * #b.
   *
   * @return the cardinality of the interval, or null if the cardinality is infinite
   */
  [[nodiscard]] constexpr Traits::CardinalityType cardinality() const { return Traits::cardinality(a, b); }

  /**
   * Tests if @p val is contained in the interval.
   *
   * @param val a value of type #Type
   * @return whether @p val is contained in the interval
   */
  [[nodiscard]] constexpr bool
  contains(Type val) const {
    return Left::matches(a, val) && Right::matches(b, val);
  }

  /**
   * Tests if the interval is empty.
   *
   * If either #a or #b are null, meaning "infinite", then the interval is nonempty.
   *
   * @return whether the interval is empty
   */
  [[nodiscard]] constexpr bool empty() const { return Traits::empty(a, b); }

  /**
   * Returns the size of the interval.
   *
   * If either #a or #b are null, then the size of the interval is null, meaning "infinite". Otherwise, the
   * size is calculated as @f$b -a@f$.
   *
   * @attention A size of 0 doesn't necessarily mean an interval is empty. For instance, the closed interval
   * @f$[2,2]@f$ has a size of 0, but a cardinality of 1, hence it is nonempty. To check if an interval is
   * empty, use #empty instead.
   *
   * @return the size of the interval, or null if the interval size is infinite
   */
  [[nodiscard]] constexpr Traits::SizeType size() const { return Traits::size(a, b); }
};

/// @op_eq{#rocket::math::Interval}
template<typename Left, typename Right>
inline bool
operator==(const Interval<Left, Right>& lhs, const Interval<Left, Right>& rhs) {
  if (lhs.empty() && rhs.empty()) {
    return true;
  }
  return lhs.a == rhs.a && lhs.b == rhs.b;
}

/**
 * Returns the intersection of the intervals @p lhs and @p rhs.
 *
 * @tparam Left the type of the lower-bound traits
 * @tparam Right the type of the upper-bound traits
 * @param_lhs
 * @param_rhs
 * @return an interval representing the intersection of @p lhs and @p rhs if such intersection exists,
 *   otherwise an empty interval
 */
template<typename Left, typename Right>
Interval<Left, Right>
operator&(const Interval<Left, Right>& lhs, const Interval<Left, Right>& rhs) {
  const auto a = Left::leftMax(lhs.a, rhs.a);
  const auto b = Right::rightMin(lhs.b, rhs.b);
  return Interval<Left, Right>(a, b);
}

/**
 * `operator&=` for type #rocket::math::Interval.
 *
 * @tparam Left the type of the lower-bound traits
 * @tparam Right the type of the upper-bound traits
 * @param_lhs
 * @param_rhs
 * @return @p lhs
 */
template<typename Left, typename Right>
inline Interval<Left, Right>&
operator&=(Interval<Left, Right>& lhs, const Interval<Left, Right>& rhs) {
  return lhs = lhs & rhs;
}

/**
 * Returns the union of the intervals @p lhs and @p rhs.
 *
 * If the intervals @p lhs and @p rhs are disjoint, a vector with two elements is returned. Otherwise, a
 * vector with one element is returned, which may be empty.
 *
 * @tparam Left the type of the lower-bound traits
 * @tparam Right the type of the upper-bound traits
 * @param_lhs
 * @param_rhs
 * @return a vector of intervals representing the union of @p lhs and @p rhs
 */
template<typename Left, typename Right>
std::vector<Interval<Left, Right>>
operator|(const Interval<Left, Right>& lhs, const Interval<Left, Right>& rhs) {
  // If one interval is empty, return the other one

  if (lhs.empty()) {
    return { rhs };
  }
  if (rhs.empty()) {
    return { lhs };
  }

  // Sort intervals

  const auto a = Left::leftMin(lhs.a, rhs.a);
  const auto b = Right::rightMax(lhs.b, rhs.b);

  const auto* pLhs = &lhs;
  const auto* pRhs = &rhs;

  if (a == rhs.a) {
    std::swap(pLhs, pRhs);
  }

  // Find out if the intervals are adjacent. If they are, collapse them into a single interval

  bool adjacent = false;

  if constexpr (IsInteger<typename Right::Type>) {
    if constexpr (Left::IsClosed && Right::IsClosed) {
      // Integer [a,b]
      adjacent = pRhs->a == pLhs->b + 1;
    } else if constexpr (Left::IsClosed ^ Right::IsClosed) {
      // Integer [a,b) or (a,b]
      adjacent = pRhs->a == pLhs->b;
    }
    // Open integer intervals are never adjacent
  } else {
    if constexpr (Left::IsClosed ^ Right::IsClosed) {
      // Float [a,b) or (a,b]
      adjacent = pRhs->a == pLhs->b;
    }
    // Closed and open float intervals are never adjacent
  }

  if (adjacent) {
    return { Interval<Left, Right>(a, b) };
  }

  // If there is no intersection, return two sorted intervals

  if ((lhs & rhs).empty()) {
    return { *pLhs, *pRhs };
  }

  // There is an intersection: collaps the two intervals into a single interval

  return { Interval<Left, Right>(a, b) };
}

// `Interval` types -----------------------------------------------------------------------------------------

/**
 * A closed interval @f$[a,b]@f$ contains all elements @f$x@f$ such that @f$a <= x <= b@f$.
 *
 * @tparam T the element type
 */
template<typename T>
using ClosedInterval = Interval<internal::LeftClosed<T>, internal::RightClosed<T>>;

/**
 * An open interval @f$(a,b)@f$ contains all elements @f$x@f$ such that @f$a < x < b@f$.
 *
 * @tparam T the element type
 */
template<typename T>
using OpenInterval = Interval<internal::LeftOpen<T>, internal::RightOpen<T>>;

/**
 * A left-open interval @f$(a,b]@f$ contains all elements @f$x@f$ such that @f$a < x <= b@f$.
 *
 * @tparam T the element type
 */
template<typename T>
using LeftOpenInterval = Interval<internal::LeftOpen<T>, internal::RightClosed<T>>;

/**
 * A right-open interval @f$[a,b)@f$ contains all elements @f$x@f$ such that @f$a <= x < b@f$.
 *
 * @tparam T the element type
 */
template<typename T>
using RightOpenInterval = Interval<internal::LeftClosed<T>, internal::RightOpen<T>>;

// `Intervals` ----------------------------------------------------------------------------------------------

/// A vector of intervals.
template<typename Left, typename Right>
using Intervals = std::vector<Interval<Left, Right>>;

/**
 * Returns the intersection of the interval vector @p val.
 *
 * @tparam Left the type of the lower-bound traits
 * @tparam Right the type of the upper-bound traits
 * @param val the vector of intervals, which may contain an arbitrary number of arbitrary intervals, empty or
 *   nonempty
 * @return an interval representing the intersection of all elements of @p val if such intersection exists,
 *   otherwise an empty interval
 */
template<typename Left, typename Right>
Interval<Left, Right>
intersectionOf(const Intervals<Left, Right>& val) {
  using Type = Interval<Left, Right>;

  std::optional<Type> ret;

  for (const auto& elem : val) {
    if (not ret) {
      ret = elem;
    } else {
      *ret &= elem;
    }
    if (ret && ret->empty()) {
      return Type();
    }
  }

  return ret.value_or(Type());
}

/**
 * Returns the union of the interval vector @p val.
 *
 * @tparam Left the type of the lower-bound traits
 * @tparam Right the type of the upper-bound traits
 * @param val the vector of intervals, which may contain an arbitrary number of arbitrary intervals, empty or
 *   nonempty
 * @return a vector of intervals representing the union of all elements of @p val. The elements in the vector
 *   are sorted by the lower bound, adjacent intervals are collapsed into a single interval
 */
template<typename Left, typename Right>
Intervals<Left, Right>
unionOf(const Intervals<Left, Right>& val) {
  using Type = Interval<Left, Right>;

  Intervals<Left, Right> sorted;
  sorted.reserve(val.size());
  for (const auto& elem : val) {
    if (not elem.empty()) {
      sorted.push_back(elem);
    }
  }

  std::sort(sorted.begin(), sorted.end(), [](const Type& lhs, const Type& rhs) {
    // Return `true` iff `lhs` starts strictly left of `rhs`
    return Left::leftMin(lhs.a, rhs.a) == lhs.a && lhs.a != rhs.a;
  });

  Intervals<Left, Right> ret;

  for (const auto& elem : sorted) {
    if (ret.empty()) {
      ret.push_back(elem);
      continue;
    }

    // Union with the current rightmost interval. `operator|` returns either one merged interval (overlap or
    // adjacent) or two sorted intervals (disjoint). Replace the rightmost with that result
    const auto u = ret.back() | elem;
    ret.pop_back();
    ret.insert(ret.end(), u.begin(), u.end());
  }

  return ret;
}

// `Intervals` types ----------------------------------------------------------------------------------------

/**
 * A vector of #rocket::math::ClosedInterval%s.
 */
template<typename T>
using ClosedIntervals = Intervals<internal::LeftClosed<T>, internal::RightClosed<T>>;

/**
 * A vector of #rocket::math::OpenInterval%s.
 */
template<typename T>
using OpenIntervals = Intervals<internal::LeftOpen<T>, internal::RightOpen<T>>;

/**
 * A vector of #rocket::math::LeftOpenInterval%s.
 */
template<typename T>
using LeftOpenIntervals = Intervals<internal::LeftOpen<T>, internal::RightClosed<T>>;

/**
 * A vector of #rocket::math::RightOpenInterval%s.
 */
template<typename T>
using RightOpenIntervals = Intervals<internal::LeftClosed<T>, internal::RightOpen<T>>;

} // namespace rocket::math

// EOF
