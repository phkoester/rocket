/**
 * @file format.h
 */

#pragma once

#include "rocket/io/io.h"
#include "rocket/type-traits.h"
#include "rocket/unicode/ConvertTo.h"

#include <fmt/format.h>

#ifdef ROCKET_OS_WINDOWS

// `fmt::formatter<i128>` -----------------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{#i128}
 *
 * This formatter uses the same format specifiers as the underlying string formatter.
 */
template<typename C> requires rocket::IsChar<C>
struct fmt::formatter<i32, C> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(i128 val, FormatContext& ctx) const {
    std::ostringstream os;
    os << val;
    auto str = os.str();
    return underlying_.format(rocket::unicode::ConvertTo<C>::apply(str), ctx);
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return underlying_.parse(ctx);
  }

  constexpr void
  set_debug_format(bool val = true) {
    underlying_.set_debug_format(val);
  }

  /// @endcond

private:

  fmt::formatter<basic_string_view<C>, C> underlying_;
};

// `fmt::formatter<u128>` -----------------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{#u128}
 *
 * This formatter uses the same format specifiers as the underlying string formatter.
 */
template<typename C> requires rocket::IsChar<C>
struct fmt::formatter<u128, C> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(u128 val, FormatContext& ctx) const {
    std::ostringstream os;
    os << val;
    auto str = os.str();
    return underlying_.format(rocket::unicode::ConvertTo<C>::apply(str), ctx);
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return underlying_.parse(ctx);
  }

  constexpr void
  set_debug_format(bool val = true) {
    underlying_.set_debug_format(val);
  }

  /// @endcond

private:

  fmt::formatter<basic_string_view<C>, C> underlying_;
};

#endif

// EOF
