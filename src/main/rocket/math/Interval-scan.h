/**
 * @file Interval-scan.h
 *
 * Scanning support for mathematical intervals.
 */

#pragma once

#include "rocket/type-traits.h"
#include "rocket/math/Interval.h"

#include <scn/scan.h>

#include <limits>
#include <optional>
#include <string_view>

// `scn::scanner<rocket::math::Interval>` -------------------------------------------------------------------

/**
 * @spec_scn_scanner{#rocket::math::Interval}
 *
 * The scanner reads what the formatter in `rocket/math/Interval-format.h` writes: `∅` for an empty interval,
 * otherwise the two bounds separated by a comma, enclosed in `[` or `]` for a closed bound and `(` or `)` for
 * an open bound, e.g., `[1,2]`, `(-1.5,3)`, `(-∞,12]`, or `[12,∞)`. Whitespace is skipped before each token.
 * The bounds are scanned with their default scanners.
 *
 * - For an open bound, `-∞` on the left and `∞` on the right denote a missing bound.
 * - For a floating-point bound, `-∞` and `∞` otherwise denote negative and positive infinity.
 * - The bracket types must match the interval type, e.g., `[1,2)` is no valid #rocket::math::ClosedInterval.
 * - No format specifiers are accepted.
 */
template<typename Left, typename Right, typename C> requires rocket::IsChar<C>
struct scn::scanner<rocket::math::Interval<Left, Right>, C> {
  /// @cond undocumented

  using Type = rocket::math::Interval<Left, Right>;
  using Elem = Type::Type;
  using Symbols = rocket::math::internal::interval::IntervalSymbols<C>;

  template<typename ParseContext>
  constexpr ParseContext::iterator
  parse(ParseContext& pctx) {
    return pctx.begin();
  }

  template<typename Context>
  scan_expected<typename Context::iterator>
  scan(Type& val, Context& ctx) const {
    if (auto it = match(ctx, Symbols::Empty); it) {
      val = Type();
      return *it;
    }

    Type ret;
    if (auto result = expect(ctx, Left::Symbol); not result) {
      return unexpected(result.error());
    }
    if (auto result = scanBound<Left>(ret.a, ctx, Symbols::NegativeInfinity); not result) {
      return unexpected(result.error());
    }
    if (auto result = expect(ctx, ','); not result) {
      return unexpected(result.error());
    }
    if (auto result = scanBound<Right>(ret.b, ctx, Symbols::Infinity); not result) {
      return unexpected(result.error());
    }
    if (auto result = expect(ctx, Right::Symbol); not result) {
      return unexpected(result.error());
    }
    val = ret;
    return ctx.begin();
  }

  /// @endcond

private:

  /**
   * Skips whitespace and matches @p str. On success, returns the iterator past @p str, without advancing
   * @p ctx.
   */
  template<typename Context>
  static std::optional<typename Context::iterator>
  match(Context& ctx, std::basic_string_view<C> str) {
    auto skipped = detail::internal_skip_classic_whitespace(ctx.range(), true);
    if (not skipped) {
      return std::nullopt;
    }
    auto it = *skipped;
    for (const C c : str) {
      if (it == ctx.end() || *it != c) {
        return std::nullopt;
      }
      ++it;
    }
    return it;
  }

  /// Skips whitespace and consumes the character @p c, which must follow.
  template<typename Context>
  static scan_expected<void>
  expect(Context& ctx, char c) {
    const C ch = static_cast<C>(c);
    if (auto it = match(ctx, std::basic_string_view<C>(&ch, 1)); it) {
      ctx.advance_to(*it);
      return {};
    }
    auto skipped = detail::internal_skip_classic_whitespace(ctx.range(), true);
    if (not skipped || *skipped == ctx.end()) {
      return unexpected(scan_error(scan_error::end_of_input, "unexpected end of interval"));
    }
    return unexpected(scan_error(scan_error::invalid_scanned_value, "invalid interval"));
  }

  /**
   * Scans a bound. @p unbounded is the notation of a missing bound, which is only valid for an open bound.
   */
  template<typename Traits, typename Context>
  static scan_expected<void>
  scanBound(typename Traits::BoundType& bound, Context& ctx, std::basic_string_view<C> unbounded) {
    if constexpr (not Traits::IsClosed) {
      if (auto it = match(ctx, unbounded); it) {
        bound = std::nullopt;
        ctx.advance_to(*it);
        return {};
      }
    }

    if constexpr (rocket::IsFloat<Elem>) {
      using Limits = std::numeric_limits<Elem>;
      if (auto it = match(ctx, Symbols::NegativeInfinity); it) {
        bound = -Limits::infinity();
        ctx.advance_to(*it);
        return {};
      }
      if (auto it = match(ctx, Symbols::Infinity); it) {
        bound = Limits::infinity();
        ctx.advance_to(*it);
        return {};
      }
    }

    Elem elem {};
    auto result = scn::scanner<Elem, C>().scan(elem, ctx);
    if (not result) {
      return unexpected(result.error());
    }
    ctx.advance_to(*result);
    bound = elem;
    return {};
  }
};

// EOF
