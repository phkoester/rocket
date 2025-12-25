/**
 * @file reflect.h
 *
 * C++ reflection support.
 */

#pragma once

#include "S.h"
#include "assert.h"
#include "codec.h"
#include "concept.h"

#include <boost/functional/hash.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/cat.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <iosfwd>
#include <type_traits>
#include <utility>

// Internal macros ------------------------------------------------------------------------------------------

/// @cond undocumented

// Members ..................................................................................................

#define ROCKET_REFLECT_MEMBERS_STRUCT__(name) BOOST_PP_SEQ_CAT((RocketReflect)(name)(__))

#define ROCKET_REFLECT_MEMBERS_MAKE_REFS_IMPL__(r, data, elem) \
    (::rocket::reflect::internal::MemberRef(BOOST_PP_STRINGIZE(elem), &data::elem))

#define ROCKET_REFLECT_MEMBERS_MAKE_REFS__(cls, seq) \
    ::std::make_tuple( \
        BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(ROCKET_REFLECT_MEMBERS_MAKE_REFS_IMPL__, cls, seq)))

#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_EQ__(cls, name) \
    inline bool \
    operator==(const cls& lhs, const cls& rhs) { \
      return ::rocket::reflect::eq(&lhs, cls::name(), &rhs, cls::name()); \
    }

#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_NE__(cls, name) \
    inline bool \
    operator!=(const cls& lhs, const cls& rhs) { \
      return ::rocket::reflect::ne(&lhs, cls::name(), &rhs, cls::name()); \
    }

#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_LT__(cls, name) \
    inline bool \
    operator<(const cls& lhs, const cls& rhs) { \
      return ::rocket::reflect::lt(&lhs, cls::name(), &rhs, cls::name()); \
    }

#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_GT__(cls, name) \
    inline bool \
    operator>(const cls& lhs, const cls& rhs) { \
      return ::rocket::reflect::gt(&lhs, cls::name(), &rhs, cls::name()); \
    }

#define ROCKET_REFLECT_MEMBERS_DEFINE_FN_HASH_VALUE__(cls, name) \
    inline size_t \
    hash_value(const cls& v) { \
      return ::rocket::reflect::hash(&v, cls::name()); \
    }

#define ROCKET_REFLECT_MEMBERS_DEFINE_FN_PARSE_RON__(cls, name) \
    inline ::std::istream& \
    parseRon(::std::istream& is, cls& v) { \
      return ::rocket::reflect::parseRon(is, v, cls::name()); \
    }

#define ROCKET_REFLECT_MEMBERS_DEFINE_FN_PRINT_RON__(cls, name) \
    inline ::std::ostream& \
    printRon(::std::ostream& os, const cls& v) { \
      return ::rocket::reflect::printRon(os, &v, cls::name()); \
    }

// Variables ................................................................................................

#define ROCKET_REFLECT_VARS_MAKE_REFS_IMPL__(r, data, elem) \
    (::rocket::reflect::internal::VarRef(BOOST_PP_STRINGIZE(elem), elem))

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
 * @param name the name for this member-reference container
 * @param seq a sequence of member names
 */
#define ROCKET_REFLECT_MEMBERS(cls, name, seq) \
    struct ROCKET_REFLECT_MEMBERS_STRUCT__(name) { \
      static constexpr auto refs = ROCKET_REFLECT_MEMBERS_MAKE_REFS__(cls, seq); \
    }; \
    \
    static consteval auto& name() { return ROCKET_REFLECT_MEMBERS_STRUCT__(name)::refs; } \

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
#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_GT(cls, name) ROCKET_REFLECT_MEMBERS_DEFINE_OP_GT__(cls, name)
/**
 * Provides an `operator>` for class @p cls, using the member-reference container named @p name.
 *
 * @param cls name of the class that holds the members (without namespace)
 * @param name the name of the member-reference container to use
 */
#define ROCKET_REFLECT_MEMBERS_DEFINE_OP_LT(cls, name) ROCKET_REFLECT_MEMBERS_DEFINE_OP_LT__(cls, name)

/**
 * Provides a `hash_value` function for class @p cls, using the member-reference container named @p name.
 *
 * @param cls name of the class that holds the members (without namespace)
 * @param name the name of the member-reference container to use
 */
#define ROCKET_REFLECT_MEMBERS_DEFINE_FN_HASH_VALUE(cls, name) \
    ROCKET_REFLECT_MEMBERS_DEFINE_FN_HASH_VALUE__(cls, name)

/**
 * Provides a `parseRon` function for class @p cls, using the member-reference container named @p name.
 *
 * @param cls name of the class that holds the members (without namespace)
 * @param name the name of the member-reference container to use
 */
#define ROCKET_REFLECT_MEMBERS_DEFINE_FN_PARSE_RON(cls, name) \
    ROCKET_REFLECT_MEMBERS_DEFINE_FN_PARSE_RON__(cls, name)

/**
 * Provides a `printRon` function for class @p cls, using the member-reference container named @p name.
 *
 * @param cls name of the class that holds the members (without namespace)
 * @param name the name of the member-reference container to use
 */
#define ROCKET_REFLECT_MEMBERS_DEFINE_FN_PRINT_RON(cls, name) \
    ROCKET_REFLECT_MEMBERS_DEFINE_FN_PRINT_RON__(cls, name)

// Variables ................................................................................................

/**
  * Provides access to a variable-reference container.
  *
  * @param seq a sequence of variable names
  */
#define ROCKET_REFLECT_VARS(seq) ROCKET_REFLECT_VARS_MAKE_REFS__(seq)

namespace rocket::reflect {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

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

  const std::string_view name_;
  T C::*p_;
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

  constexpr T& get() { return ref_; }

  constexpr const T& get() const { return ref_; }

  constexpr std::string_view name() const { return name_; }

  inline void reset() { get() = T(); }

private:

  const std::string_view name_;
  T& ref_;
};

template<typename T>
struct IsVarRefImpl : std::false_type {};

template<typename T>
struct IsVarRefImpl<VarRef<T>> : std::true_type {};

template<typename T> struct IsVarRef : IsVarRefImpl<std::decay_t<T>>::type {};

// Functions ................................................................................................

template<size_t Index, typename T, typename Tuple>
constexpr auto&
refGet(T* v, const Tuple& refs) noexcept {
  auto& ref = std::get<Index>(refs);

  static_assert(IsMemberRef<decltype(ref)>::value || IsVarRef<decltype(ref)>::value);

  if constexpr (IsMemberRef<decltype(ref)>::value)
    return ref.get(*v);
  else
    return ref.get();
}

template<size_t Index, typename Tuple>
constexpr std::string_view
refName(const Tuple& refs) noexcept {
  auto& ref = std::get<Index>(refs);

  static_assert(IsMemberRef<decltype(ref)>::value || IsVarRef<decltype(ref)>::value);

  return ref.name();
}

template<typename T, typename Tuple, size_t... Index>
bool
eq(
    const T* lhs,
    const Tuple& lhsRefs,
    const T* rhs,
    const Tuple& rhsRefs,
    std::index_sequence<Index...>) {
  return (... && std::equal_to()(refGet<Index>(lhs, lhsRefs), refGet<Index>(rhs, rhsRefs)));
}

template<typename T, typename Tuple, size_t... Index>
bool
ne(
    const T* lhs,
    const Tuple& lhsRefs,
    const T* rhs,
    const Tuple& rhsRefs,
    std::index_sequence<Index...>) {
  return (... || std::not_equal_to()(refGet<Index>(lhs, lhsRefs), refGet<Index>(rhs, rhsRefs)));
}

template<typename T, typename Tuple, size_t... Index>
bool
lt(
    const T* lhs,
    const Tuple& lhsRefs,
    const T* rhs,
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
gt(
    const T* lhs,
    const Tuple& lhsRefs,
    const T* rhs,
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

template<typename T, typename Tuple, size_t... Index>
size_t
hash(const T* v, const Tuple& refs, std::index_sequence<Index...>) {
  size_t ret = 0;
  (..., ::boost::hash_combine(ret, refGet<Index>(v, refs)));
  return ret;
}

template<size_t Size, size_t Index = 0, typename T, typename Tuple>
std::istream&
parseRonImpl(
    std::istream& is, size_t pos, size_t first, size_t last, T* v, Tuple& refs, std::string_view name) {
  if constexpr (Index < Size) {
    if (refName<Index>(refs) == name) {
      auto& value = refGet<Index>(v, refs);
      return parseRon(is, value);
    } else {
      // Go on with next tuple element using template recursion
      return parseRonImpl<Size, Index + 1>(is, pos, first, last, v, refs, name);
    }
  }
  else
    throw except::ParseFailure<char>(is, first, { first, last }, S << "Invalid name: " << name);
}

template<typename T, typename Tuple, size_t... Index>
std::istream&
parseRon(std::istream& is, T* v, Tuple& refs, std::index_sequence<Index...>) {
  using namespace codec;

  if constexpr (IsMemberRef<decltype(std::get<0>(refs))>::value) {
    // Member references: Simply default-construct `*v`
    ROCKET_ASSERT(v, "Member references must be used with an instance");
    *v = T();
  }
  else {
    // Variable references: Reset each reference in the tuple
    ROCKET_ASSERT(not v, "Variable references must be used without an instance, i.e. `nullptr`");
    (..., std::get<Index>(refs).reset());
  }

  ron::parsing::skip(is);
  size_t inputPos = io::tellg(is);
  io::getChar(is, '{');

  bool first = true;
  while (true) {
    ron::parsing::skip(is);
    auto right = io::getOptionalChar(is, '}');
    if (right)
      return is;

    if (first)
      first = false;
    else
      io::getChar(is, ',');

    ron::parsing::skip(is);
    size_t nameFirst = io::tellg(is);
    std::string name = io::getUntil<char>(
        is,
        [](char c) { return std::isspace(c) || c == '=' || c == ',' || c == '}'; },
        "whitespace, '=', ',', or '}'",
        false,
        1);
    size_t nameLast = nameFirst + name.size();

    ron::parsing::skip(is);
    io::getChar(is, '=');

    parseRonImpl<std::tuple_size_v<Tuple>>(is, inputPos, nameFirst, nameLast, v, refs, name);
  }
}

template<size_t Index, typename T, typename Tuple>
void
printRonImpl(std::ostream& os, bool indentChildren, const T* v, const Tuple& refs) {
  using namespace codec::ron::printing;
  if constexpr (Index == 0)
    firstChild(os, indentChildren);
  else
    nextChild(os, indentChildren, ", ", ",");
  os << refName<Index>(refs) << '=';
  printRon(os, refGet<Index>(v, refs));
}

template<typename T, typename Tuple, size_t... Index>
std::ostream&
printRon(
    std::ostream& os,
    bool indentChildren,
    const T* v,
    const Tuple& refs,
    std::index_sequence<Index...>) {
  using namespace codec::ron::printing;
  os << '{'; {
    ROCKET_CODEC_RON_PRINT_CHILDREN();
    (..., printRonImpl<Index>(os, indentChildren, v, refs));
  }
  return endParent(os, indentChildren, '}');
}

} // namespace internal

// `Reference` ----------------------------------------------------------------------------------------------

/**
 * A concept for types that are considered member or variable references.
 *
 * @tparam T the type to test
 */
template<typename T>
concept Reference = internal::IsMemberRef<T>::value || internal::IsVarRef<T>::value;

// Functions ------------------------------------------------------------------------------------------------

/**
 * Tests if (@p lhs, @p lhsRefs) equals (@p rhs, @p rhsRefs).
 *
 * @param lhs pointer to the left instance, or `nullptr` for variable references
 * @param lhsRefs the left references
 * @param rhs pointer to the right instance, or `nullptr` for variable references
 * @param rhsRefs the right references
 * @return `true` if (@p lhs, @p lhsRefs) equals (@p rhs, @p rhsRefs)
 */
template<typename T, typename... Ref> requires (... && Reference<Ref>)
inline bool
eq(const T* lhs, const std::tuple<Ref...>& lhsRefs, const T* rhs, const std::tuple<Ref...>& rhsRefs) {
  return internal::eq(lhs, lhsRefs, rhs, rhsRefs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * Tests if (@p lhs, @p lhsRefs) does not equal (@p rhs, @p rhsRefs).
 *
 * @param lhs pointer to the left instance, or `nullptr` for variable references
 * @param lhsRefs the left references
 * @param rhs pointer to the right instance, or `nullptr` for variable references
 * @param rhsRefs the right references
 * @return `true` if (@p lhs, @p lhsRefs) does not equal (@p rhs, @p rhsRefs)
 */
template<typename T, typename... Ref> requires (... && Reference<Ref>)
inline bool
ne(const T* lhs, const std::tuple<Ref...>& lhsRefs, const T* rhs, const std::tuple<Ref...>& rhsRefs) {
  return internal::ne(lhs, lhsRefs, rhs, rhsRefs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * Tests if (@p lhs, @p lhsRefs) is less than (@p rhs, @p rhsRefs).
 *
 * @param lhs pointer to the left instance, or `nullptr` for variable references
 * @param lhsRefs the left references
 * @param rhs pointer to the right instance, or `nullptr` for variable references
 * @param rhsRefs the right references
 * @return `true` if (@p lhs, @p lhsRefs) is less than (@p rhs, @p rhsRefs)
 */
template<typename T, typename... Ref> requires (... && Reference<Ref>)
inline bool
lt(const T* lhs, const std::tuple<Ref...>& lhsRefs, const T* rhs, const std::tuple<Ref...>& rhsRefs) {
  return internal::lt(lhs, lhsRefs, rhs, rhsRefs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * Tests if (@p lhs, @p lhsRefs) is greater than (@p rhs, @p rhsRefs).
 *
 * @param lhs pointer to the left instance, or `nullptr` for variable references
 * @param lhsRefs the left references
 * @param rhs pointer to the right instance, or `nullptr` for variable references
 * @param rhsRefs the right references
 * @return `true` if (@p lhs, @p lhsRefs) is greater than  (@p rhs, @p rhsRefs)
 */
template<typename T, typename... Ref> requires (... && Reference<Ref>)
inline bool
gt(const T* lhs, const std::tuple<Ref...>& lhsRefs, const T* rhs, const std::tuple<Ref...>& rhsRefs) {
  return internal::gt(lhs, lhsRefs, rhs, rhsRefs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * Calculates a hash value for (@p v, @p refs).
 *
 * @param v pointer to the instance, or `nullptr` for variable references
 * @param refs the references
 * @return a hash value
 */
template<typename T, typename... Ref> requires (... && Reference<Ref>)
inline size_t
hash(const T* v, const std::tuple<Ref...>& refs) {
  return internal::hash(v, refs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * Parses (@p v, @p refs) as RON.
 *
 * This overload is for member references. In this case, the tuple may be a const.
 *
 * @param is the output stream
 * @param v the instance
 * @param refs the member references
 * @return @p is
 */
template<typename T, typename... Ref> requires (... && internal::IsMemberRef<Ref>::value)
inline std::istream&
parseRon(std::istream& is, T& v, const std::tuple<Ref...>& refs) {
  return internal::parseRon(is, &v, refs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * Parses @p refs as RON.
 *
 * This overload is for variable references.
 *
 * @param is the output stream
 * @param refs the variable references
 * @return @p is
 */
template<typename T, typename... Ref> requires (... && internal::IsVarRef<Ref>::value)
inline std::istream&
parseRon(std::istream& is, std::tuple<Ref...>& refs) {
  return internal::parseRon(is, nullptr, refs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * Prints (@p v, @p refs) as RON.
 *
 * @param os the output stream
 * @param v pointer to the instance, or `nullptr` for variable references
 * @param refs the references
 * @return @p os
 */
template<typename T, typename... Ref> requires (... && Reference<Ref>)
inline std::ostream&
printRon(std::ostream& os, const T* v, const std::tuple<Ref...>& refs) {
  constexpr bool indentChildren = (... || IsContainer<typename Ref::ValueType>::value);
  return internal::printRon(os, indentChildren, v, refs, std::make_index_sequence<sizeof...(Ref)>());
}

} // namespace rocket::reflect

// EOF
