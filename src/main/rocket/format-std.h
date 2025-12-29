/**
 * @file format-std.h
 *
 * Formatting of standard library types, built on top of {fmt}.
 */

#pragma once

#include "format.h"

/// This requires modifying `fmt/std.h`!
#define FMT_STD_NO_OPTIONAL
/// This requires modifying `fmt/std.h`!
#define FMT_STD_NO_VARIANT

#include <fmt/ranges.h>
#include <fmt/std.h>

#include <optional>

namespace fmt {

// `std::optional` ------------------------------------------------------------------------------------------

template<typename T, typename Char>
struct formatter<std::optional<T>, Char, std::enable_if_t<is_formattable<T, Char>::value>> {
  template <typename FormatContext>
  constexpr auto
  format(const std::optional<T>& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    if (not v) {
      return detail::write<Char>(ctx.out(), "<none>");
    }
    return underlying_.format(*v, ctx);
  }

  constexpr const Char*
  parse(parse_context<Char>& ctx) {
    return underlying_.parse(ctx);
  }

private:

  formatter<T, Char> underlying_;
};

// `std::variant` ------------------------------------------------------------------------------------------

template<typename T> struct is_variant_like {
  static constexpr bool value = detail::is_variant_like_<T>::value;
};

template<typename Char> struct formatter<std::monostate, Char> {
  template<typename FormatContext>
  constexpr auto
  format(const std::monostate&, FormatContext& ctx) const -> decltype(ctx.out()) {
    return detail::write<Char>(ctx.out(), "<monostate>");
  }

  constexpr const Char*
  parse(parse_context<Char>& ctx) {
    return ctx.begin();
  }
};

template <typename Variant, typename Char>
struct formatter<Variant, Char, std::enable_if_t<
    std::conjunction_v<
        is_variant_like<Variant>,
        detail::is_variant_formattable<Variant, Char>>>> {
  template <typename FormatContext>
  constexpr auto
  format(const Variant& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    auto out = ctx.out();
    try {
      std::visit([&](const auto& v) {
        // We need the index to be able to parse the variant back
        out = format_to(out, "{}:", value.index());
        out = detail::write_escaped_alternative<Char>(out, v, ctx);
      }, value);
    }
    catch (const std::bad_variant_access&) {
      out = detail::write<Char>(out, "<invalid>");
    }
    return out;
  }

  constexpr const Char*
  parse(parse_context<Char>& ctx) {
    return ctx.begin();
  }
};

} // namespace fmt

// EOF
