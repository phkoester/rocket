/**
 * @file enum-decl.h
 *
 * Enum utilities: declarations.
 */

#pragma once

#include "format.h"

#include <iosfwd>
#include <type_traits>

// Macros ---------------------------------------------------------------------------------------------------

/// @cond undocumented

// Local ....................................................................................................

#define ROCKET_ENUM_DECLARE_OP_OUTPUT__(type) ::std::ostream& operator<<(::std::ostream&, type)

#define ROCKET_ENUM_DECLARE_LOCAL__(type) \
    ROCKET_ENUM_DECLARE_OP_OUTPUT__(type)

// Global ...................................................................................................

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
    \
    private: \
      \
      ::rocket::format::NativeFormatter<string_view, Char> underlying_; \
    }

#define ROCKET_ENUM_DECLARE_ROCKET_ENUM__(type) \
    template<> \
    struct rocket::Enum<type> { \
      static type toType(::std::string_view s); \
    }

#define ROCKET_ENUM_DECLARE_GLOBAL__(type) \
    ROCKET_ENUM_DECLARE_FMT_FORMATTER__(type); \
    ROCKET_ENUM_DECLARE_ROCKET_ENUM__(type)

/// @endcond

/**
 * Provides declarations for the enum @p type needed for full Rocket interoperability.
 *
 * This macro must be called in the enum's local namespace.
 *
 * @param type the type of the enum, without namespace, e.g. `MyClass::MyEnum`
 */
#define ROCKET_ENUM_DECLARE_LOCAL(type) ROCKET_ENUM_DECLARE_LOCAL__(type)

/**
 * Provides declarations for the enum @p type needed for full Rocket interoperability.
 *
 * This macro must be called in the global namespace.
 *
 * @param type the fully-qualified type of the enum, with namespace, e.g. `mynamespace::MyClass::MyEnum`
 */
#define ROCKET_ENUM_DECLARE_GLOBAL(type) ROCKET_ENUM_DECLARE_GLOBAL__(type)

namespace rocket {

// `Enum` ---------------------------------------------------------------------------------------------------

/**
 * A class template for Rocket enums, providing some additional information about an enum.
 */
template<typename E> requires std::is_enum_v<E>
struct Enum;

} // namespace rocket

// EOF
