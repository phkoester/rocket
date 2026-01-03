/**
 * @file std.h
 *
 * Formatting of standard library types, built on top of {fmt}.
 */

#pragma once

#include "rocket/format/format.h"

/// This requires modifying `fmt/std.h`!
#define FMT_STD_NO_OPTIONAL
/// This requires modifying `fmt/std.h`!
#define FMT_STD_NO_VARIANT

#include <fmt/ranges.h>
#include <fmt/std.h>

#include <optional>

namespace fmt {

// `std::optional` ------------------------------------------------------------------------------------------

template<typename T, typename C>
struct formatter<std::optional<T>, C, std::enable_if_t<is_formattable<T, C>::value>> {
  template <typename FormatContext>
  constexpr FormatContext::iterator
  format(const std::optional<T>& v, FormatContext& ctx) const {
    if (not v) {
      return detail::write<C>(ctx.out(), "<none>");
    }
    return underlying_.format(*v, ctx);
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return underlying_.parse(ctx);
  }

private:

  formatter<T, C> underlying_;
};

// `std::variant` ------------------------------------------------------------------------------------------

template<typename T>
struct is_variant_like {
  static constexpr bool value = detail::is_variant_like_<T>::value;
};

template<typename C>
struct formatter<std::monostate, C> {
  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const std::monostate&, FormatContext& ctx) const {
    return detail::write<C>(ctx.out(), "<monostate>");
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return ctx.begin();
  }
};

template <typename Variant, typename C>
struct formatter<Variant, C, std::enable_if_t<
    std::conjunction_v<
        is_variant_like<Variant>,
        detail::is_variant_formattable<Variant, C>>>> {
  template <typename FormatContext>
  constexpr FormatContext::iterator
  format(const Variant& value, FormatContext& ctx) const {
    auto out = ctx.out();
    try {
      std::visit([&](const auto& v) {
        // We need the index to be able to parse the variant back
        out = format_to(out, "{}:", value.index());
        out = detail::write_escaped_alternative<C>(out, v, ctx);
      }, value);
    }
    catch (const std::bad_variant_access&) {
      out = detail::write<C>(out, "<invalid>");
    }
    return out;
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return ctx.begin();
  }
};

} // namespace fmt

// EOF
