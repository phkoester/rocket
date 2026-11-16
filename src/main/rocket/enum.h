/**
 * @file enum.h
 *
 * Rich enums: automatic declarations and definitions.
 */

#pragma once

#include "rocket/bimap.h"
#include "rocket/Result.h"
#include "rocket/TypeId.h"
#include "rocket/assert.h"
#include "rocket/format.h"
#include "rocket/format/StringFormatter.h"
#include "rocket/scan.h"
#include "rocket/str/message/message.h"
#include "rocket/unicode/ConvertTo.h"

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

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

// `Enum` ---------------------------------------------------------------------------------------------------

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
   * @throw #rocket::InvalidState if the operation fails
   */
  static std::string_view toString(E val);

  /**
   * Scans a string to an enum value.
   *
   * @param str the string to scan
   * @param strict whether the string must strictly match in its entirety. If this value is `true`, the
   *   string can be scanned in a more efficient way
   * @return a pair of the size of the scanned portion of the string and the enum value, or an error if the
   *   operation fails
   */
  static Result<std::pair<u64, E>> toType(std::string_view str, bool strict);
};

} // namespace rocket

// `fmt::formatter<rocket::Enum>` ---------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::Enum}
 *
 * This formatter uses the same format specifiers as the underlying string formatter.
 */
template<typename E, typename C> requires rocket::Enum<E>::value && rocket::IsChar<C>
struct fmt::formatter<E, C> : rocket::internal::format::StringFormatter<C> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(E val, FormatContext& ctx) const {
    return this->formatUtf8(rocket::Enum<E>::toString(val), ctx); // This may throw, which is okay
  }

  /// @endcond
};

// `scn::scanner<rocket::Enum>` -----------------------------------------------------------------------------

/**
 * @spec_scn_scanner{#rocket::Enum}
 *
 * This scanner uses the same format specifiers as the underlying string scanner. It scans a word and then
 * gives back the characters that follow the longest enum name the word starts with, so that, for example,
 * `Red,` scans as `Red` and leaves the comma in the input.
 */
template<typename E> requires rocket::Enum<E>::value
struct scn::scanner<E, char> : scn::scanner<std::string, char> {
  /// @cond undocumented

  template<typename Context>
  scn::scan_expected<typename Context::iterator>
  scan(E& val, Context& ctx) const {
    // This must scan a #std::string, not a #std::string_view, because the latter does not work when scanning
    // from a #std::istream
    std::string str;
    auto scanResult = scn::scanner<std::string, char>::scan(str, ctx);
    if (not scanResult) {
      return scn::unexpected(scanResult.error());
    }
    const auto enumResult = rocket::Enum<E>::toType(str, false);
    if (not enumResult) {
      return scn::unexpected(scn::scan_error(scn::scan_error::invalid_scanned_value, "invalid enum value"));
    }
    const auto [size, enumVal] = *enumResult;
    val = enumVal;

    // Give back what follows the scanned portion of the word. The iterators of scnlib are bidirectional
    auto it = *scanResult;
    std::advance(it, static_cast<std::ptrdiff_t>(size) - static_cast<std::ptrdiff_t>(str.size()));
    return it;
  }

  /// @endcond
};

// EOF
