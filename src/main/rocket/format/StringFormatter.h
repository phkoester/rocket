/**
 * @file StringFormatter.h
 *
 * Building blocks for {fmt} formatters of types that are formatted as UTF-8 strings.
 */

#pragma once

#include "rocket/type-traits.h"
#include "rocket/unicode/ConvertTo.h"

#include <fmt/format.h>

#include <string_view>

/// @cond undocumented

namespace rocket::internal::format {

/**
 * Writes a UTF-8 string to an output iterator, converted to the character type @p C.
 *
 * @tparam C the character type
 * @tparam Out the type of the output iterator
 * @param out the output iterator
 * @param str the UTF-8 string
 * @return the output iterator past the written characters
 */
template<typename C, typename Out> requires IsChar<C>
Out
writeUtf8(Out out, std::string_view str) {
  const auto converted = unicode::ConvertTo<C>::apply(str);
  // Unlike `std::ranges::copy`, this appends to {fmt}'s buffers in bulk instead of character by character
  const std::basic_string_view<C> view(converted);
  return fmt::detail::copy<C>(view.data(), view.data() + view.size(), out);
}

/**
 * Parses format specifications that consist of single-character flags, which may appear in any order.
 *
 * @tparam C the character type
 * @tparam F the type of the flag handler
 * @param ctx the parse context
 * @param onFlag a function that is called with each flag and returns whether the flag is valid
 * @return an iterator to the end of the format specifications
 */
template<typename C, typename F> requires IsChar<C>
constexpr const C*
parseFlags(fmt::parse_context<C>& ctx, F onFlag) {
  auto it = ctx.begin();
  const auto end = ctx.end();
  for (; it != end && *it != '}'; ++it) {
    if (not onFlag(*it)) {
      fmt::report_error("invalid format specifier");
    }
  }
  return it;
}

/**
 * A base for formatters of types that are formatted as UTF-8 strings.
 *
 * The derived formatter converts its value to a UTF-8 string and passes it to `formatUtf8`. Format
 * specifications, including the debug format, are those of the {fmt} string formatter and apply to the whole
 * string.
 *
 * @tparam C the character type
 */
template<typename C> requires IsChar<C>
class StringFormatter {
public:

  constexpr const C*
  parse(fmt::parse_context<C>& ctx) {
    return formatter_.parse(ctx);
  }

  constexpr void
  set_debug_format(bool val = true) {
    formatter_.set_debug_format(val);
  }

protected:

  template<typename FormatContext>
  FormatContext::iterator
  formatUtf8(std::string_view str, FormatContext& ctx) const {
    const auto converted = unicode::ConvertTo<C>::apply(str);
    return formatter_.format(std::basic_string_view<C>(converted), ctx);
  }

private:

  fmt::formatter<std::basic_string_view<C>, C> formatter_;
};

} // namespace rocket::internal::format

/// @endcond

// EOF
