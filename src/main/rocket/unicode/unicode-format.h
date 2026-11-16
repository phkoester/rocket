/**
 * @file unicode-format.h
 *
 * Formatting support for the Unicode API.
 */

#pragma once

#include "rocket/type-traits.h"
#include "rocket/format/StringFormatter.h"
#include "rocket/unicode/Character.h"
#include "rocket/unicode/unicode.h"

#include <fmt/format.h>

// `fmt::formatter<rocket::unicode::CodePoint>` -------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::unicode::CodePoint}
 *
 * The code point is formatted as `U+` followed by at least four uppercase hexadecimal digits, for example
 * `U+00E4`.
 *
 * This formatter uses the same format specifiers as the underlying string formatter; they apply to the whole
 * string including the `U+`.
 */
template<typename C> requires rocket::IsChar<C>
struct fmt::formatter<rocket::unicode::CodePoint, C> : rocket::format::internal::StringFormatter<C> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(rocket::unicode::CodePoint val, FormatContext& ctx) const {
    // This is simple enough that we don’t need a `FormattedCodec` for it
    return this->formatUtf8(fmt::format("U+{:04X}", static_cast<rocket::u32>(val)), ctx);
  }

  /// @endcond
};

namespace rocket::unicode {

/// @fn_format_as{#rocket::unicode::BasicCharacter}
template<typename C, typename String> requires IsChar<C>
constexpr auto
format_as(const BasicCharacter<C, String>& val) {
  return static_cast<std::basic_string_view<C>>(val);
}

} // namespace rocket::unicode

// EOF
