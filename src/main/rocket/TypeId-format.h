/// @file TypeId-format.h
///
/// Formatting support for #rocket::TypeId.

#pragma once

#include "rocket/TypeId.h"
#include "rocket/type-traits.h"
#include "rocket/format/StringFormatter.h"

#include <fmt/format.h>

// `fmt::formatter<rocket::TypeId>` -------------------------------------------------------------------------

/// @spec_fmt_formatter{#rocket::TypeId}
///
/// The type ID is formatted as its demangled name in backticks, for example
/// <code>`rocket::InvalidArgument`</code>.
///
/// This formatter uses the same format specifiers as the underlying string formatter; they apply to the name
/// including the backticks.
template<typename C> requires rocket::IsChar<C>
struct fmt::formatter<rocket::TypeId, C> : rocket::format::internal::StringFormatter<C> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(const rocket::TypeId& val, FormatContext& ctx) const {
    return this->formatUtf8('`' + val.prettyName() + '`', ctx);
  }

  /// @endcond
};

// EOF
