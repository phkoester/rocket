/**
 * @file enum.h
 *
 * Rich enums: automatic declarations and definitions.
 */

#pragma once

#include "rocket/Result.h"
#include "rocket/TypeId-format.h"
#include "rocket/assert.h"
#include "rocket/bimap.h"
#include "rocket/enum-fwd.h"
#include "rocket/str/message/message.h"

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Provides all the declarations for the enum @p type needed for full Rocket interoperability.
 *
 * In particular, this provides a #rocket::Enum specialization.
 *
 * @note This macro must be called in the global namespace.
 *
 * @param ns the namespace of the enum, e.g., `mynamespace`. May be left empty if the enum is in the global
 *   namespace
 * @param type the type of the enum, without namespace, e.g., `MyClass::MyEnum`
 * @param name the name to use for generated identifiers, e.g., `MyClass_MyEnum`
 */
#define ROCKET_ENUM_DECLARE(ns, type, name) ROCKET_ENUM_DECLARE__(ns, type, name)

/**
 * Provides all the definitions for the enum @p type needed for full Rocket interoperability.
 *
 * This macro must be called in the enum’s local namespace.
 *
 * @param ns the namespace of the enum, e.g., `mynamespace`. May be left empty if the enum is in the global
 *   namespace
 * @param type the type of the enum, without namespace, e.g., `MyClass::MyEnum`
 * @param name the name to use for generated identifiers, e.g., `MyClass_MyEnum`
 * @param seq a sequence for the enum values, e.g., `(red)(green)(blue)`
 */
#define ROCKET_ENUM_DEFINE(ns, type, name, seq) ROCKET_ENUM_DEFINE__(ns, type, name, seq)

// Internal macros ------------------------------------------------------------------------------------------

/// @cond undocumented

// Declarations .............................................................................................

#define ROCKET_ENUM_DECLARE_ROCKET_ENUM__(ns, type) \
  template<> \
  struct rocket::Enum<ns::type> : ::std::true_type { \
    static ::std::string_view toString(ns::type val); \
    \
    static ::rocket::Result<::std::pair<u64, ns::type>> toType(::std::string_view str, bool strict); \
  }

#define ROCKET_ENUM_DECLARE_MAP__(type, name) \
    extern const ::rocket::bimap<type, ::std::string_view> name##Map__; \
    const ::rocket::bimap<type, ::std::string_view>& get##name##Map__();

#define ROCKET_ENUM_DECLARE__(ns, type, name) \
  ROCKET_ENUM_DECLARE_ROCKET_ENUM__(ns, type); \
  ROCKET_NAMESPACE_BEGIN(ns); \
  ROCKET_ENUM_DECLARE_MAP__(type, name); \
  ROCKET_NAMESPACE_END(ns)

// Definitions ..............................................................................................

#define ROCKET_ENUM_DEFINE_ROCKET_ENUM__(ns, type, name) \
  ::std::string_view \
  rocket::Enum<ns::type>::toString(ns::type val) { \
    auto it = ns::get##name##Map__().left.find(val); \
    if (it == ns::get##name##Map__().left.end()) { \
      ROCKET_FAIL("invalid {} value {}", ::rocket::TypeId::of<ns::type>(), ::std::to_underlying(val)); \
    } \
    return it->second; \
  } \
  \
  ::rocket::Result<::std::pair<u64, ns::type>> \
  rocket::Enum<ns::type>::toType(::std::string_view str, bool strict) { \
    if (strict) { \
      /* Strict */ \
      const auto it = ns::get##name##Map__().right.find(str); \
      if (it == ns::get##name##Map__().right.end()) { \
        return ::std::unexpected(::rocket::Error { \
          ::rocket::str::message::cannotScanAs(str, typeid(ns::type)) \
        }); \
      } \
      return ::std::pair { it->first.size(), it->second }; \
    } \
    \
    /* Nonstrict */ \
    u64 maxValueSize = 0; \
    ns::type maxKey; \
    for (const auto& [key, val] : ns::get##name##Map__().left) { \
      if (str.starts_with(val) && val.size() > maxValueSize) { \
        maxValueSize = val.size(); \
        maxKey = key; \
      } \
    } \
    if (maxValueSize > 0) { \
      return ::std::pair { maxValueSize, maxKey }; \
    } \
    return ::std::unexpected(::rocket::Error { \
      ::rocket::str::message::cannotScanAs(str, typeid(ns::type)) \
    }); \
  }

#define ROCKET_ENUM_DEFINE_MAP_ELEM__(r, data, elem) { data::elem, BOOST_PP_STRINGIZE(elem) },

#define ROCKET_ENUM_DEFINE_MAP__(type, name, seq) \
  const ::rocket::bimap<type, ::std::string_view> name##Map__ = \
    ::rocket::make_bimap<type, ::std::string_view>({ \
    BOOST_PP_SEQ_FOR_EACH(ROCKET_ENUM_DEFINE_MAP_ELEM__, type, seq) \
  }); \
  \
  const ::rocket::bimap<type, ::std::string_view>& \
  get##name##Map__() { \
    return name##Map__; \
  }

#define ROCKET_ENUM_DEFINE__(ns, type, name, seq) \
  ROCKET_ENUM_DEFINE_ROCKET_ENUM__(ns, type, name); \
  ROCKET_NAMESPACE_BEGIN(ns); \
  ROCKET_ENUM_DEFINE_MAP__(type, name, seq); \
  ROCKET_NAMESPACE_END(ns)

/// @endcond

// EOF
