/**
 * @file reflect.h
 *
 * C++ reflection support.
 */

#pragma once

#include "rocket/rocket.h"
#include "rocket/format/std.h"
#include "rocket/unicode/ConvertTo.h"

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/cat.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <ostream>
#include <tuple>
#include <type_traits>
#include <utility>

// Internal macros ------------------------------------------------------------------------------------------

/// @cond undocumented

// Members ..................................................................................................

#define ROCKET_REFLECT_MEMBERS_STRUCT__(name) BOOST_PP_SEQ_CAT((RocketReflect)(name)(__))

#define ROCKET_REFLECT_MEMBERS_MAKE_REFS_IMPL__(r, data, elem) \
    (::rocket::reflect::MemberRef(BOOST_PP_STRINGIZE(elem), &data::elem))

#define ROCKET_REFLECT_MEMBERS_MAKE_REFS__(cls, seq) \
    ::std::make_tuple( \
        BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(ROCKET_REFLECT_MEMBERS_MAKE_REFS_IMPL__, cls, seq)))

// Members (global) .........................................................................................

#define ROCKET_REFLECT_MEMBERS_DEFINE_FMT_FORMATTER__(cls, _name) \
    template<typename C> \
    struct fmt::formatter<cls, C> { \
      template<typename FormatContext> \
      constexpr FormatContext::iterator \
      format(const cls& v, FormatContext& ctx) const{ \
        auto out = ctx.out(); \
        if (withType_) { \
          auto type = ::rocket::Type::of(v); \
          detail::write<C>(out, ::rocket::unicode::ConvertTo<C>().apply(type.name())); \
        } \
        ::rocket::reflect::internal::format<cls, C>(v, ctx, debug_, cls::_name()); \
        return ctx.out(); \
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
      set_debug_format(bool v = true) { \
        debug_ = v; \
      } \
      \
    private: \
    \
      bool debug_ = false; \
      bool withType_ = false; \
    };

#define ROCKERT_REFLECT_MEMBERS_DEFINE_GLOBAL__(cls, name) \
    ROCKET_REFLECT_MEMBERS_DEFINE_FMT_FORMATTER__(cls, name)

// Members (local) ..........................................................................................

#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_EQ__(cls, name) \
    inline bool \
    operator==(const cls& lhs, const cls& rhs) { \
      return ::rocket::reflect::eq(lhs, cls::name(), rhs, cls::name()); \
    }

#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_NE__(cls, name) \
    inline bool \
    operator!=(const cls& lhs, const cls& rhs) { \
      return ::rocket::reflect::ne(lhs, cls::name(), rhs, cls::name()); \
    }

#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_LT__(cls, name) \
    inline bool \
    operator<(const cls& lhs, const cls& rhs) { \
      return ::rocket::reflect::lt(lhs, cls::name(), rhs, cls::name()); \
    }

#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_GT__(cls, name) \
    inline bool \
    operator>(const cls& lhs, const cls& rhs) { \
      return ::rocket::reflect::gt(lhs, cls::name(), rhs, cls::name()); \
    }

#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_OUTPUT__(cls) \
    inline ::std::ostream& \
    operator<<(::std::ostream& lhs, const cls& rhs) { \
      return lhs << ::fmt::format("{}", rhs); \
    }

#define ROCKERT_REFLECT_MEMBERS_DEFINE_LOCAL__(cls, name) \
    ROCKET_REFLECT_MEMBERS_DEFINE_OP_EQ__(cls, name) \
    ROCKET_REFLECT_MEMBERS_DEFINE_OP_NE__(cls, name) \
    ROCKET_REFLECT_MEMBERS_DEFINE_OP_LT__(cls, name) \
    ROCKET_REFLECT_MEMBERS_DEFINE_OP_GT__(cls, name) \
    ROCKET_REFLECT_MEMBERS_DEFINE_OP_OUTPUT__(cls)

// Variables ................................................................................................

#define ROCKET_REFLECT_VARS_MAKE_REFS_IMPL__(r, data, elem) \
    (::rocket::reflect::VarRef(BOOST_PP_STRINGIZE(elem), elem))

#define ROCKET_REFLECT_VARS_MAKE_REFS__(seq) \
    ::std::make_tuple( \
        BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(ROCKET_REFLECT_VARS_MAKE_REFS_IMPL__, ~, seq)))

/// @endcond

// Macros ---------------------------------------------------------------------------------------------------

// Members ..................................................................................................

/**
 * Provides access to a named member-reference container.
 *
 * @param cls the name of the class that holds the members (without namespace)
 * @param name the name for this member-reference container. e.g. `index`
 * @param seq a sequence of member names
 */
#define ROCKET_REFLECT_MEMBERS(cls, name, seq) \
    struct ROCKET_REFLECT_MEMBERS_STRUCT__(name) { \
      static constexpr auto refs = ROCKET_REFLECT_MEMBERS_MAKE_REFS__(cls, seq); \
    }; \
    \
    static consteval auto& name() { return ROCKET_REFLECT_MEMBERS_STRUCT__(name)::refs; } \

// Members (global) .........................................................................................

/**
 * Provides a `fmt::formatter for @p cls.
 *
 * @note This macro must be called in the global namespace.
 *
 * - If the `?` format specifier is used, then the formatter is set to debug mode.
 * - If the `t` format specifier is used, then the type of the instance is included.
 *
 * @param cls fully qualified name of the class, including namespace
 * @param name the name of the member-reference container to use
 */
#define ROCKET_REFLECT_MEMBERS_DEFINE_FMT_FORMATTER(cls, name) ROCKET_REFLECT_MEMBERS_DEFINE_FMT_FORMATTER__(cls, name)

/**
 * Provides all necessary definitions in the global namespace for full Rocket interoperability.
 *
 * @note This macro must be called in the global namespace, and prior to
 *     #ROCKERT_REFLECT_MEMBERS_DEFINE_LOCAL.
 *
 * @param cls fully qualified name of the class, including namespace
 * @param name the name of the member-reference container to use
 */
#define ROCKERT_REFLECT_MEMBERS_DEFINE_GLOBAL(cls, name) ROCKET_REFLECT_MEMBERS_DEFINE_GLOBAL__(cls, name)

// Members (local) ..........................................................................................

/**
 * Provides an `operator==` for class @p cls, using the member-reference container named @p name.
 *
 * @param cls name of the class that holds the members (without namespace)
 * @param name the name of the member-reference container to use
 */
#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_EQ(cls, name) ROCKET_REFLECT_MEMBERS_DEFINE_OP_EQ__(cls, name)

/**
 * Provides an `operator!=` for class @p cls, using the member-reference container named @p name.
 *
 * @param cls name of the class that holds the members (without namespace)
 * @param name the name of the member-reference container to use
 */
#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_NE(cls, name) ROCKET_REFLECT_MEMBERS_DEFINE_OP_NE__(cls, name)

/**
 * Provides an `operator<` for class @p cls, using the member-reference container named @p name.
 *
 * @param cls name of the class that holds the members (without namespace)
 * @param name the name of the member-reference container to use
 */
#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_LT(cls, name) ROCKET_REFLECT_MEMBERS_DEFINE_OP_LT__(cls, name)

/**
 * Provides an `operator>` for class @p cls, using the member-reference container named @p name.
 *
 * @param cls name of the class that holds the members (without namespace)
 * @param name the name of the member-reference container to use
 */
#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_GT(cls, name) ROCKET_REFLECT_MEMBERS_DEFINE_OP_GT__(cls, name)

/**
 * Provides an `operator<<` for class @p cls.
 *
 * Requires a preceding #ROCKET_REFLECT_MEMBERS_DEFINE_FMT_FORMATTER.
 *
 * @param cls name of the class that holds the members (without namespace)
 */
#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_OUTPUT(cls) ROCKET_REFLECT_MEMBERS_DEFINE_OP_OUTPUT__(cls)

/**
 * Provides all necessary definitions in the local namespace for full Rocket interoperability.
 *
 * @note This macro must be called in the local namespace, and after #ROCKERT_REFLECT_MEMBERS_DEFINE_GLOBAL.
 *
 * @param cls name of the class that holds the members (without namespace)
 * @param name the name of the member-reference container to use
 */
#define ROCKERT_REFLECT_MEMBERS_DEFINE_LOCAL(cls, name) ROCKET_REFLECT_MEMBERS_DEFINE_LOCAL__(cls, name)

// Variables ................................................................................................

/**
  * Provides access to a variable-reference container.
  *
  * @param seq a sequence of variable names
  */
#define ROCKET_REFLECT_VARS(seq) ROCKET_REFLECT_VARS_MAKE_REFS__(seq)

namespace rocket::reflect {

// `MemberRef` ..............................................................................................

/**
 * References on members that need an instance to evaluate. Instances of this class are returned by
 * #ROCKET_REFLECT_MEMBERS.
 */
template<typename C, typename T>
struct MemberRef {
  using ValueType = T;

  consteval MemberRef(const char* name, T C::* p) : name_(name), p_(p) {}

  constexpr T& get(C& v) const { return v.*p_; }

  constexpr const T& get(const C& v) const { return v.*p_; }

  constexpr std::string_view name() const { return name_; }

private:

  const std::string_view name_; ///< The name of the member.
  T C::*p_; ///< The pointer to the member.
};

template<typename T>
struct IsMemberRefImpl : std::false_type {};

template<typename C, typename T>
struct IsMemberRefImpl<MemberRef<C, T>> : std::true_type {};

template<typename T> struct IsMemberRef : IsMemberRefImpl<std::decay_t<T>>::type {};

// `VarRef` .................................................................................................

/**
 * References on variables that need need no instance to evaluate. Instances of this class are returned by
 * #ROCKET_REFLECT_VARS.
 */
template<typename T>
struct VarRef {
  using ValueType = T;

  constexpr VarRef(const char* name, T& ref) : name_(name), ref_(ref) {}

  bool operator==(const VarRef& rhs) const { return ref_ == rhs.ref_; }

  bool operator!=(const VarRef& rhs) const { return ref_ != rhs.ref_; }

  bool operator<(const VarRef& rhs) const { return ref_ < rhs.ref_; }

  bool operator<=(const VarRef& rhs) const { return ref_ <= rhs.ref_; }

  bool operator>(const VarRef& rhs) const { return ref_ > rhs.ref_; }

  bool operator>=(const VarRef& rhs) const { return ref_ >= rhs.ref_; }

  constexpr T& get() { return ref_; }

  constexpr const T& get() const { return ref_; }

  constexpr std::string_view name() const { return name_; }

  inline void reset() { get() = T(); }

private:

  const std::string_view name_;
  T& ref_;
};

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

template<typename T, typename C, size_t Index, typename FormatContext, typename Tuple>
void
formatMemberRefImpl(const T& v, FormatContext& ctx, bool debug, const Tuple& refs) {
  using namespace fmt;

  // Write separator
  auto out = ctx.out();
  if constexpr (Index > 0) {
    detail::write<C>(out, static_cast<C>(','));
    detail::write<C>(out, static_cast<C>(' '));
  }

  // Get ref at index
  const auto& ref = std::get<Index>(refs);
  static_assert(IsMemberRef<decltype(ref)>::value);

  // Write name
  detail::write<C>(out, rocket::unicode::ConvertTo<C>().apply(ref.name()));
  detail::write<C>(out, static_cast<C>('='));

  // Write value
  auto&& value = ref.get(v);
  using valueType = decltype(value);
  fmt::formatter<std::remove_cvref_t<valueType>, C> underlying;
  detail::maybe_set_debug_format(underlying, debug);
  underlying.format(value, ctx);
}

template<typename T, typename C, typename FormatContext, typename Tuple, size_t... Index>
void
formatImpl(const T& v, FormatContext& ctx, bool debug, const Tuple& refs, std::index_sequence<Index...>) {
  using namespace fmt;

  // Write outer parentheses, inner members
  auto out = ctx.out();
  detail::write<C>(out, static_cast<C>('('));
  (..., formatMemberRefImpl<T, C, Index>(v, ctx, debug, refs));
  detail::write<C>(out, static_cast<C>(')'));
}

template<typename T, typename C, typename FormatContext, typename... Ref> requires
    (... && IsMemberRef<Ref>::value)
void
format(const T& v, FormatContext& ctx, bool debug, const std::tuple<Ref...>& refs) {
  formatImpl<T, C>(v, ctx, debug, refs, std::make_index_sequence<sizeof...(Ref)>());
}

template<size_t Index, typename T, typename Tuple>
constexpr auto&
refGet(T& v, const Tuple& refs) noexcept {
  auto& ref = std::get<Index>(refs);
  static_assert(IsMemberRef<decltype(ref)>::value);
  return ref.get(v);
}

template<typename T, typename Tuple, size_t... Index>
bool
eqImpl(
    const T& lhs,
    const Tuple& lhsRefs,
    const T& rhs,
    const Tuple& rhsRefs,
    std::index_sequence<Index...>) {
  return (... && std::equal_to()(refGet<Index>(lhs, lhsRefs), refGet<Index>(rhs, rhsRefs)));
}

template<typename T, typename Tuple, size_t... Index>
bool
neImpl(
    const T& lhs,
    const Tuple& lhsRefs,
    const T& rhs,
    const Tuple& rhsRefs,
    std::index_sequence<Index...>) {
  return (... || std::not_equal_to()(refGet<Index>(lhs, lhsRefs), refGet<Index>(rhs, rhsRefs)));
}

template<typename T, typename Tuple, size_t... Index>
bool
ltImpl(
    const T& lhs,
    const Tuple& lhsRefs,
    const T& rhs,
    const Tuple& rhsRefs,
    std::index_sequence<Index...> indices) {
  bool ret = false;
  auto _unused = (... ||
      ((ret = std::less()(refGet<Index>(lhs, lhsRefs), refGet<Index>(rhs, rhsRefs))) == true ||
       ((Index + 1 < indices.size()) &&
        std::less()(refGet<Index>(rhs, rhsRefs), refGet<Index>(lhs, lhsRefs)))));
  nop(_unused);
  return ret;
}

template<typename T, typename Tuple, size_t... Index>
bool
gtImpl(
    const T& lhs,
    const Tuple& lhsRefs,
    const T& rhs,
    const Tuple& rhsRefs,
    std::index_sequence<Index...> indices) {
  bool ret = false;
  auto _unused = (... ||
      ((ret = std::greater()(refGet<Index>(lhs, lhsRefs), refGet<Index>(rhs, rhsRefs))) == true ||
       ((Index + 1 < indices.size()) &&
        std::greater()(refGet<Index>(rhs, rhsRefs), refGet<Index>(lhs, lhsRefs)))));
  nop(_unused);
  return ret;
}

} // namespace internal

// Functions ------------------------------------------------------------------------------------------------

/**
 * Tests if (@p lhs, @p lhsRefs) equals (@p rhs, @p rhsRefs).
 *
 * @param lhs the left instance
 * @param lhsRefs the left references
 * @param rhs the right instance
 * @param rhsRefs the right references
 * @return `true` if (@p lhs, @p lhsRefs) equals (@p rhs, @p rhsRefs)
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>::value)
inline bool
eq(const T& lhs, const std::tuple<Ref...>& lhsRefs, const T& rhs, const std::tuple<Ref...>& rhsRefs) {
  return internal::eqImpl(lhs, lhsRefs, rhs, rhsRefs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * Tests if (@p lhs, @p lhsRefs) does not equal (@p rhs, @p rhsRefs).
 *
 * @param lhs the left instance
 * @param lhsRefs the left references
 * @param rhs the right instance
 * @param rhsRefs the right references
 * @return `true` if (@p lhs, @p lhsRefs) does not equal (@p rhs, @p rhsRefs)
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>::value)
inline bool
ne(const T& lhs, const std::tuple<Ref...>& lhsRefs, const T& rhs, const std::tuple<Ref...>& rhsRefs) {
  return internal::neImpl(lhs, lhsRefs, rhs, rhsRefs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * Tests if (@p lhs, @p lhsRefs) is less than (@p rhs, @p rhsRefs).
 *
 * @param lhs the left instance
 * @param lhsRefs the left references
 * @param rhs the right instance
 * @param rhsRefs the right references
 * @return `true` if (@p lhs, @p lhsRefs) is less than (@p rhs, @p rhsRefs)
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>::value)
inline bool
lt(const T& lhs, const std::tuple<Ref...>& lhsRefs, const T& rhs, const std::tuple<Ref...>& rhsRefs) {
  return internal::ltImpl(lhs, lhsRefs, rhs, rhsRefs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * Tests if (@p lhs, @p lhsRefs) is greater than (@p rhs, @p rhsRefs).
 *
 * @param lhs the left instance
 * @param lhsRefs the left references
 * @param rhs the right instance
 * @param rhsRefs the right references
 * @return `true` if (@p lhs, @p lhsRefs) is greater than  (@p rhs, @p rhsRefs)
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>::value)
inline bool
gt(const T& lhs, const std::tuple<Ref...>& lhsRefs, const T& rhs, const std::tuple<Ref...>& rhsRefs) {
  return internal::gtImpl(lhs, lhsRefs, rhs, rhsRefs, std::make_index_sequence<sizeof...(Ref)>());
}

} // namespace rocket::reflect

// `fmt::formatter<rocket::reflect::VarRef>` ----------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::reflect::VarRef}
 *
 * This formatter uses the same format specifiers as the underlying formatter for type @p T.
 */
template<typename T, typename C> requires fmt::is_formattable<T, C>::value
struct fmt::formatter<rocket::reflect::VarRef<T>, C> {
  /// @cond undocumented

  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const rocket::reflect::VarRef<T>& v, FormatContext& ctx) const{
    auto out = ctx.out();
    out = detail::write<C>(out, rocket::unicode::ConvertTo<C>().apply(v.name()));
    out = detail::write<C>(out, static_cast<C>('='));
    out = underlying_.format(v.get(), ctx);
    return out;
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return underlying_.parse(ctx);
  }

  constexpr void
  set_debug_format(bool v = true) {
    detail::maybe_set_debug_format(underlying_, v);
  }

  /// @endcond

private:

  formatter<std::remove_cvref_t<T>, C> underlying_;
};

// EOF
