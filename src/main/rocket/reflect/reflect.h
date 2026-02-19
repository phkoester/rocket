/**
 * @file reflect.h
 *
 * C++ reflection support: members and variables.
 */

#pragma once

#include "rocket/codec/CompareEncoder.h"
#include "rocket/codec/EqualToEncoder.h"
#include "rocket/codec/HashEncoder.h"
#include "rocket/codec/FormattedCodec.h"
#include "rocket/nio/nio.h"
#include "rocket/unicode/ConvertTo.h"

#include <fmt/std.h> // #fmt::formatter<#std::type_info>

// Macros ---------------------------------------------------------------------------------------------------

// Members ..................................................................................................

/**
 * Provides access to a named member-reference container.
 *
 * @note This macro must be called inside the class declaration, in a public section.
 *
 * @param cls the name of the class that holds the members (without namespace)
 * @param name the name of the member-reference container, e.g. `Index`
 * @param seq a sequence of member names
 */
#define ROCKET_REFLECT_MEMBERS(cls, name, seq) ROCKET_REFLECT_MEMBERS__(cls, name, seq)

/**
 * Provides all the declarations for the class @p cls needed for full Rocket interoperability.
 *
 * In particular, this provides:
 *
 * - a #rocket::reflect::Declared specialization;
 * - comparison operators `==`, `<=>`;
 * - an output operator for #std::ostream.
 *
 * @note This macro must be called in the global namespace.
 *
 * @param ns the namespace of the class, e.g. `mynamespace`. May be left empty if the class is in the global
 *   namespace
 * @param cls the type of the class without namespace, e.g. `MyClass`
 * @param name the name of the member-reference container to use, e.g. `Index`
*/
#define ROCKET_REFLECT_MEMBERS_DECLARE(ns, cls, name) ROCKET_REFLECT_MEMBERS_DECLARE__(ns, cls, name)

/**
 * Provides all the definitions for the class @p cls needed for full Rocket interoperability.
 *
 * @note This macro must be called in the global namespace.
 *
 * @param ns the namespace of the class, e.g. `mynamespace`. May be left empty if the class is in the global
 *   namespace
 * @param cls the type of the class without namespace, e.g. `MyClass`
 * @param name the name of the member-reference container to use, e.g. `Index`
*/
#define ROCKET_REFLECT_MEMBERS_DEFINE(ns, cls, name) ROCKET_REFLECT_MEMBERS_DEFINE__(ns, cls, name)

/**
 * Provides access to a named member-reference container for a derived class.
 *
 * @note This macro must be called inside the class declaration, in a public section.
 *
 * @param baseCls the name of the base class
 * @param baseName the name of the member-reference container of the base class
 * @param cls the name of the derived class that holds the members (without namespace)
 * @param name the name for this member-reference container. e.g. `Index`
 * @param seq a sequence of member names
 */
#define ROCKET_REFLECT_MEMBERS_DERIVED(baseCls, baseName, cls, name, seq) \
  ROCKET_REFLECT_MEMBERS_DERIVED__(baseCls, baseName, cls, name, seq)

// Variables ................................................................................................

/**
 * Makes a variable-reference container, which is in fact a #std::tuple of #rocket::reflect::VarRef
 * instances.
 *
 * @param seq a sequence of variable names
 */
#define ROCKET_REFLECT_VARS(seq) ROCKET_REFLECT_VARS__(seq)

// Internal macros ------------------------------------------------------------------------------------------

/// @cond undocumented

// Members ..................................................................................................

#define ROCKET_REFLECT_MEMBERS_REFS_ELEM__(r, data, elem) \
  (::rocket::reflect::MemberRef(BOOST_PP_STRINGIZE(elem), &data::elem))

#define ROCKET_REFLECT_MEMBERS_REFS__(cls, seq) \
  ::std::make_tuple( \
    BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(ROCKET_REFLECT_MEMBERS_REFS_ELEM__, cls, seq)))

#define ROCKET_REFLECT_MEMBERS__(cls, name, seq) \
  struct name { \
    static constexpr auto refs = ROCKET_REFLECT_MEMBERS_REFS__(cls, seq); \
    \
    using Cmp = ::rocket::StdCompare; \
    using Ordering = ::rocket::codec::internal::CmpCommonOrdering<Cmp, Purge<decltype(refs)>>; \
  }

#define ROCKET_REFLECT_MEMBERS_DERIVED__(baseCls, baseName, cls, name, seq) \
  struct name { \
    static constexpr auto refs = ::std::tuple_cat( \
      baseCls::baseName::refs, \
      ROCKET_REFLECT_MEMBERS_REFS__(cls, seq)); \
    \
    using Cmp = ::rocket::StdCompare; \
    using Ordering = ::rocket::codec::internal::CmpCommonOrdering<Cmp, Purge<decltype(refs)>>; \
  }

#define ROCKET_REFLECT_MEMBERS_DECLARE_DECLARED__(ns, cls, name) \
  template<> \
  struct rocket::reflect::Declared<ns::cls> : ::std::true_type{ \
    static constexpr auto& refs = ns::cls::name::refs; \
  }

#define ROCKET_REFLECT_MEMBERS_DECLARE_OP_EQ__(cls) \
  bool operator==(const cls& lhs, const cls& rhs)

#define ROCKET_REFLECT_MEMBERS_DECLARE_OP_CMP__(cls, name) \
  cls::name::Ordering operator<=>(const cls& lhs, const cls& rhs)

#define ROCKET_REFLECT_MEMBERS_DECLARE_OP_OUTPUT__(cls) \
  ::std::ostream& operator<<(::std::ostream&, const cls& rhs)

#define ROCKET_REFLECT_MEMBERS_DECLARE__(ns, cls, name) \
  ROCKET_REFLECT_MEMBERS_DECLARE_DECLARED__(ns, cls, name); \
  ROCKET_NAMESPACE_BEGIN(ns); \
  ROCKET_REFLECT_MEMBERS_DECLARE_OP_EQ__(cls); \
  ROCKET_REFLECT_MEMBERS_DECLARE_OP_CMP__(cls, name); \
  ROCKET_REFLECT_MEMBERS_DECLARE_OP_OUTPUT__(cls); \
  ROCKET_NAMESPACE_END(ns)

#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_EQ__(cls) \
  bool \
  operator==(const cls& lhs, const cls& rhs) { \
    return ::rocket::codec::EqualToEncoder<>().encode(lhs, rhs); \
  }

#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_CMP__(cls, name) \
  cls::name::Ordering \
  operator<=>(const cls& lhs, const cls& rhs) { \
    return ::rocket::codec::CompareEncoder<>().encode(lhs, rhs); \
  }

#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_OUTPUT__(cls) \
  ::std::ostream& \
  operator<<(::std::ostream& lhs, const cls& rhs) { \
    return lhs << fmt::format("{}", rhs); \
  }

#define ROCKET_REFLECT_MEMBERS_DEFINE__(ns, cls, name) \
  ROCKET_NAMESPACE_BEGIN(ns); \
  ROCKET_REFLECT_MEMBERS_DEFINE_OP_EQ__(cls); \
  ROCKET_REFLECT_MEMBERS_DEFINE_OP_CMP__(cls, name); \
  ROCKET_REFLECT_MEMBERS_DEFINE_OP_OUTPUT__(cls); \
  ROCKET_NAMESPACE_END(ns)

// Variables ................................................................................................

#define ROCKET_REFLECT_VARS_ELEM__(r, data, elem) \
  (::rocket::reflect::VarRef(BOOST_PP_STRINGIZE(elem), elem))

#define ROCKET_REFLECT_VARS__(seq) \
  ::std::make_tuple(BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(ROCKET_REFLECT_VARS_ELEM__, ~, seq)))

/// @endcond

namespace rocket::reflect {

// #Instance ------------------------------------------------------------------------------------------------

template<typename T, typename Inner>
inline bool
operator==(const Instance<T, Inner>& lhs, const Instance<T, Inner>& rhs) {
  return codec::EqualToEncoder<>().encode(lhs, rhs);
}

template<typename T, typename Inner>
inline auto
operator<=>(const Instance<T, Inner>& lhs, const Instance<T, Inner>& rhs) {
  return codec::CompareEncoder<>().encode(lhs, rhs);
}

// @op_output{#rocket::reflect::Instance}
template<typename T, typename Inner>
inline std::ostream&
operator<<(std::ostream& lhs, const Instance<T, Inner>& rhs) {
  return lhs << fmt::format("{}", rhs);
}

// #VarRef --------------------------------------------------------------------------------------------------

// @op_eq{#rocket::reflect::VarRef}
template<typename T>
inline bool
operator==(const VarRef<T>& lhs, const VarRef<T>& rhs) {
  return codec::EqualToEncoder<>().encode(lhs.get(), rhs.get());
}

// @op_cmp{#rocket::reflect::VarRef}
template<typename T>
inline auto
operator<=>(const VarRef<T>& lhs, const VarRef<T>& rhs) {
  return codec::CompareEncoder<>().encode(lhs, rhs);
}

// @op_output{#rocket::reflect::VarRef}
template<typename T>
inline std::ostream&
operator<<(std::ostream& lhs, const VarRef<T>& rhs) {
  return lhs << fmt::format("{}", rhs);
}

} // namespace rocket::reflect

// #fmt::formatter<#rocket::reflect::Declared> --------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::reflect::Declared}
 *
 * - If the `i` format specifier is used, then the output is indented.
 * - If the `t` format specifier is used, then the type name is included.
 */
template<typename T, typename C> requires rocket::reflect::Declared<T>::value && rocket::IsChar<C>
struct fmt::formatter<T, C> {
  /// @cond undocumented
  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const T& val, FormatContext& ctx) const {
    auto out = ctx.out();
    if (withType_) {
      const std::string typeName = fmt::format("{}", typeid(val));
      // GCC 13.3 needs `fmt::detail` here
      out = fmt::detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(typeName));
    }
    rocket::codec::FormattedCodec codec;
    rocket::nio::StringSink sink;
    codec.encode(val, sink, { .indent=indent_ });
    // GCC 13.3 needs `fmt::detail` here
    out = fmt::detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(sink.str()));
    return out;
  }

  constexpr const C*
  parse(fmt::parse_context<C>& ctx) { // GCC 13.3 needs `fmt::parse_context` here
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it == 'i') {
      indent_ = true;
      ++it;
    }
    if (it != end && *it == 't') {
      withType_ = true;
      ++it;
    }
    return it;
  }
  /// @endcond

private:

  bool indent_ = false;
  bool withType_ = false;
};

// #fmt::formatter<#rocket::reflect::Instance> --------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::reflect::Instance}
 *
 * - If the `i` format specifier is used, then the output is indented.
 * - If the `t` format specifier is used, then the type name is included.
 */
template<typename T, typename Inner, typename C> requires rocket::IsChar<C>
struct fmt::formatter<rocket::reflect::Instance<T, Inner>, C> {
  /// @cond undocumented
  using Type = rocket::reflect::Instance<T, Inner>;

  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const Type& val, FormatContext& ctx) const {
    auto out = ctx.out();
    if (withType_) {
      const std::string typeName = fmt::format("{}", typeid(typename Type::Type));
      out = detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(typeName));
    }
    rocket::codec::FormattedCodec codec;
    rocket::nio::StringSink sink;
    codec.encode(val, sink, { .indent=indent_ });
    out = detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(sink.str()));
    return out;
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it == 'i') {
      indent_ = true;
      ++it;
    }
    if (it != end && *it == 't') {
      withType_ = true;
      ++it;
    }
    return it;
  }
  /// @endcond

private:

  bool indent_ = false;
  bool withType_ = false;
};

// #fmt::formatter<#rocket::reflect::VarRef> ----------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::reflect::VarRef}
 *
 * - If the `i` format specifier is used, then the output is indented.
 */
template<typename T, typename C> requires fmt::is_formattable<T, C>::value && rocket::IsChar<C>
struct fmt::formatter<rocket::reflect::VarRef<T>, C> {
  /// @cond undocumented
  using Type = rocket::reflect::VarRef<T>;

  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const Type& val, FormatContext& ctx) const {
    auto out = ctx.out();
    rocket::codec::FormattedCodec codec;
    rocket::nio::StringSink sink;
    codec.encode(val, sink, { .indent=indent_ });
    out = detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(sink.str()));
    return out;
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it == 'i') {
      indent_ = true;
      ++it;
    }
    return it;
  }
  /// @endcond

private:

  bool indent_ = false;
};

// #std::hash<#rocket::reflect::Declared> -------------------------------------------------------------------

/// @spec_std_hash{#rocket::reflect::Declared}
template<typename T> requires rocket::reflect::Declared<T>::value
struct std::hash<T> {
  /// @cond undocumented
  u64
  operator()(const T& val) const {
    return rocket::codec::HashEncoder<>().encode(val);
  }
  /// @endcond
};

// #std::hash<#rocket::reflect::Instance> -------------------------------------------------------------------

/// @spec_std_hash{#rocket::reflect::Instance}
template<typename T, typename Inner>
struct std::hash<rocket::reflect::Instance<T, Inner>> {
  /// @cond undocumented
  using Type = rocket::reflect::Instance<T, Inner>;

  u64
  operator()(const Type& val) const {
    return rocket::codec::HashEncoder<>().encode(val);
  }
  /// @endcond
};

// #std::hash<#rocket::reflect::VarRef> ---------------------------------------------------------------------

/// @spec_std_hash{#rocket::reflect::VarRef}
template<typename T>
struct std::hash<rocket::reflect::VarRef<T>> {
  /// @cond undocumented
  using Type = rocket::reflect::VarRef<T>;

  u64
  operator()(const Type& val) const {
    return rocket::codec::HashEncoder<>().encode(val);
  }
  /// @endcond
};

// EOF
