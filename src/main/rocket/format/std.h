/**
 * @file std.h
 *
 * Formatting of standard library types, built on top of {fmt}.
 *
 * Include this file rather than `fmt/ranges.h` or `fmt/std.h` directly.
 */

#pragma once

#include "rocket/Exception.h"
#include "rocket/Type.h"
#include "rocket/format/format.h"

/// @attention This requires modifying `<fmt/std.h>`!
#define FMT_STD_NO_EXCEPTION_FORMATTER
/// @attention This requires modifying `<fmt/std.h>`!
#define FMT_STD_NO_OPTIONAL_FORMATTER
/// @attention This requires modifying `<fmt/std.h>`!
#define FMT_STD_NO_VARIANT_FORMATTER

#include "rocket/3rdparty/fmt/std.h"

#include <optional>

namespace fmt {

// `fmt::formatter<Exception>` ------------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{`Exception`}
 *
 * - If the `?` format specifier is used, then the stack trace is included.
 * - If the `t` format specifier is used, then the type of the exception is included.
 */
template<typename Exception, typename C> requires std::is_base_of<std::exception, Exception>::value
struct formatter<Exception, C> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(const Exception& val, FormatContext& ctx) const{
    // If requested, append type

    auto out = ctx.out();
    if (withType_) {
      auto type = rocket::Type::of(val);
      if constexpr (std::is_same_v<C, char>) {
        out = format_to(out, "`{}`: ", rocket::unicode::ConvertTo<C>().apply(type.name()));
      } else {
        out = format_to(out, U"`{}`: ", rocket::unicode::ConvertTo<C>().apply(type.name()));
      }
    }

    // Append message

    out = detail::write<C>(out, rocket::unicode::ConvertTo<C>().apply(rocket::what(val)));

    // If debug, append stack trace

    if (debug_) {
      const rocket::Exception* p = dynamic_cast<const rocket::Exception*>(&val);
      if (p && p->stackTrace()) {
        out = detail::write<C>(out, static_cast<C>('\n'));
        std::ostringstream os;
        os << *p->stackTrace();
        std::string str = os.str();
        str.pop_back(); // Remove trailing '\n'
        out = detail::write<C>(out, rocket::unicode::ConvertTo<C>().apply(str));
      }
    }
    return out;
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it == '?') {
      debug_ = true;
      ++it;
    }
    if (it != end && *it == 't') {
      withType_ = true;
      ++it;
    }
    return it;
  }

  constexpr void
  set_debug_format(bool val = true) {
    debug_ = val;
  }

  /// @endcond

private:

  bool debug_ = false;
  bool withType_ = false;
};

// `fmt::formatter<std::optional>` --------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{`std::optional`}
 *
 * This formatter uses the same format specifiers as the underlying formatter for type @p T.
 */
template<typename T, typename C> requires is_formattable<T, C>::value
struct formatter<std::optional<T>, C> {
  /// @cond undocumented

  template <typename FormatContext>
  constexpr FormatContext::iterator
  format(const std::optional<T>& val, FormatContext& ctx) const {
    if (not val) {
      return detail::write<C>(ctx.out(), NONE);
    }
    return underlying_.format(*val, ctx);
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

  static constexpr basic_string_view<C> NONE =
      detail::string_literal<C, '<', 'n', 'o', 'n', 'e', '>'> {};

  formatter<rocket::PurgeType<T>, C> underlying_;
};

// `fmt::formatter<std::monostate>` -------------------------------------------------------------------------

/// @spec_fmt_formatter{`std::monostate`}
template<typename C>
struct formatter<std::monostate, C> {
  /// @cond undocumented

  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const std::monostate&, FormatContext& ctx) const {
    return detail::write<C>(ctx.out(), MONOSTATE);
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return ctx.begin();
  }

  /// @endcond

private:

  static constexpr basic_string_view<C> MONOSTATE =
      detail::string_literal<C, '<', 'm', 'o', 'n', 'o', 's', 't', 'a', 't', 'e', '>'> {};
};

// `fmt::formatter<Variant>` --------------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{`Variant`}
 *
 * - If the `?` format specifier is used, then the underlying formatter is set to debug mode.
 */
template<typename Variant, typename C> requires std::conjunction_v<
    is_variant_like<Variant>,
    detail::is_variant_formattable<Variant, C>>
struct formatter<Variant, C> {
  /// @cond undocumented

  template <typename FormatContext>
  constexpr FormatContext::iterator
  format(const Variant& value, FormatContext& ctx) const {
    auto out = ctx.out();
    try {
      std::visit([&](const auto& val) {
        // We need the index to be able to parse the variant back
        if constexpr (std::is_same_v<C, char>) {
          out = format_to(out, "{}:", value.index());
        } else {
          out = format_to(out, U"{}:", value.index());
        }

        formatter<rocket::PurgeType<decltype(val)>, C> underlying;
        detail::maybe_set_debug_format(underlying, debug_);
        ctx.advance_to(out);
        out = underlying.format(val, ctx);
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
  set_debug_format(bool val = true) {
    debug_ = val;
  }

  /// @endcond

private:

  static constexpr basic_string_view<C> INVALID =
      detail::string_literal<C, '<', 'i', 'n', 'v', 'a', 'l', 'i', 'd', '>'> {};

  bool debug_ = false;
};

} // namespace fmt

// EOF
