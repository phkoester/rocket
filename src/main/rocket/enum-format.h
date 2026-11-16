/**
 * @file enum-format.h
 *
 * Formatting support for Rocket enums.
 */

#pragma once

#include "rocket/enum-fwd.h"
#include "rocket/type-traits.h"
#include "rocket/format/StringFormatter.h"

#include <fmt/format.h>

// `fmt::formatter<rocket::Enum>` ---------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::Enum}
 *
 * This formatter uses the same format specifiers as the underlying string formatter.
 */
template<typename E, typename C> requires rocket::Enum<E>::value && rocket::IsChar<C>
struct fmt::formatter<E, C> : rocket::format::internal::StringFormatter<C> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(E val, FormatContext& ctx) const {
    return this->formatUtf8(rocket::Enum<E>::toString(val), ctx); // This may throw, which is okay
  }

  /// @endcond
};

// EOF
