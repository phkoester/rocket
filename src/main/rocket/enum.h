/**
 * @file enum.h
 *
 * Rich enums: automatic declarations and definitions.
 */

#pragma once

#include "rocket/Exception.h"
#include "rocket/Type.h"
#include "rocket/UnorderedBimap.h"
#include "rocket/format/format.h"
#include "rocket/str/message/message.h"
#include "rocket/unicode/ConvertTo.h"

#include <ostream>

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Provides all the declarations for the enum @p type needed for full Rocket interoperability.
 *
 * In particular, it provides
 *
 * - an `operator<<` for `std::ostream`;
 * - a `fmt::formatter` specialization so the enum can be formatted using `fmt::format()`;
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

/**
 * Provides all the definitions for the enum @p name needed for full Rocket interoperability.
 *
 * This macro must be called in the enum's local namespace.
 *
 * @param ns the namespace of the enum, e.g. `mynamespace`. May be left empty if the enum is in the global
 *     namespace
 * @param type the type of the enum, without namespace, e.g. `MyClass::MyEnum`
 * @param name the name to use for generated identifiers, e.g. `MyClass_MyEnum`
 * @param seq a sequence for the enum values, e.g. `(red)(green)(blue)`
 */
#define ROCKET_ENUM_DEFINE(ns, type, name, seq) ROCKET_ENUM_DEFINE__(ns, type, name, seq)

// Internal macros ------------------------------------------------------------------------------------------

/// @cond undocumented

// Declarations .............................................................................................

#define ROCKET_ENUM_DECLARE_MAP__(type, name) \
    extern const ::rocket::UnorderedBimap<type, ::std::string_view> name##Map__;

#define ROCKET_ENUM_DECLARE_OP_OUTPUT__(type) \
    ::std::ostream& operator<<(::std::ostream&, type);

#define ROCKET_ENUM_DECLARE_FMT_FORMATTER__(ns, type, name) \
    template<typename C> \
    struct fmt::formatter<ns::type, C> { \
      template<typename FormatContext> \
      constexpr FormatContext::iterator \
      format(ns::type val, FormatContext& ctx) const { \
        if (auto it = ns::name##Map__.left.find(val); it != ns::name##Map__.left.end()) { \
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
      set_debug_format(bool val = true) { \
        underlying_.set_debug_format(val); \
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
    struct rocket::Enum<type> : ::std::true_type { \
      static type toType(::std::string_view str); \
    }

#define ROCKET_ENUM_DECLARE__(ns, type, name) \
    ROCKET_NS_BEGIN(ns); \
    ROCKET_ENUM_DECLARE_MAP__(type, name); \
    ROCKET_ENUM_DECLARE_OP_OUTPUT__(type); \
    ROCKET_NS_END(ns); \
    ROCKET_ENUM_DECLARE_FMT_FORMATTER__(ns, type, name); \
    ROCKET_ENUM_DECLARE_ROCKET_ENUM__(ns::type)

// Definitions ..............................................................................................

#define ROCKET_ENUM_DEFINE_MAP_ELEM__(r, data, elem) { data::elem, BOOST_PP_STRINGIZE(elem) },

#define ROCKET_ENUM_DEFINE_MAP__(type, name, seq) \
    /* gcc accepts no `auto` here*/ \
    const ::rocket::UnorderedBimap<type, ::std::string_view> name##Map__ = \
        ::rocket::makeUnorderedBimap<type, ::std::string_view>({ \
      BOOST_PP_SEQ_FOR_EACH(ROCKET_ENUM_DEFINE_MAP_ELEM__, type, seq) \
    })

#define ROCKET_ENUM_DEFINE_OP_OUTPUT__(type, name) \
    ::std::ostream& \
    operator<<(::std::ostream& lhs, type rhs) { \
      return lhs << ::fmt::format("{}", rhs); \
    }

#define ROCKET_ENUM_DEFINE_ROCKET_ENUM__(ns, type, name) \
    ns::type \
    rocket::Enum<ns::type>::toType(::std::string_view str) { \
      auto it = ns::name##Map__.right.find(str); \
      if (it != ns::name##Map__.right.end()) { \
        return it->second; \
      } else { \
        throw ::rocket::InvalidState(::rocket::str::message::cannotScanAs(str, ::rocket::Type::of<ns::type>())); \
      } \
    }

#define ROCKET_ENUM_DEFINE__(ns, type, name, seq) \
    ROCKET_NS_BEGIN(ns); \
    ROCKET_ENUM_DEFINE_MAP__(type, name, seq); \
    ROCKET_ENUM_DEFINE_OP_OUTPUT__(type, name); \
    ROCKET_NS_END(ns); \
    ROCKET_ENUM_DEFINE_ROCKET_ENUM__(ns, type, name)

/// @endcond

namespace rocket {

// `Enum` ---------------------------------------------------------------------------------------------------

/**
 * A class template for Rocket enums, providing some additional information about an enum.
 */
template<typename E> requires std::is_enum_v<E>
struct Enum : std::false_type {};

} // namespace rocket

// EOF
