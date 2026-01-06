/**
 * @file enum-decl.h
 *
 * Enum utilities: declarations.
 */

#pragma once

#include "rocket/UnorderedBimap.h"
#include "rocket/format/format.h"
#include "rocket/unicode/ConvertTo.h"

#include <iosfwd>
#include <type_traits>

// Macros ---------------------------------------------------------------------------------------------------

/// @cond undocumented

#define ROCKET_ENUM_DECLARE_MAP__(type, name) \
    extern const ::rocket::UnorderedBimap<type, ::std::string_view> name##Map__;

#define ROCKET_ENUM_DECLARE_OP_OUTPUT__(type) \
    ::std::ostream& operator<<(::std::ostream&, type);

#define ROCKET_ENUM_DECLARE_FMT_FORMATTER__(ns, type, name) \
    template<typename C> \
    struct fmt::formatter<ns::type, C> { \
      template<typename FormatContext> \
      constexpr FormatContext::iterator \
      format(ns::type v, FormatContext& ctx) const { \
        if (auto it = ns::name##Map__.left.find(v); it != ns::name##Map__.left.end()) { \
          return underlying_.format(::rocket::unicode::ConvertTo<C>().apply(it->second), ctx); \
        } else { \
          return detail::write<C>(ctx.out(), INVALID); \
        } \
      } \
      \
      constexpr const C* \
      parse(parse_context<C>& ctx) { \
        return underlying_.parse(ctx); \
      } \
      \
      constexpr void \
      set_debug_format(bool v = true) { \
        underlying_.set_debug_format(v); \
      } \
    \
    private: \
      \
      static constexpr basic_string_view<C> INVALID = \
          detail::string_literal<C, '<', 'i', 'n', 'v', 'a', 'l', 'i', 'd', '>'> {}; \
      \
      ::fmt::formatter<basic_string_view<C>, C> underlying_; \
    }

#define ROCKET_ENUM_DECLARE_ROCKET_ENUM__(type) \
    template<> \
    struct rocket::Enum<type> : std::true_type { \
      static type toType(::std::string_view s); \
    }

#define ROCKET_ENUM_DECLARE__(ns, type, name) \
    ROCKET_NS_BEGIN(ns); \
    ROCKET_ENUM_DECLARE_MAP__(type, name); \
    ROCKET_ENUM_DECLARE_OP_OUTPUT__(type); \
    ROCKET_NS_END(ns); \
    ROCKET_ENUM_DECLARE_FMT_FORMATTER__(ns, type, name); \
    ROCKET_ENUM_DECLARE_ROCKET_ENUM__(ns::type)

/// @endcond

/**
 * Provides all the declarations for the enum @p type needed for full Rocket interoperability.
 *
 * In particular, it provides
 *
 * - an output operator for `std::ostream`,
 * - a `fmt::formatter` specialization for the enum,
 * - a #rocket::Enum specialization so #rocket::str::StringConvert may be used with the enum.
 *
 * @note This macro must be called in the global namespace.
 *
 * @param ns the namespace of the enum, e.g. `mynamespace`. May be left empty if the enum is in the global
 *     namespace
 * @param type the type of the enum, without namespace, e.g. `MyClass::MyEnum`
 * @param name the name to use for generated identifiers, e.g. `MyClass_MyEnum`
 */
#define ROCKET_ENUM_DECLARE(ns, type, name) ROCKET_ENUM_DECLARE__(ns, type, name)

namespace rocket {

// `Enum` ---------------------------------------------------------------------------------------------------

/**
 * A class template for Rocket enums, providing some additional information about an enum.
 */
template<typename E> requires std::is_enum_v<E>
struct Enum : std::false_type {};

} // namespace rocket

// EOF
