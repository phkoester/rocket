/**
 * @file enum.h
 *
 * Enum utilities.
 */

#pragma once

#include "rocket/enum-decl.h"

#include "rocket/Exception.h"
#include "rocket/Type.h"
#include "container/container.h"
#include "rocket/message/message.h"

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

// Macros ---------------------------------------------------------------------------------------------------

/// @cond undocumented

// Local ....................................................................................................

#define ROCKET_ENUM_DEFINE_MAP_ELEM__(r, data, elem) { data::elem, BOOST_PP_STRINGIZE(elem) },

#define ROCKET_ENUM_DEFINE_MAP__(type, name, seq) \
    const auto name##Map__ = ::rocket::container::makeUnorderedBimap<type, ::std::string_view>({ \
      BOOST_PP_SEQ_FOR_EACH(ROCKET_ENUM_DEFINE_MAP_ELEM__, type, seq) \
    })

#define ROCKET_ENUM_DEFINE_VALUES__(type, name) \
    const auto name##Values__ = ::rocket::container::values<type, ::std::string_view>(name##Map__)

#define ROCKET_ENUM_DEFINE_OP_OUTPUT__(type, name) \
    ::std::ostream& \
    operator<<(::std::ostream& lhs, type rhs) { \
      return lhs << fmt::format("{}", rhs); \
    }

#define ROCKET_ENUM_DEFINE_LOCAL__(type, name, seq) \
    ROCKET_ENUM_DEFINE_MAP__(type, name, seq); \
    ROCKET_ENUM_DEFINE_VALUES__(type, name); \
    ROCKET_ENUM_DEFINE_OP_OUTPUT__(type, name)

// Global ...................................................................................................

#define ROCKET_ENUM_DEFINE_FMT_FORMATTER__(ns, type, name) \
    template<typename Char> \
    template<typename FormatContext> \
    auto \
    fmt::formatter<ns::type, Char>::format(ns::type v, FormatContext& ctx) const -> decltype(ctx.out()) { \
      if (auto it = ns::name##Map__.left.find(v); it != ns::name##Map__.left.end()) { \
        return underlying_.format(it->second, ctx); \
      } else { \
        return detail::write<char>(ctx.out(), "<invalid>"); \
      } \
    }

#define ROCKET_ENUM_DEFINE_ROCKET_ENUM__(ns, type, name) \
    ns::type \
    rocket::Enum<ns::type>::toType(::std::string_view s) { \
      auto it = ns::name##Map__.right.find(s); \
      if (it != ns::name##Map__.right.end()) { \
        return it->second; \
      } else { \
        throw ::rocket::InvalidState(::rocket::message::cannotParseAs(s, rocket::Type::of<ns::type>())); \
      } \
    }

#define ROCKET_ENUM_DEFINE_GLOBAL__(ns, type, name) \
    ROCKET_ENUM_DEFINE_FMT_FORMATTER__(ns, type, name); \
    ROCKET_ENUM_DEFINE_ROCKET_ENUM__(ns, type, name)

/// @endcond

/**
 * Provides definitions for the enum @p name needed for full Rocket interoperability.
 *
 * This macro must be called in the enum's local namespace.
 *
 * @param type the type of the enum, without namespace, e.g. `MyClass::MyEnum`
 * @param name the name to use for generated identifiers, e.g. `MyClass_MyEnum`
 * @param seq a sequence for the enum values, e.g. `(red)(green)(blue)`
 */
#define ROCKET_ENUM_DEFINE_LOCAL(type, name, seq) ROCKET_ENUM_DEFINE_LOCAL__(type, name, seq)

/**
 * Provides definitions for the enum @p name needed for full Rocket interoperability.
 *
 * This macro must be called in the global namespace.
 *
 * @param ns the namespace of the enum, e.g. `mynamespace`. May be left empty if the enum is in the global
 *     namespace
 * @param type the type of the enum, without namespace, e.g. `MyClass::MyEnum`
 * @param name the name to use for generated identifiers, e.g. `MyClass_MyEnum`
 */
#define ROCKET_ENUM_DEFINE_GLOBAL(ns, type, name) ROCKET_ENUM_DEFINE_GLOBAL__(ns, type, name)

// EOF
