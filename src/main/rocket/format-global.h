/**
 * @file format-global.h
 *
 * Formatting of native types in the global namespace.
 */

#pragma once

#include "format.h"
#include "unicode.h"

#include <iostream> // XXX

namespace fmt {

// `char32_t` -----------------------------------------------------------------------------------------------

template<typename Char>
struct formatter<char32_t, Char> {
  template<typename FormatContext>
  constexpr auto
  format(char32_t v, FormatContext& ctx) const -> decltype(ctx.out()) {
    // Convert char32_t to UTF-8 string
    std::string s = static_cast<std::string>(rocket::unicode::CodePoint(v));

    // Let the underlying formatter format the string
    auto begin = ctx.begin();
    auto out = ctx.out();
    out = underlying_.format(s, ctx);

    // Debug?
    if (underlying_.specs().type() == presentation_type::debug) {
      // Replace double quotes by single quotes
      std::cout << "XXX Peek begin: [" << *begin << "]" << std::endl;
      std::cout << "XXX Peek out - 1: [" << *(out - 1) << "]" << std::endl;
    }

    return out;
  }

  constexpr const Char*
  parse(parse_context<Char>& ctx) {
    return underlying_.parse(ctx);
  }

private:

  rocket::format::NativeFormatter<string_view, Char> underlying_;
};

} // namespace fmt

// EOF
