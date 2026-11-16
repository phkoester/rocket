/**
 * @file scan-bimap.h
 *
 * Scanning support for Bimaps.
 */

#pragma once

#include "rocket/bimap.h"

#include <scn/ranges.h>
#include <scn/scan.h>

#include <string_view>
#include <type_traits>
#include <utility>

// `scn::range_format_kind<boost::bimaps::bimap>` -----------------------------------------------------------

/// Keeps the range scanners of scnlib away from Bimaps, which are scanned by the scanner below.
template<typename A, typename B, typename C>
struct scn::range_format_kind<boost::bimaps::bimap<A, B>, C> :
  std::integral_constant<scn::range_format, scn::range_format::disabled> {};

// `scn::scanner<boost::bimaps::bimap>` ---------------------------------------------------------------------

/**
 * @spec_scn_scanner{boost::bimaps::bimap}
 *
 * The scanner scans the left view of the bimap, just as scnlib scans a #std::map: as `{k1: v1, k2: v2}`.
 * Whitespace is skipped before the braces, the commas, and the colons. Keys and values are scanned with
 * their default scanners, without format specifiers.
 *
 * Like inserting into a #std::map, an element is dropped if its key, or with a bimap also its value, is
 * already in the bimap.
 *
 * - Like for a #std::map, no format specifiers are accepted.
 */
template<typename A, typename B, typename C>
struct scn::scanner<boost::bimaps::bimap<A, B>, C> {
  /// @cond undocumented

  using Type = boost::bimaps::bimap<A, B>;
  using Key = std::remove_cvref_t<typename Type::left_key_type>;
  using Value = std::remove_cvref_t<typename Type::left_data_type>;

  template<typename ParseContext>
  constexpr ParseContext::iterator
  parse(ParseContext& pctx) {
    return pctx.begin();
  }

  template<typename Context>
  scan_expected<typename Context::iterator>
  scan(Type& val, Context& ctx) const {
    static constexpr C open[] = { C('{') };
    static constexpr C close[] = { C('}') };
    static constexpr C sep[] = { C(',') };
    static constexpr C keySep[] = { C(':') };

    const auto scanStr = [&ctx](const C* str) -> scan_expected<void> {
      auto result = detail::scan_str(ctx.range(), std::basic_string_view<C>(str, 1));
      if (not result) {
        return unexpected(result.error());
      }
      ctx.advance_to(*result);
      return {};
    };

    if (auto result = scanStr(open); not result) {
      return unexpected(result.error());
    }
    for (bool first = true; ; first = false) {
      // Peek at the closing brace; it is consumed after the loop
      if (detail::scan_str(ctx.range(), std::basic_string_view<C>(close, 1))) {
        break;
      }
      if (not first) {
        if (auto result = scanStr(sep); not result) {
          return unexpected(result.error());
        }
      }
      Key k {};
      if (auto result = scn::scanner<Key, C>().scan(k, ctx); result) {
        ctx.advance_to(*result);
      } else {
        return unexpected(result.error());
      }
      if (auto result = scanStr(keySep); not result) {
        return unexpected(result.error());
      }
      Value v {};
      if (auto result = scn::scanner<Value, C>().scan(v, ctx); result) {
        ctx.advance_to(*result);
      } else {
        return unexpected(result.error());
      }
      val.insert({ std::move(k), std::move(v) }); // `bimap` has no `emplace`
    }
    return detail::scan_str(ctx.range(), std::basic_string_view<C>(close, 1));
  }

  /// @endcond
};

// EOF
