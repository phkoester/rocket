/**
 * @file format-std.h
 *
 * Formatting of standard library types, built on top of {fmt}.
 */

#pragma once

#include <fmt/format.h>

#include <optional>

namespace fmt {

// `std::optional` ------------------------------------------------------------------------------------------

template<typename T, typename Char>
struct formatter<std::optional<T>, Char, std::enable_if_t<is_formattable<T, Char>::value>> {
  FMT_CONSTEXPR auto parse(parse_context<Char>& ctx) {
    // XXX detail::maybe_set_debug_format(underlying_, true);
    return underlying_.parse(ctx);
  }

  template <typename FormatContext>
  auto
  format(const std::optional<T>& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    if (not v) {
      return detail::write<Char>(ctx.out(), NONE);
    }
    return underlying_.format(*v, ctx);
  }

private:

  static constexpr basic_string_view<Char> NONE = detail::string_literal<Char, 'n', 'o', 'n', 'e'>{};

  formatter<std::remove_cv_t<T>, Char> underlying_;
};

}

// EOF
