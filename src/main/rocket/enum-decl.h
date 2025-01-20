/**
 * @file enum-decl.h
 *
 * Enum utilities: declarations.
 */

#pragma once

#include <format>
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

#define ROCKET_ENUM_DECLARE_STD_FORMATTER__(type) \
  namespace std { \
  \
  template <> \
  struct formatter<type> : formatter<string_view> { \
    format_context::iterator format(type v, format_context& ctx) const; \
  }; \
  \
  }

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
 * Declares a `std::formatter` specialization for the enum @p type.
 *
 * This macro must be called in the global namespace.
 *
 * @param type the fully-qualified type of the enum, with namespace, e.g. `mynamespace::MyClass::MyEnum`
 */
#define ROCKET_ENUM_DECLARE_STD_FORMATTER(type) ROCKET_ENUM_DECLARE_STD_FORMATTER__(type)

// EOF
