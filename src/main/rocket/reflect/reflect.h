/**
 * @file reflect.h
 *
 * C++ reflection support: member and variable references.
 */

#pragma once

#include "rocket/macro.h"
#include "rocket/type-traits.h"

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/cat.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <ostream>
#include <tuple>

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
  }

#define ROCKET_REFLECT_MEMBERS_DERIVED__(baseCls, baseName, cls, name, seq) \
  struct name { \
    static constexpr auto refs = ::std::tuple_cat( \
      baseCls::baseName::refs, \
      ROCKET_REFLECT_MEMBERS_REFS__(cls, seq)); \
  }

#define ROCKET_REFLECT_MEMBERS_DECLARE_DECLARED__(ns, cls, name) \
  template<> \
  struct rocket::reflect::Declared<ns::cls> : ::std::true_type{ \
    static constexpr auto& refs = ns::cls::name::refs; \
  }

#define ROCKET_REFLECT_MEMBERS_DECLARE_OP_EQ__(cls) \
  bool operator==(const cls& lhs, const cls& rhs)

#define ROCKET_REFLECT_MEMBERS_DECLARE_OP_CMP__(cls, name) \
  std::partial_ordering operator<=>(const cls& lhs, const cls& rhs)

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
  std::partial_ordering \
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

// #Declared ------------------------------------------------------------------------------------------------

/**
 * This template provides access to default member references of a type.
 */
template<typename T>
struct Declared : std::false_type {};

// #Instance ------------------------------------------------------------------------------------------------

/**
 * A #rocket::reflect::Instance is an instance together with specified member references.
 *
 * @tparam T the type of the instance
 * @tparam Inner the inner type of @p T that holds the member references
 */
template<typename T, typename Inner>
struct Instance {
  using Type = T; ///< @type_alias
  using InnerType = Inner; ///< @type_alias
  using PointerType = Purge<Type>*;

  /// The member references, taken from the inner type.
  static constexpr auto& refs = Inner::refs;

  /// A pointer to the instance.
  PointerType instance;

  /**
   * @ctor
   *
   * @param instance the instance
   */
  Instance(T& instance) : instance(&instance) {}

   /**
   * @ctor
   *
   * @param instance the instance
   */
  Instance(const T& instance) : instance(const_cast<PointerType>(&instance)) {}
};

// #MemberRef -----------------------------------------------------------------------------------------------

/**
 * References on members that need an instance to evaluate.
 *
 * Instances of this class are returned by #ROCKET_REFLECT_MEMBERS.
 */
template<typename C, typename T>
struct MemberRef {
  using ValueType = T; ///< @type_alias

  /**
   * @ctor
   *
   * @param name the name of the member
   * @param p the pointer to the member
   */
  consteval MemberRef(const char* name, T C::* p) : name_(name), p_(p) {}

  /**
   * Returns the value of the member.
   *
   * @param val the instance
   * @return the value of the member
   */
  [[nodiscard]] constexpr T& get(C& val) const { return val.*p_; }

  /**
   * Returns the value of the member.
   *
   * @param val the instance
   * @return the value of the member
   */
  [[nodiscard]] constexpr const T& get(const C& val) const { return val.*p_; }

  /**
   * Returns the name of the member.
   *
   * @return the name of the member
   */
  [[nodiscard]] constexpr std::string_view name() const { return name_; }

private:

  std::string_view name_; ///< The name of the member.
  T C::*p_; ///< The pointer to the member.
};

// #VarRef --------------------------------------------------------------------------------------------------

/**
 * References on variables that need need no instance to evaluate.
 *
 * Instances of this class are returned by #ROCKET_REFLECT_VARS.
 *
 * @param T the type of the variable
 */
template<typename T>
struct VarRef {
  using ValueType = T; ///< @type_alias

  /**
   * @ctor
   *
   * @param name the name of the variable
   * @param ref the reference to the variable
   */
  constexpr VarRef(const char* name, T& ref) : name_(name), ptr_(&ref) {}

  /**
   * Returns the value of the variable.
   *
   * @return the value of the variable
   */
  [[nodiscard]] constexpr T& get() { return *ptr_; }

  /**
   * Returns the value of the variable.
   *
   * @return the value of the variable
   */
  [[nodiscard]] constexpr const T& get() const { return *ptr_; }

  /**
   * Returns the name of the variable.
   *
   * @return the name of the variable
   */
  [[nodiscard]] constexpr std::string_view name() const { return name_; }

private:

  std::string_view name_;
  T* ptr_;
};

} // namespace rocket::reflect

// EOF
