/**
 * @file enum-decl.h
 *
 * Enum utilities: declarations.
 */

#pragma once

#include <fmt/format.h>

#include <iosfwd>

// Macros ---------------------------------------------------------------------------------------------------

/// @cond undocumented

#define ROCKET_ENUM_DECLARE_OP_INPUT__(type) ::std::istream& operator>>(::std::istream&, type&)
#define ROCKET_ENUM_DECLARE_OP_OUTPUT__(type) ::std::ostream& operator<<(::std::ostream&, type)
#define ROCKET_ENUM_DECLARE_FN_PARSE_RON__(type) ::std::istream& parseRon(::std::istream&, type&)
#define ROCKET_ENUM_DECLARE_FN_PRINT_RON__(type) ::std::ostream& printRon(::std::ostream&, type)

#define ROCKET_ENUM_DECLARE__(type) \
    ROCKET_ENUM_DECLARE_OP_INPUT__(type); \
    ROCKET_ENUM_DECLARE_OP_OUTPUT__(type); \
    ROCKET_ENUM_DECLARE_FN_PARSE_RON__(type); \
    ROCKET_ENUM_DECLARE_FN_PRINT_RON__(type) \

#define ROCKET_ENUM_DECLARE_FMT_FORMATTER__(type) \
    template<> \
    struct fmt::formatter<type> : ::rocket::format::NativeFormatter<string_view> { \
      using Base = ::rocket::format::NativeFormatter<string_view>; \
      \
      template<typename FormatContext> \
      auto \
      format(type val, FormatContext& ctx) const -> decltype(ctx.out()); \
      \
      constexpr const char* \
      parse(fmt::parse_context<char>& ctx) { \
        return Base::parse(ctx); \
      } \
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
