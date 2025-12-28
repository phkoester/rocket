/**
 * @file enum-decl.h
 *
 * Enum utilities: declarations.
 */

#pragma once

#include "format.h"

#include <iosfwd>

// Macros ---------------------------------------------------------------------------------------------------

/// @cond undocumented

#define ROCKET_ENUM_DECLARE_OP_INPUT__(type) ::std::istream& operator>>(::std::istream&, type&)
#define ROCKET_ENUM_DECLARE_OP_OUTPUT__(type) ::std::ostream& operator>>(::std::ostream&, type)

#define ROCKET_ENUM_DECLARE__(type) \
    ROCKET_ENUM_DECLARE_OP_INPUT__(type); \
    ROCKET_ENUM_DECLARE_OP_OUTPUT__(type); \

#define ROCKET_ENUM_DECLARE_FMT_FORMATTER__(type) \
    template<typename Char> \
    struct fmt::formatter<type, Char> { \
      template<typename FormatContext> \
      auto \
      format(type val, FormatContext& ctx) const -> decltype(ctx.out()); \
      \
      constexpr const Char* \
      parse(parse_context<Char>& ctx) { \
        return underlying_.parse(ctx); \
      } \
      private: \
      \
      ::rocket::format::NativeFormatter<string_view, Char> underlying_; \
    };

/// @endcond

/**
 * Provides declarations for the enum @p type needed for full Rocket interoperability.
 *
 * This macro must be called in the enum's namespace.
 *
 * @param type the type of the enum, without namespace, e.g. `MyClass::MyEnum`
 */
#define ROCKET_ENUM_DECLARE(type) ROCKET_ENUM_DECLARE__(type)

/**
 * Declares a `fmt::formatter` specialization for the enum @p type.
 *
 * This macro must be called in the global namespace.
 *
 * @param type the fully-qualified type of the enum, with namespace, e.g. `mynamespace::MyClass::MyEnum`
 */
#define ROCKET_ENUM_DECLARE_FMT_FORMATTER(type) ROCKET_ENUM_DECLARE_FMT_FORMATTER__(type)

// EOF
