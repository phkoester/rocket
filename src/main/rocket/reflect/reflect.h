/**
 * @file reflect.h
 *
 * C++ reflection support: members and variables.
 */

#pragma once

#include "rocket/codec/CompareEncoder.h"
#include "rocket/codec/EqualToEncoder.h"
#include "rocket/codec/HashEncoder.h"

#include <functional> // `std::hash`

// Macros ---------------------------------------------------------------------------------------------------

// Members ..................................................................................................

/**
 * Provides access to a named member-reference container.
 *
 * @note This macro must be called inside the class declaration, in a public section.
 *
 * @param cls the name of the class that holds the members (without namespace)
 * @param name the name of the member-reference container, e.g., `Index`
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
 *
 * @note This macro must be called in the global namespace.
 *
 * @param ns the namespace of the class, e.g., `mynamespace`. May be left empty if the class is in the global
 *   namespace
 * @param cls the type of the class without namespace, e.g., `MyClass`
 * @param name the name of the member-reference container to use, e.g., `Index`
*/
#define ROCKET_REFLECT_MEMBERS_DECLARE(ns, cls, name) ROCKET_REFLECT_MEMBERS_DECLARE__(ns, cls, name)

/**
 * Provides all the definitions for the class @p cls needed for full Rocket interoperability.
 *
 * @note This macro must be called in the global namespace.
 *
 * @param ns the namespace of the class, e.g., `mynamespace`. May be left empty if the class is in the global
 *   namespace
 * @param cls the type of the class without namespace, e.g., `MyClass`
 * @param name the name of the member-reference container to use, e.g., `Index`
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
 * @param name the name for this member-reference container. e.g., `Index`
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
    using Ordering = ::rocket::codec::internal::compare::CmpCommonOrdering<Cmp, Purge<decltype(refs)>>; \
  }

#define ROCKET_REFLECT_MEMBERS_DERIVED__(baseCls, baseName, cls, name, seq) \
  struct name { \
    static constexpr auto refs = ::std::tuple_cat( \
      baseCls::baseName::refs, \
      ROCKET_REFLECT_MEMBERS_REFS__(cls, seq)); \
    \
    using Cmp = ::rocket::StdCompare; \
    using Ordering = ::rocket::codec::internal::compare::CmpCommonOrdering<Cmp, Purge<decltype(refs)>>; \
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

#define ROCKET_REFLECT_MEMBERS_DECLARE__(ns, cls, name) \
  ROCKET_REFLECT_MEMBERS_DECLARE_DECLARED__(ns, cls, name); \
  ROCKET_NAMESPACE_BEGIN(ns); \
  ROCKET_REFLECT_MEMBERS_DECLARE_OP_EQ__(cls); \
  ROCKET_REFLECT_MEMBERS_DECLARE_OP_CMP__(cls, name); \
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

#define ROCKET_REFLECT_MEMBERS_DEFINE__(ns, cls, name) \
  ROCKET_NAMESPACE_BEGIN(ns); \
  ROCKET_REFLECT_MEMBERS_DEFINE_OP_EQ__(cls); \
  ROCKET_REFLECT_MEMBERS_DEFINE_OP_CMP__(cls, name); \
  ROCKET_NAMESPACE_END(ns)

// Variables ................................................................................................

#define ROCKET_REFLECT_VARS_ELEM__(r, data, elem) \
  (::rocket::reflect::VarRef(BOOST_PP_STRINGIZE(elem), elem))

#define ROCKET_REFLECT_VARS__(seq) \
  ::std::make_tuple(BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(ROCKET_REFLECT_VARS_ELEM__, ~, seq)))

/// @endcond

namespace rocket::reflect {

// `Instance` -----------------------------------------------------------------------------------------------

/// @op_eq{#rocket::reflect::Instance}
template<typename T, typename Inner>
inline bool
operator==(const Instance<T, Inner>& lhs, const Instance<T, Inner>& rhs) {
  return codec::EqualToEncoder<>().encode(lhs, rhs);
}

/// @op_cmp{#rocket::reflect::Instance}
template<typename T, typename Inner>
inline auto
operator<=>(const Instance<T, Inner>& lhs, const Instance<T, Inner>& rhs) {
  return codec::CompareEncoder<>().encode(lhs, rhs);
}

// `VarRef` -------------------------------------------------------------------------------------------------

/// @op_eq{#rocket::reflect::VarRef}
template<typename T>
inline bool
operator==(const VarRef<T>& lhs, const VarRef<T>& rhs) {
  return codec::EqualToEncoder<>().encode(lhs.get(), rhs.get());
}

/// @op_cmp{#rocket::reflect::VarRef}
template<typename T>
inline auto
operator<=>(const VarRef<T>& lhs, const VarRef<T>& rhs) {
  return codec::CompareEncoder<>().encode(lhs, rhs);
}

} // namespace rocket::reflect

// `std::hash<rocket::reflect::Declared>` -------------------------------------------------------------------

/// @spec_std_hash{#rocket::reflect::Declared}
template<typename T> requires rocket::reflect::Declared<T>::value
struct std::hash<T> {
  /// @cond undocumented
  rocket::std_size_t
  operator()(const T& val) const {
    return static_cast<rocket::std_size_t>(rocket::codec::HashEncoder<>().encode(val));
  }
  /// @endcond
};

// `std::hash<rocket::reflect::Instance>` -------------------------------------------------------------------

/// @spec_std_hash{#rocket::reflect::Instance}
template<typename T, typename Inner>
struct std::hash<rocket::reflect::Instance<T, Inner>> {
  /// @cond undocumented
  using Type = rocket::reflect::Instance<T, Inner>;

  rocket::std_size_t
  operator()(const Type& val) const {
    return static_cast<rocket::std_size_t>(rocket::codec::HashEncoder<>().encode(val));
  }
  /// @endcond
};

// `std::hash<rocket::reflect::VarRef>` ---------------------------------------------------------------------

/// @spec_std_hash{#rocket::reflect::VarRef}
template<typename T>
struct std::hash<rocket::reflect::VarRef<T>> {
  /// @cond undocumented
  using Type = rocket::reflect::VarRef<T>;

  rocket::std_size_t
  operator()(const Type& val) const {
    return static_cast<rocket::std_size_t>(rocket::codec::HashEncoder<>().encode(val));
  }
  /// @endcond
};

// EOF
