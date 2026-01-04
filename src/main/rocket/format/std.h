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

// `fmt::formatter<std::optional>` --------------------------------------------------------------------------

template<typename T, typename C>
struct fmt::formatter<std::optional<T>, C, std::enable_if_t<fmt::is_formattable<T, C>::value>> {
  template <typename FormatContext>
  constexpr FormatContext::iterator
  format(const std::optional<T>& v, FormatContext& ctx) const {
    if (not v) {
      return detail::write<C>(ctx.out(), NONE);
    }
    return underlying_.format(*v, ctx);
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return underlying_.parse(ctx);
  }

  constexpr void
  set_debug_format(bool v = true) {
    detail::maybe_set_debug_format(underlying_, v);
  }

private:

  static constexpr basic_string_view<C> NONE =
      detail::string_literal<C, '<', 'n', 'o', 'n', 'e', '>'> {};

  formatter<std::remove_cvref_t<T>, C> underlying_;
};

// `fmt::formatter<std::monostate>` -------------------------------------------------------------------------

template<typename C>
struct fmt::formatter<std::monostate, C> {
  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const std::monostate&, FormatContext& ctx) const {
    return detail::write<C>(ctx.out(), MONOSTATE);
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return ctx.begin();
  }

private:

  static constexpr basic_string_view<C> MONOSTATE =
      detail::string_literal<C, '<', 'm', 'o', 'n', 'o', 's', 't', 'a', 't', 'e', '>'> {};
};

// `fmt::formatter<Variant>` --------------------------------------------------------------------------------

template<typename Variant, typename C>
struct fmt::formatter<Variant, C, std::enable_if_t<
    std::conjunction_v<
        fmt::is_variant_like<Variant>,
        fmt::detail::is_variant_formattable<Variant, C>>>> {
  template <typename FormatContext>
  constexpr FormatContext::iterator
  format(const Variant& value, FormatContext& ctx) const {
    auto out = ctx.out();
    try {
      std::visit([&](const auto& v) {
        // We need the index to be able to parse the variant back
        if constexpr (std::is_same_v<C, char>) {
          out = format_to(out, "{}:", value.index());
        } else {
          out = format_to(out, U"{}:", value.index());
        }

        if (debug_) {
          out = detail::write_escaped_alternative<C>(out, v, ctx);
        } else {
          ctx.advance_to(out);
          formatter<std::remove_cvref_t<decltype(v)>, C> underlying;
          out = underlying.format(v, ctx);
        }
      }, value);
      return out;
    }
    catch (const std::bad_variant_access&) {
      return detail::write<C>(out, INVALID);
    }
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it == '?') {
      debug_ = true;
      ++it;
    }
    return it;
  }

  constexpr void
  set_debug_format(bool v = true) {
    debug_ = v;
  }

private:

  static constexpr basic_string_view<C> INVALID =
      detail::string_literal<C, '<', 'i', 'n', 'v', 'a', 'l', 'i', 'd', '>'> {};

  bool debug_ = false;
};

// EOF
