/**
 * @file Interval.h
 *
 * Mathematical intervals.
 */

#pragma once

#include "rocket/3rdparty/std.h"
#include "rocket/format/format.h"

#include <algorithm>
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
  matches(BoundType bound, Type val) {
    if constexpr (Left) {
      // Left
      if constexpr (Closed)
        return bound <= val;
      else
        return not bound ? true : *bound < val;
    } else {
      // Right
     if constexpr (Closed)
       return bound >= val;
     else
       return not bound ? true : *bound > val;
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

// `IntervalSymbols` ........................................................................................

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

// Functions ................................................................................................

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
  constexpr IntervalImpl() : lower(static_cast<T>(1)), upper(static_cast<T>(0)) {}

  /**
   * @ctor
   *
   * Makes an interval.
   *
   * @param lower the lower bound. If null, then there is no lower bound
   * @param upper the upper bound. If null, then there is no upper bound
   */
  constexpr IntervalImpl(LowerType lower, UpperType upper) : lower(lower), upper(upper) {}

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
   * Tests if @p val is contained in this interval.
   *
   * @param val a value of type @p T
   * @return whether @p val is contained in this interval
   */
  constexpr bool contains(T val) const { return Left::matches(lower, val) && Right::matches(upper, val); }

  /**
   * Tests if this interval is empty.
   *
   * If either #lower or #upper are null, meaning "infinite", then this interval is nonempty.
   *
   * @return whether this interval is empty
   */
  constexpr bool empty() const { return Traits::empty(lower, upper); }

  /**
   * Returns the size of this interval.
   *
   * If either #lower or #upper are null, then the size of the interval is null, meaning "infinite".
   * Otherwise, the size is calculated as @f$upper -lower@f$.
   *
   * @attention A size of 0 doesn't necessarily mean an interval is empty. For instance, the closed interval
   * @f$[2,2]@f$ has a size of 0 and is nonempty. On the other hand, an empty interval always has a size of
   * 0. To check if an interval is empty, use the #empty member function.
   *
   * @return the size of this interval
   */
  constexpr Traits::SizeType size() const { return Traits::size(lower, upper); }
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

} // namespace rocket::math

// `fmt::formatter<IntervalImpl>` ---------------------------------------------------------------------------

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
      auto opt = rocket::option(val.lower);
      if (not opt) {
        out = detail::write<C>(out, IntervalSymbols<C>::NegativeInfinity);
      } else {
        ctx.advance_to(out);
        out = underlying_.format(*opt, ctx);
      }
      out = detail::write<C>(out, static_cast<C>(','));
      opt = rocket::option(val.upper);
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
