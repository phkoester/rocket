/**
 * @file std.h
 *
 * Formatting of standard library types, built on top of {fmt}.
 *
 * Include this file rather than `fmt/ranges.h` or `fmt/std.h` directly.
 */

#pragma once

#include "rocket/Exception.h"
#include "rocket/format/format.h"
#include "rocket/unicode/ConvertTo.h"

/// @attention This requires modifying `fmt/std.h`!
#define FMT_STD_NO_EXCEPTION_FORMATTER
#include "rocket/external/fmt/std.h"

namespace fmt {

// #fmt::formatter<#Exception> ------------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{`Exception`}
 *
 * - If the `?` format specifier is used, then the stack trace is included.
 * - If the `t` format specifier is used, then the type of the exception is included.
 */
template<typename Exception, typename C> requires std::is_base_of_v<std::exception, Exception>
struct formatter<Exception, C> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(const Exception& val, FormatContext& ctx) const{
    // If requested, append type

    auto out = ctx.out();
    if (withType_) {
      const std::string typeName = fmt::format("{}", typeid(val));
      if constexpr (std::is_same_v<C, char>) {
        out = format_to(out, "`{}`: ", rocket::unicode::ConvertTo<C>::apply(typeName));
      } else {
        out = format_to(out, U"`{}`: ", rocket::unicode::ConvertTo<C>::apply(typeName));
      }
    }

    // Append message

    out = detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(rocket::what(val)));

    // If debug, append stack trace

    if (debug_) {
      const auto* const p = dynamic_cast<const rocket::Exception*>(&val);
      if (p && p->stackTrace()) {
        out = detail::write<C>(out, static_cast<C>('\n'));
        std::ostringstream os;
        os << *p->stackTrace();
        std::string str = os.str();
        str.pop_back(); // Remove trailing '\n'
        out = detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(str));
      }
    }
    return out;
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    auto it = ctx.begin();
    auto end = ctx.end();
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

} // namespace fmt

// EOF
