/**
 * @file MemberRef.h
 *
 * C++ reflection: member references.
 */

#pragma once

#include "rocket/type-traits.h"

#include <string_view>

namespace rocket::reflect {

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

// #IsMemberRef ---------------------------------------------------------------------------------------------

template<typename T>
struct IsMemberRefImpl : std::false_type {};

template<typename C, typename T>
struct IsMemberRefImpl<MemberRef<C, T>> : std::true_type {};

template<typename T> concept IsMemberRef = IsMemberRefImpl<PurgeType<T>>::value;

} // namespace rocket::reflect

// EOF
