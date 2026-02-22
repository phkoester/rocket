/**
 * @file enum.h
 *
 * Rich enums: automatic declarations and definitions.
 */

#pragma once

#include "rocket/assert.h"
#include "rocket/Bimap.h"
#include "rocket/str/message/message.h"
#include "rocket/unicode/ConvertTo.h"

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <fmt/format.h>

#include <scn/scan.h>

#include <ostream>

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Provides all the declarations for the enum @p type needed for full Rocket interoperability.
 *
 * In particular, this provides:
 *
 * - a #rocket::Enum specialization;
 * - an output operator for #std::ostream.
 *
 * @note This macro must be called in the global namespace.
 *
 * @param ns the namespace of the enum, e.g. `mynamespace`. May be left empty if the enum is in the global
 *   namespace
 * @param type the type of the enum, without namespace, e.g. `MyClass::MyEnum`
 * @param name the name to use for generated identifiers, e.g. `MyClass_MyEnum`
 */
#define ROCKET_ENUM_DECLARE(ns, type, name) ROCKET_ENUM_DECLARE__(ns, type, name)

/**
 * Provides all the definitions for the enum @p type needed for full Rocket interoperability.
 *
 * This macro must be called in the enum's local namespace.
 *
 * @param ns the namespace of the enum, e.g. `mynamespace`. May be left empty if the enum is in the global
 *   namespace
 * @param type the type of the enum, without namespace, e.g. `MyClass::MyEnum`
 * @param name the name to use for generated identifiers, e.g. `MyClass_MyEnum`
 * @param seq a sequence for the enum values, e.g. `(red)(green)(blue)`
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
    static ::std::pair<u64, ns::type> toType(::std::string_view str, bool strict); \
  }

#define ROCKET_ENUM_DECLARE_MAP__(type, name) \
    extern const ::rocket::Bimap<type, ::std::string_view> name##Map__; \
    const ::rocket::Bimap<type, ::std::string_view>& get##name##Map__();

#define ROCKET_ENUM_DECLARE_OP_OUTPUT__(type) \
  ::std::ostream& operator<<(::std::ostream& lhs, type rhs);

#define ROCKET_ENUM_DECLARE__(ns, type, name) \
  ROCKET_ENUM_DECLARE_ROCKET_ENUM__(ns, type); \
  ROCKET_NAMESPACE_BEGIN(ns); \
  ROCKET_ENUM_DECLARE_MAP__(type, name); \
  ROCKET_ENUM_DECLARE_OP_OUTPUT__(type); \
  ROCKET_NAMESPACE_END(ns)

// Definitions ..............................................................................................

#define ROCKET_ENUM_DEFINE_ROCKET_ENUM__(ns, type, name) \
  ::std::string_view \
  rocket::Enum<ns::type>::toString(ns::type val) { \
    auto it = ns::get##name##Map__().left.find(val); \
    if (it == ns::get##name##Map__().left.end()) { \
      ROCKET_FAIL("Invalid `{}` value: {}", typeid(ns::type), ::std::to_underlying(val)); \
    } \
    return it->second; \
  } \
  \
  ::std::pair<u64, ns::type> \
  rocket::Enum<ns::type>::toType(::std::string_view str, bool strict) { \
    if (strict) { \
      /* Strict */ \
      const auto it = ns::get##name##Map__().right.find(str); \
      if (it == ns::get##name##Map__().right.end()) { \
        throw ::rocket::InvalidState(::rocket::str::message::cannotScanAs(str, typeid(ns::type))); \
      } \
      return { it->first.size(), it->second }; \
    } \
    \
    /* Relaxed */ \
    u64 maxValueSize = 0; \
    ns::type maxKey; \
    for (const auto& [key, value] : ns::get##name##Map__().left) { \
      if (str.starts_with(value) && value.size() > maxValueSize) { \
        maxValueSize = value.size(); \
        maxKey = key; \
      } \
    } \
    if (maxValueSize > 0) { \
      return { maxValueSize, maxKey }; \
    } \
    throw ::rocket::InvalidState(::rocket::str::message::cannotScanAs(str, typeid(ns::type))); \
  }

#define ROCKET_ENUM_DEFINE_MAP_ELEM__(r, data, elem) { data::elem, BOOST_PP_STRINGIZE(elem) },

#define ROCKET_ENUM_DEFINE_MAP__(type, name, seq) \
  const ::rocket::Bimap<type, ::std::string_view> name##Map__ = \
    ::rocket::makeBimap<type, ::std::string_view>({ \
    BOOST_PP_SEQ_FOR_EACH(ROCKET_ENUM_DEFINE_MAP_ELEM__, type, seq) \
  }); \
  \
  const ::rocket::Bimap<type, ::std::string_view>& \
  get##name##Map__() { \
    return name##Map__; \
  }

#define ROCKET_ENUM_DEFINE_OP_OUTPUT__(type) \
  ::std::ostream& \
  operator<<(::std::ostream& lhs, type rhs) { \
    return lhs << fmt::format("{}", rhs); \
  }

#define ROCKET_ENUM_DEFINE__(ns, type, name, seq) \
  ROCKET_ENUM_DEFINE_ROCKET_ENUM__(ns, type, name); \
  ROCKET_NAMESPACE_BEGIN(ns); \
  ROCKET_ENUM_DEFINE_MAP__(type, name, seq); \
  ROCKET_ENUM_DEFINE_OP_OUTPUT__(type); \
  ROCKET_NAMESPACE_END(ns)

/// @endcond

namespace rocket {

// #Enum ----------------------------------------------------------------------------------------------------

/**
 * A class template for Rocket enums, providing some additional information about an enum.
 */
template<typename E> requires std::is_enum_v<E>
struct Enum : std::false_type {
  /**
   * Converts an enum value to a string.
   *
   * @param val the enum value
   * @return a string
   * @throw #std::exception if the operation fails
   */
  static std::string_view toString(E val);

  /**
   * Scans a string to an enum value.
   *
   * @param str the string to scan
   * @param strict whether the string must strictly match in its entirety. If this value is `true`, the
   *   string can be scanned in a more efficient way
   * @return a pair of the size of the scanned portion of the string and the enum value
   * @throw #std::exception if the operation fails
   */
  static std::pair<u64, E> toType(std::string_view str, bool strict);
};

} // namespace rocket

// #fmt::formatter<#rocket::Enum> ---------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::Enum}
 *
 * This formatter uses the same format specifiers as the underlying string formatter.
 */
template<typename E, typename C> requires rocket::Enum<E>::value && rocket::IsChar<C>
struct fmt::formatter<E, C> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(E val, FormatContext& ctx) const {
    try {
      auto str = rocket::Enum<E>::toString(val);
      return underlying_.format(rocket::unicode::ConvertTo<C>::apply(str), ctx);
    } catch (const std::exception&) {
      return underlying_.format(INVALID, ctx);
    }
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return underlying_.parse(ctx);
  }

  constexpr void
  set_debug_format(bool val = true) {
    underlying_.set_debug_format(val);
  }

  /// @endcond

private:

  static constexpr std::basic_string_view<C> INVALID =
    rocket::LiteralString<C, '<', 'i', 'n', 'v', 'a', 'l', 'i', 'd', '>'> {};

  fmt::formatter<basic_string_view<C>, C> underlying_;
};

// #scn::scanner<#rocket::Enum> -----------------------------------------------------------------------------

/**
 * @spec_scn_scanner{#rocket::Enum}
 *
 * This scanner uses the same format specifiers as the underlying string scanner.
 */
template<typename E> requires rocket::Enum<E>::value
struct scn::scanner<E, char> : scn::scanner<::std::string_view, char> {
  /// @cond undocumented

  using Base = scn::scanner<::std::string_view, char>;

  template<typename Context>
  scan_expected<typename Context::iterator>
  scan(E& val, Context& ctx) const {
    std::string_view str;
    auto result = Base::scan(str, ctx);
    if (result) {
      try {
        const auto [size, enumVal] = ::rocket::Enum<E>::toType(str, false);
        // If the consumed string is longer than the scanned portion, we need to correct the iterator
        const i64 correction = size - str.size(); // Zero or negative
        val = enumVal;
        auto it = result.value();
        std::advance(it, correction);
        return it; \
      } catch (const ::std::exception&) {
        return unexpected(scan_error(scan_error::invalid_scanned_value, "Invalid enum value"));
      }
    } else {
      return unexpected(result.error());
    }
  }

  /// @endcond
};

// EOF
