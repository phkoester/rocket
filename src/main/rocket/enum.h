/**
 * @file enum.h
 *
 * Enum utilities.
 */

#pragma once

#include "enum-decl.h"

#include "rocket/S.h"
#include "rocket/Type.h"
#include "rocket/assert.h"
#include "rocket/boost.h"
#include "rocket/codec.h"
#include "rocket/except.h"
#include "rocket/io-decl.h"

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

// Macros ---------------------------------------------------------------------------------------------------

/// @cond undocumented

#define ROCKET_ENUM_DEFINE_MAP_ELEM__(r, data, elem) { data::elem, BOOST_PP_STRINGIZE(elem) },

#define ROCKET_ENUM_DEFINE_MAP__(type, name, seq) \
    const auto name##Map__ = ::rocket::boost::bimap::UnorderedBimap<type, ::std::string_view>::of({ \
      BOOST_PP_SEQ_FOR_EACH(ROCKET_ENUM_DEFINE_MAP_ELEM__, type, seq) \
    })

#define ROCKET_ENUM_DEFINE_VALUES__(type, name) \
    const auto name##Values__ = ::rocket::boost::bimap::values<type, ::std::string_view>(name##Map__)

#define ROCKET_ENUM_DEFINE_INPUT_OP__(type, name) \
    ::std::istream& \
    operator>>(::std::istream& lhs, type& rhs) { \
      try { \
        auto value = ::rocket::io::getString(lhs, name##Values__); \
        rhs = name##Map__.right.find(value)->second; \
        return lhs; \
      } catch (const ::std::exception& ex) { \
        lhs.setstate(std::ios::failbit); \
        return lhs; \
      } \
    }

#define ROCKET_ENUM_DEFINE_OUTPUT_OP__(type, name) \
    ::std::ostream& \
    operator<<(::std::ostream& lhs, type rhs) { \
      if (auto it = name##Map__.left.find(rhs); it != name##Map__.left.end()) \
        return lhs << it->second; \
      else \
        ROCKET_CHECK(rhs, false, ::rocket::S << "Invalid " << ::rocket::Type::of<type>() << ": " << static_cast<int>(rhs)); \
    }

#define ROCKET_ENUM_DEFINE_PARSE_RON__(type, name) \
    ::std::istream& \
    parseRon(::std::istream& is, type& v) { \
      auto enumResult = ::rocket::codec::ron::parsing::parseEnum(is); \
      if (auto it = name##Map__.right.find(enumResult.input); it != name##Map__.right.end()) { \
        v = it->second; \
        return is; \
      } else { \
        throw ::rocket::except::ParseFailure<char>( \
            is, \
            enumResult.actualInputPos, \
            { enumResult.actualInputPos, enumResult.actualInputPos + enumResult.actualInput.size() }, \
            ::rocket::except::message::cannotParseAs(enumResult.actualInput, Type::of<type>())); \
      } \
    }

#define ROCKET_ENUM_DEFINE_PRINT_RON__(type, name) \
    ::std::ostream& \
    printRon(::std::ostream& os, type v) { \
      if (auto it = name##Map__.left.find(v); it != name##Map__.left.end()) \
        return printRon(os, it->second); \
      else \
        ROCKET_CHECK(v, false, ::rocket::S << "Invalid " << ::rocket::Type::of<type>() << ": " << static_cast<int>(v)); \
    }

#define ROCKET_ENUM_DEFINE__(type, name, seq) \
    ROCKET_ENUM_DEFINE_MAP__(type, name, seq); \
    ROCKET_ENUM_DEFINE_VALUES__(type, name); \
    ROCKET_ENUM_DEFINE_INPUT_OP__(type, name) \
    ROCKET_ENUM_DEFINE_OUTPUT_OP__(type, name) \
    ROCKET_ENUM_DEFINE_PARSE_RON__(type, name) \
    ROCKET_ENUM_DEFINE_PRINT_RON__(type, name) \

#define ROCKET_ENUM_DEFINE_FMT_FORMATTER__(ns, type, name) \
    auto \
    ::fmt::formatter<ns::type>::format(ns::type v, format_context& ctx) const -> \
        format_context::iterator { \
      if (auto it = ns::name##Map__.left.find(v); it != ns::name##Map__.left.end()) \
        return formatter<string_view>::format(it->second, ctx); \
      else \
        ROCKET_CHECK(v, false, ::rocket::S << "Invalid " << ::rocket::Type::of<ns::type>() << ": " << static_cast<int>(v)); \
    } \

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
