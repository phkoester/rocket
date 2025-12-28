/**
 * @file enum.h
 *
 * Enum utilities.
 */

#pragma once

#include "enum-decl.h"

#include "Type.h"
#include "codec.h"
#include "container.h"
#include "io-decl.h"

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

// Macros ---------------------------------------------------------------------------------------------------

/// @cond undocumented

#define ROCKET_ENUM_DEFINE_MAP_ELEM__(r, data, elem) { data::elem, BOOST_PP_STRINGIZE(elem) },

#define ROCKET_ENUM_DEFINE_MAP__(type, name, seq) \
    const auto name##Map__ = ::rocket::container::makeUnorderedBimap<type, ::std::string_view>({ \
      BOOST_PP_SEQ_FOR_EACH(ROCKET_ENUM_DEFINE_MAP_ELEM__, type, seq) \
    })

#define ROCKET_ENUM_DEFINE_VALUES__(type, name) \
    const auto name##Values__ = ::rocket::container::values<type, ::std::string_view>(name##Map__)

#define ROCKET_ENUM_DEFINE_OP_INPUT__(type, name) \
    ::std::istream& \
    operator>>(::std::istream& lhs, type& rhs) { \
      try { \
        auto value = ::rocket::io::getString(lhs, name##Values__); \
        auto it = name##Map__.right.find(value); \
        ROCKET_EXPECT(it != name##Map__.right.end()); \
        rhs = it->second; \
        return lhs; \
      } catch (const ::std::exception&) { \
        lhs.setstate(std::ios::failbit); \
        return lhs; \
      } \
    }

#define ROCKET_ENUM_DEFINE_OP_OUTPUT__(type, name) \
    ::std::ostream& \
    operator<<(::std::ostream& lhs, type rhs) { \
      return lhs << fmt::format("{}", rhs); \
    }

#define ROCKET_ENUM_DEFINE_PARSE_RON__(type, _name) \
    ::std::istream& \
    parseRon(::std::istream& is, type& v) { \
      auto enumResult = ::rocket::codec::ron::parsing::parseEnum(is); \
      if (auto it = _name##Map__.right.find(enumResult.input); it != _name##Map__.right.end()) { \
        v = it->second; \
        return is; \
      } else { \
        throw ::rocket::io::ParseFailure( \
            is, \
            enumResult.actualInputPos, \
            { enumResult.actualInputPos, enumResult.actualInputPos + enumResult.actualInput.size() }, \
            message::cannotParseAs(enumResult.actualInput, Type::of<type>())); \
      } \
    }

#define ROCKET_ENUM_DEFINE__(type, name, seq) \
    ROCKET_ENUM_DEFINE_MAP__(type, name, seq); \
    ROCKET_ENUM_DEFINE_VALUES__(type, name); \
    ROCKET_ENUM_DEFINE_OP_INPUT__(type, name) \
    ROCKET_ENUM_DEFINE_OP_OUTPUT__(type, name)

#define ROCKET_ENUM_DEFINE_FMT_FORMATTER__(ns, type, _name) \
    template<typename Char> \
    template<typename FormatContext> \
    auto \
    fmt::formatter<ns::type, Char>::format(ns::type v, FormatContext& ctx) const -> decltype(ctx.out()) { \
      if (auto it = ns::_name##Map__.left.find(v); it != ns::_name##Map__.left.end()) { \
        return underlying_.format(it->second, ctx); \
      } else { \
        return detail::write<char>(ctx.out(), "<invalid>"); \
      } \
    }

/// @endcond

/**
 * Provides definitions for the enum @p name needed for full Rocket interoperability.
 *
 * This macro must be called in the enum's namespace.
 *
 * @param type the type of the enum, without namespace, e.g. `MyClass::MyEnum`
 * @param name the name to use for generated identifiers, e.g. `MyClass_MyEnum`
 * @param seq a sequence for the enum values
 */
#define ROCKET_ENUM_DEFINE(type, name, seq) ROCKET_ENUM_DEFINE__(type, name, seq)

/**
 * Defines a `fmt::formatter` specialization for the enum @p type.
 *
 * This macro must be called in the global namespace.
 *
 * @param ns the namespace of the enum, e.g. `mynamespace`. May be left empty if the enum is in the global
 *     namespace
 * @param type the type of the enum, without namespace, e.g. `MyClass::MyEnum`
 * @param name the name to use for generated identifiers, e.g. `MyClass_MyEnum`
 */
#define ROCKET_ENUM_DEFINE_FMT_FORMATTER(ns, type, name) ROCKET_ENUM_DEFINE_FMT_FORMATTER__(ns, type, name)

// EOF
