/**
 * @file reflect.h
 *
 * C++ reflection support.
 */

#pragma once

#include "rocket/macro.h"
#include "rocket/codec/HashEncoder.h"
#include "rocket/nio/nio.h"
#include "rocket/unicode/ConvertTo.h"

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/cat.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <fmt/std.h>

#include <tuple>
#include <utility>

// Macros ---------------------------------------------------------------------------------------------------

// Members ..................................................................................................

/**
 * Provides access to a named member-reference container.
 *
 * @note This macro must be called inside the class declaration, in a public section.
 *
 * @param cls the name of the class that holds the members (without namespace)
 * @param name the name for this member-reference container. e.g. `index`
 * @param seq a sequence of member names
 */
#define ROCKET_REFLECT_MEMBERS(cls, name, seq) ROCKET_REFLECT_MEMBERS__(cls, name, seq)

/**
 * Provides all the declarations for the class @p cls needed for full Rocket interoperability.
 *
 * In particular, it provides
 *
 * - a `fmt::formatter` specialization so the class can be formatted using `fmt::format()`;
 * - a #std::hash specialization for the class;
 * - `operator==`, `operator!=`, `operator<`, `operator>`;
 * - an `operator<<` for #std::ostream;
 *
 * @note This macro must be called in the global namespace.
 *
 * @param ns the namespace of the class, e.g. `mynamespace`. May be left empty if the class is in the global
 *   namespace
 * @param cls the type of the class without namespace, e.g. `MyClass`
 * @param name the name of the member-reference container to use
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
 * @param name the name of the member-reference container to use
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
 * @param name the name for this member-reference container. e.g. `index`
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

#define ROCKET_REFLECT_MEMBERS_STRUCT__(name) BOOST_PP_SEQ_CAT((RocketReflect)(name)(__))

#define ROCKET_REFLECT_MEMBERS_REFS_ELEM__(r, data, elem) \
  (::rocket::reflect::MemberRef(BOOST_PP_STRINGIZE(elem), &data::elem))

#define ROCKET_REFLECT_MEMBERS_REFS__(cls, seq) \
  ::std::make_tuple( \
    BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(ROCKET_REFLECT_MEMBERS_REFS_ELEM__, cls, seq)))

#define ROCKET_REFLECT_MEMBERS__(cls, name, seq) \
  struct ROCKET_REFLECT_MEMBERS_STRUCT__(name) { \
    static constexpr auto refs = ROCKET_REFLECT_MEMBERS_REFS__(cls, seq); \
  }; \
  \
  static consteval auto& name() { return ROCKET_REFLECT_MEMBERS_STRUCT__(name)::refs; }

#define ROCKET_REFLECT_MEMBERS_DERIVED__(baseCls, baseName, cls, name, seq) \
  struct ROCKET_REFLECT_MEMBERS_STRUCT__(name) { \
    static constexpr auto refs = ::std::tuple_cat( \
      baseCls::ROCKET_REFLECT_MEMBERS_STRUCT__(baseName)::refs, \
      ROCKET_REFLECT_MEMBERS_REFS__(cls, seq)); \
  }; \
  \
  static consteval auto& name() { \
    return ROCKET_REFLECT_MEMBERS_STRUCT__(name)::refs; \
  }

#define ROCKET_REFLECT_MEMBERS_DECLARE_FMT_FORMATTER__(ns, cls, name) \
  template<typename C> \
  struct fmt::formatter<ns::cls, C> { \
    template<typename FormatContext> \
    constexpr FormatContext::iterator \
    format(const ns::cls& val, FormatContext& ctx) const{ \
      auto out = ctx.out(); \
      if (withType_) { \
        const std::string typeName = fmt::format("{}", typeid(val)); \
        out = ::fmt::detail::write<C>(out, ::rocket::unicode::ConvertTo<C>::apply(typeName)); \
      } \
      out = ::rocket::reflect::internal::format<ns::cls, C>(val, ctx, debug_, ns::cls::name()); \
      return out; \
    } \
    \
    constexpr const C* \
    parse(parse_context<C>& ctx) { \
      auto it = ctx.begin(), end = ctx.end(); \
      if (it != end && *it == '?') { \
        debug_ = true; \
        ++it; \
      } \
      if (it != end && *it == 't') { \
        withType_ = true; \
        ++it; \
      } \
      return it; \
    } \
    \
    constexpr void \
    set_debug_format(bool val = true) { \
      debug_ = val; \
    } \
    \
  private: \
  \
    bool debug_ = false; \
    bool withType_ = false; \
  };

#define ROCKET_REFLECT_MEMBERS_DECLARE_OP_EQ__(cls, name) \
  inline bool \
  operator==(const cls& lhs, const cls& rhs) { \
    return ::rocket::reflect::eq(lhs, rhs, cls::name()); \
  }

#define ROCKET_REFLECT_MEMBERS_DECLARE_OP_NE__(cls, name) \
  inline bool \
  operator!=(const cls& lhs, const cls& rhs) { \
    return ::rocket::reflect::ne(lhs, rhs, cls::name()); \
  }

#define ROCKET_REFLECT_MEMBERS_DECLARE_OP_LT__(cls, name) \
  inline bool \
  operator<(const cls& lhs, const cls& rhs) { \
    return ::rocket::reflect::lt(lhs, rhs, cls::name()); \
  }

#define ROCKET_REFLECT_MEMBERS_DECLARE_OP_GT__(cls, name) \
  inline bool \
  operator>(const cls& lhs, const cls& rhs) { \
    return ::rocket::reflect::gt(lhs, rhs, cls::name()); \
  }

#define ROCKET_REFLECT_MEMBERS_DECLARE_OP_OUTPUT__(cls) \
  inline ::std::ostream& \
  operator<<(::std::ostream& lhs, const cls& rhs) { \
    return lhs << ::fmt::format("{}", rhs); \
  }

#define ROCKET_REFLECT_MEMBERS_DECLARE_MEMBER_REF_PROVIDER__(ns, cls, name) \
  template<> \
  struct rocket::reflect::MemberRefProvider<ns::cls> : ::std::true_type{ \
    static constexpr auto& refs = ns::cls::name(); \
  }

#define ROCKET_REFLECT_MEMBERS_DECLARE_STD_HASH__(ns, cls) \
  template<> \
  struct std::hash<ns::cls> { \
    u64 operator()(const ns::cls& val) const; \
  }

#define ROCKET_REFLECT_MEMBERS_DECLARE__(ns, cls, name) \
  ROCKET_REFLECT_MEMBERS_DECLARE_FMT_FORMATTER__(ns, cls, name); \
  ROCKET_NAMESPACE_BEGIN(ns); \
  ROCKET_REFLECT_MEMBERS_DECLARE_OP_EQ__(cls, name); \
  ROCKET_REFLECT_MEMBERS_DECLARE_OP_NE__(cls, name); \
  ROCKET_REFLECT_MEMBERS_DECLARE_OP_LT__(cls, name); \
  ROCKET_REFLECT_MEMBERS_DECLARE_OP_GT__(cls, name); \
  ROCKET_REFLECT_MEMBERS_DECLARE_OP_OUTPUT__(cls); \
  ROCKET_NAMESPACE_END(ns); \
  ROCKET_REFLECT_MEMBERS_DECLARE_MEMBER_REF_PROVIDER__(ns, cls, name); \
  ROCKET_REFLECT_MEMBERS_DECLARE_STD_HASH__(ns, cls)

#define ROCKET_REFLECT_MEMBERS_DEFINE_STD_HASH__(ns, cls, name) \
  u64 \
  std::hash<ns::cls>::operator()(const ns::cls& val) const { \
    return ::rocket::codec::HashEncoder().encode(val); \
  }

#define ROCKET_REFLECT_MEMBERS_DEFINE__(ns, cls, name) \
  ROCKET_REFLECT_MEMBERS_DEFINE_STD_HASH__(ns, cls, name)

// Variables ................................................................................................

#define ROCKET_REFLECT_VARS_ELEM__(r, data, elem) \
  (::rocket::reflect::VarRef(BOOST_PP_STRINGIZE(elem), elem))

#define ROCKET_REFLECT_VARS__(seq) \
  ::std::make_tuple(BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(ROCKET_REFLECT_VARS_ELEM__, ~, seq)))

/// @endcond

namespace rocket::reflect {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

template<typename T, typename C, u64 Index, typename FormatContext, typename Tuple>
constexpr FormatContext::iterator
formatElemImpl(const T& val, FormatContext& ctx, bool debug, const Tuple& refs) {
  using namespace fmt;

  // Write separator
  auto out = ctx.out();
  if constexpr (Index > 0) {
    out = detail::write<C>(out, static_cast<C>(','));
    out = detail::write<C>(out, static_cast<C>(' '));
  }

  // Get ref at index
  const auto& ref = std::get<Index>(refs);
  static_assert(IsMemberRef<decltype(ref)>::value);

  // Write name
  out = detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(ref.name()));
  out = detail::write<C>(out, static_cast<C>('='));

  // Write value
  const auto& value = ref.get(val);
  using ValueType = decltype(value);
  fmt::formatter<rocket::PurgeType<ValueType>, C> underlying;
  detail::maybe_set_debug_format(underlying, debug); // NOLINT
  ctx.advance_to(out);
  out = underlying.format(value, ctx);
  return out;
}

template<typename T, typename C, typename FormatContext, typename Tuple, u64... Index>
constexpr FormatContext::iterator
formatImpl(
  const T& val,
  FormatContext& ctx,
  bool debug,
  const Tuple& refs,
  std::index_sequence<Index...>) { // NOLINT
  using namespace fmt;

  // Write outer parentheses, inner members
  auto out = ctx.out();
  out = detail::write<C>(out, static_cast<C>('('));
  (..., (out = formatElemImpl<T, C, Index>(val, ctx, debug, refs)));
  return detail::write<C>(out, static_cast<C>(')'));
}

template<typename T, typename C, typename FormatContext, typename... Ref> requires
    (... && IsMemberRef<Ref>::value)
constexpr FormatContext::iterator
format(const T& val, FormatContext& ctx, bool debug, const std::tuple<Ref...>& refs) {
  return internal::formatImpl<T, C>(val, ctx, debug, refs, std::make_index_sequence<sizeof...(Ref)>());
}

template<u64 Index, typename T, typename Tuple>
constexpr auto&
refGet(T& val, const Tuple& refs) noexcept {
  auto& ref = std::get<Index>(refs);
  static_assert(IsMemberRef<decltype(ref)>::value);
  return ref.get(val);
}

template<typename T, typename Tuple, u64... Index>
bool
eqImpl(
  const T& lhs,
  const T& rhs,
  const Tuple& refs,
  std::index_sequence<Index...>) { // NOLINT
  return (... && std::equal_to()(refGet<Index>(lhs, refs), refGet<Index>(rhs, refs)));
}

template<typename T, typename Tuple, u64... Index>
bool
neImpl(
  const T& lhs,
  const T& rhs,
  const Tuple& refs,
  std::index_sequence<Index...>) { // NOLINT
  return (... || std::not_equal_to()(refGet<Index>(lhs, refs), refGet<Index>(rhs, refs)));
}

template<typename T, typename Tuple, u64... Index>
bool
ltImpl(
  const T& lhs,
  const T& rhs,
  const Tuple& refs,
  std::index_sequence<Index...> indices) { // NOLINT
  bool ret = false;
  (... ||
    ((ret = std::less()(refGet<Index>(lhs, refs), refGet<Index>(rhs, refs))) == true ||
      ((Index + 1 < indices.size()) &&
      std::less()(refGet<Index>(rhs, refs), refGet<Index>(lhs, refs)))));
  return ret;
}

template<typename T, typename Tuple, u64... Index>
bool
gtImpl(
  const T& lhs,
  const T& rhs,
  const Tuple& refs,
  std::index_sequence<Index...> indices) { // NOLINT
  bool ret = false;
  (... ||
    ((ret = std::greater()(refGet<Index>(lhs, refs), refGet<Index>(rhs, refs))) == true ||
      ((Index + 1 < indices.size()) &&
      std::greater()(refGet<Index>(rhs, refs), refGet<Index>(lhs, refs)))));
  return ret;
}

template<typename T, u64 Index, typename Tuple>
u64
writeElemImpl(nio::Sink& out, const T& val, const Tuple& refs) {
  // Write separator
  u64 ret = 0;
  if constexpr (Index > 0) {
    ret += out.write(", ");
  }

  // Get ref at index
  const auto& ref = std::get<Index>(refs);
  static_assert(IsMemberRef<decltype(ref)>::value);

  // Write name
  ret += out.write(ref.name());
  ret += out.write('=');

  // Write value
  const auto& value = ref.get(val);
  ret += out.print("{}", value);
  return ret;
}

template<typename T, typename Tuple, u64... Index>
u64
writeImpl(
  nio::Sink& out,
  const T& val,
  const Tuple& refs,
  std::index_sequence<Index...>) { // NOLINT
  u64 ret = out.write('(');
  (..., (ret += writeElemImpl<T, Index>(out, val, refs)));
  ret += out.write(')');
  return ret;
}

} // namespace internal

// Functions ------------------------------------------------------------------------------------------------

/**
 * `eq` function for member references.
 *
 * @param lhs the left-hand side
 * @param rhs the right-hand side
 * @param refs the references
 * @return whether @p lhs is equal to @p rhs as defined by #std::equal_to
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>::value)
inline bool
eq(const T& lhs, const T& rhs, const std::tuple<Ref...>& refs) {
  return internal::eqImpl(lhs, rhs, refs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * `hash` function for member references.
 *
 * @param val the instance
 * @param refs the references
 * @return a hash value
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>::value)
inline u64
hash(const T& val, const std::tuple<Ref...>& refs) {
  u64 ret = std::tuple_size<PurgeType<decltype(refs)>>::value;
  std::apply([&](auto&&... arg) {
    (..., (hash::combine(ret, rocket::codec::HashEncoder().encode(arg.get(val)))));
  }, refs);
  return ret;
}

/**
 * `ne`function for member references.
 *
 * @param lhs the left-hand side
 * @param rhs the right-hand side
 * @param refs the references
 * @return whether @p lhs is not equal to @p rhs as defined by #std::not_equal_to
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>::value)
inline bool
ne(const T& lhs, const T& rhs, const std::tuple<Ref...>& refs) {
  return internal::neImpl(lhs, rhs, refs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * `le` function for member references.
 *
 * @param lhs the left-hand side
 * @param rhs the right-hand side
 * @param refs the references
 * @return whether @p lhs is less than @p rhs as defined by #std::less
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>::value)
inline bool
lt(const T& lhs, const T& rhs, const std::tuple<Ref...>& refs) {
  return internal::ltImpl(lhs, rhs, refs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * `gt` function for member references.
 *
 * @param lhs the left-hand side
 * @param rhs the right-hand side
 * @param refs the references
 * @return whether @p lhs is greater than @p rhs as defined by #std::greater
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>::value)
inline bool
gt(const T& lhs, const T& rhs, const std::tuple<Ref...>& refs) {
  return internal::gtImpl(lhs, rhs, refs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * `write` function for member references.
 *
 * @param out the sink to write to
 * @param val the instance
 * @param refs the references
 * @return the number of bytes written
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>::value)
inline u64
write(nio::Sink& out, const T& val, const std::tuple<Ref...>& refs) {
  return internal::writeImpl(out, val, refs, std::make_index_sequence<sizeof...(Ref)>());
}

} // namespace rocket::reflect

// EOF
