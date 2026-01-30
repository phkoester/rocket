/**
 * @file interval.h
 *
 * Mathematical intervals.
 */

#pragma once

#include "rocket/literal.h"
#include "rocket/type-traits.h"
#include "rocket/format/format.h"
#include "rocket/std/std.h"

#include <algorithm>
#include <optional>
#include <ostream>
#include <type_traits>

namespace rocket::math {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

// #BoundTraits .............................................................................................

template<typename T, typename BoundType_, bool Left_, char Symbol_>
struct BoundTraits {
  using Type = T;
  using BoundType = BoundType_;

  static constexpr bool Left = Left_;
  static constexpr bool Closed = std::is_same_v<BoundType, T>;
  static constexpr char Symbol = Symbol_;

  static constexpr bool
  matches(BoundType bound, Type val) {
    if constexpr (Left) {
      // Left
      if constexpr (Closed) {
        return bound <= val;
      } else {
        return not bound ? true : *bound < val;
      }
    } else {
      // Right
     if constexpr (Closed) {
       return bound >= val;
     } else {
       return not bound ? true : *bound > val;
     }
    }
  }

  static constexpr BoundType
  leftMax(BoundType a, BoundType b) {
    if constexpr (Closed) {
      return std::max(a, b);
    } else {
      // null < anything
      if (not a) {
        return b;
      }
      if (not b) {
        return a;
      }
      return std::max(*a, *b);
    }
  }

  static constexpr BoundType
  leftMin(BoundType a, BoundType b) {
    if constexpr (Closed) {
      return std::min(a, b);
    } else {
      // null < anything
      if (not a) {
        return a;
      }
      if (not b) {
        return b;
      }
      return std::min(*a, *b);
    }
  }

  static constexpr BoundType
  of(i32 val) {
    if constexpr (std::is_same_v<T, BoundType>) {
      return static_cast<T>(val);
    } else {
      return std::optional<T>(static_cast<T>(val));
    }
  }

  static constexpr BoundType
  rightMax(BoundType a, BoundType b) {
    if constexpr (Closed) {
      return std::max(a, b);
    } else {
      // null > anything
      if (not a) {
        return a;
      }
      if (not b) {
        return b;
      }
      return std::max(*a, *b);
    }
  }

  static constexpr BoundType
  rightMin(BoundType a, BoundType b) {
    if constexpr (Closed) {
      return std::min(a, b);
    } else {
      // null > anything
      if (not a) {
        return b;
      }
      if (not b) {
        return a;
      }
      return std::min(*a, *b);
    }
  }
};

template<typename T> using LeftClosed = BoundTraits<T, T, true, '['>;
template<typename T> using LeftOpen = BoundTraits<T, std::optional<T>, true, '('>;
template<typename T> using RightClosed = BoundTraits<T, T, false, ']'>;
template<typename T> using RightOpen = BoundTraits<T, std::optional<T>, false, ')'>;

// #Cardinality .............................................................................................

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

// #IntervalSymbols .........................................................................................

template<typename C> requires IsChar<C>
struct IntervalSymbols;

template<>
struct IntervalSymbols<char> {
  static constexpr auto Empty = "∅";
  static constexpr auto NegativeInfinity = "-∞";
  static constexpr auto PositiveInfinity = "+∞";
};

template<>
struct IntervalSymbols<char32> {
  static constexpr auto Empty = U"∅";
  static constexpr auto NegativeInfinity = U"-∞";
  static constexpr auto PositiveInfinity = U"+∞";
};

// #IntervalTraits ..........................................................................................

template<typename T, typename Left, typename Right>
struct IntervalTraits;

// Closed interval
template<typename T>
struct IntervalTraits<T, LeftClosed<T>, RightClosed<T>> {
  using LeftType = LeftClosed<T>;
  using RightType = RightClosed<T>;

  using CardinalityType = Cardinality<T, LeftType::Closed, RightType::Closed>::Type;
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
struct IntervalTraits<T, LeftOpen<T>, RightOpen<T>> {
  using LeftType = LeftOpen<T>;
  using RightType = RightOpen<T>;

  using CardinalityType = Cardinality<T, LeftType::Closed, RightType::Closed>::Type;
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
struct IntervalTraits<T, LeftOpen<T>, RightClosed<T>> {
  using LeftType = LeftOpen<T>;
  using RightType = RightClosed<T>;

  using CardinalityType = Cardinality<T, LeftType::Closed, RightType::Closed>::Type;
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
struct IntervalTraits<T, LeftClosed<T>, RightOpen<T>> {
  using LeftType = LeftClosed<T>;
  using RightType = RightOpen<T>;

  using CardinalityType = Cardinality<T, LeftType::Closed, RightType::Closed>::Type;
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

// Functions ................................................................................................

template<typename Left, typename Right>
constexpr std::pair<typename Left::BoundType, typename Right::BoundType>
intersectionImpl(
    typename Left::BoundType lhsA, typename Right::BoundType lhsB,
    typename Left::BoundType rhsA, typename Right::BoundType rhsB) {
  return { Left::leftMax(lhsA, rhsA), Right::rightMin(lhsB, rhsB) };
}

template<typename Left, typename Right>
constexpr std::pair<typename Left::BoundType, typename Right::BoundType>
unionImpl(
    typename Left::BoundType lhsA, typename Right::BoundType lhsB,
    typename Left::BoundType rhsA, typename Right::BoundType rhsB) {
  return { Left::leftMin(lhsA, rhsA), Right::rightMax(lhsB, rhsB) };
}

} // namespace internal

// #IntervalImpl --------------------------------------------------------------------------------------------

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

  /// The interval traits.
  using Traits = internal::IntervalTraits<T, Left, Right>;

  /// The element type.
  using Type = T;

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
   * Makes an empty interval.
   */
  constexpr IntervalImpl() : a(Left::of(1_i32)), b(Right::of(0_i32)) {}

  /**
   * @ctor
   *
   * Makes an interval.
   *
   * If #b is less than #a, then the interval is empty.
   *
   * @param a the lower bound. If null, then there is no lower bound
   * @param b the upper bound. If null, then there is no upper bound
   */
  constexpr IntervalImpl(A a, B b) : a(a), b(b) {}

  /// @member_op_eq
  bool
  operator==(const IntervalImpl& rhs) const {
    if (empty() && rhs.empty()) {
      return true;
    }
    return a == rhs.a && b == rhs.b;
  }

  /// @member_op_ne
  inline bool operator!=(const IntervalImpl& rhs) const { return not operator==(rhs); }

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
  constexpr Traits::CardinalityType cardinality() const { return Traits::cardinality(a, b); }

  /**
   * Tests if @p val is contained in the interval.
   *
   * @param val a value of type @p T
   * @return whether @p val is contained in the interval
   */
  constexpr bool contains(T val) const { return Left::matches(a, val) && Right::matches(b, val); }

  /**
   * Tests if the interval is empty.
   *
   * If either #a or #b are null, meaning "infinite", then the interval is nonempty.
   *
   * @return whether the interval is empty
   */
  constexpr bool empty() const { return Traits::empty(a, b); }

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
  constexpr Traits::SizeType size() const { return Traits::size(a, b); }
};

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
  auto pair = internal::intersectionImpl<Left, Right>(lhs.a, lhs.b, rhs.a, rhs.b);
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
 * Two disjoint intervals are merged into one interval, e.g. @f$[5,7] \cup [1,3] = [1,7]@f$. This is not
 * mathematically correct, but useful in many cases. To handle disjoint intervals specifically, test for an
 * intersection beforehands using `operator&`.
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
  auto pair = internal::unionImpl<Left, Right>(lhs.a, lhs.b, rhs.a, rhs.b);
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

} // namespace rocket::math

// #fmt::formatter<#IntervalImpl> ---------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::math::IntervalImpl}
 *
 * This formatter uses the same format specifiers as the underlying formatter for type @p T.
 */
template<typename T, typename Left, typename Right, typename C>
struct fmt::formatter<rocket::math::IntervalImpl<T, Left, Right>, C> {
  /// @cond undocumented

  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const rocket::math::IntervalImpl<T, Left, Right>& val, FormatContext& ctx) const {
    using namespace rocket::math::internal;

    auto out = ctx.out();
    if (val.empty()) {
      // Empty: Use a neat mathematical symbol

      out = detail::write<C>(out, IntervalSymbols<C>::Empty);
    } else {
      // Nonempty

      out = detail::write<C>(out, static_cast<C>(Left::Symbol));
      auto opt = rocket::option(val.a);
      if (not opt) {
        out = detail::write<C>(out, IntervalSymbols<C>::NegativeInfinity);
      } else {
        ctx.advance_to(out);
        out = underlying_.format(*opt, ctx);
      }
      out = detail::write<C>(out, static_cast<C>(','));
      opt = rocket::option(val.b);
      if (not opt) {
        // In interval notation, we prefer `+∞` over `∞`
        out = detail::write<C>(out, IntervalSymbols<C>::PositiveInfinity);
      } else {
        ctx.advance_to(out);
        out = underlying_.format(*opt, ctx);
      }
      out = detail::write<C>(out, static_cast<C>(Right::Symbol));
    }
    return out;
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return underlying_.parse(ctx);
  }

  constexpr void
  set_debug_format(bool val = true) {
    detail::maybe_set_debug_format(underlying_, val);
  }

  /// @endcond

private:

  fmt::formatter<rocket::PurgeType<T>, C> underlying_;
};

namespace rocket::math {

// @op_output{#rocket::math::IntervalImpl}
template<typename T, typename Left, typename Right>
std::ostream&
operator<<(std::ostream& os, const IntervalImpl<T, Left, Right>& val) {
  return os << fmt::format("{}", val);
}

// Interval types -------------------------------------------------------------------------------------------

/**
 * A closed interval @f$[a,b]@f$ contains all elements @f$x@f$ such that @f$a <= x <= b@f$.
 *
 * @tparam T the element type
 */
template<typename T>
using ClosedInterval = IntervalImpl<T, internal::LeftClosed<T>, internal::RightClosed<T>>;

/**
 * An open interval @f$(a,b)@f$ contains all elements @f$x@f$ such that @f$a < x < b@f$.
 *
 * @tparam T the element type
 */
template<typename T>
using OpenInterval = IntervalImpl<T, internal::LeftOpen<T>, internal::RightOpen<T>>;

/**
 * A left-open interval @f$(a,b]@f$ contains all elements @f$x@f$ such that @f$a < x <= b@f$.
 *
 * @tparam T the element type
 */
template<typename T>
using LeftOpenInterval = IntervalImpl<T, internal::LeftOpen<T>, internal::RightClosed<T>>;

/**
 * A right-open interval @f$[a,b)@f$ contains all elements @f$x@f$ such that @f$a <= x < b@f$.
 *
 * @tparam T the element type
 */
template<typename T>
using RightOpenInterval = IntervalImpl<T, internal::LeftClosed<T>, internal::RightOpen<T>>;

} // namespace rocket::math

// EOF
