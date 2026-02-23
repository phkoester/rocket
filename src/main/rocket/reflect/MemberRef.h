/**
 * @file MemberRef.h
 *
 * C++ reflection support: member references.
 */

#pragma once

#include "rocket/type-traits.h"

namespace rocket::reflect {

// #MemberRef -----------------------------------------------------------------------------------------------

/**
 * References on members that need an instance to evaluate.
 *
 * Instances of this class are returned by #ROCKET_REFLECT_MEMBERS.
 */
template<typename C, typename T>
struct MemberRef {
  using ValueType = Purge<T>; ///< @type_alias

  /**
   * @ctor
   *
   * @param name the name of the member
   * @param p the pointer to the member
   */
  consteval MemberRef(const char* name, T C::* p) : name_(name), ptr_(p) {}

  /**
   * Returns the value of the member.
   *
   * @param instance the instance
   * @return the value of the member
   */
  [[nodiscard]] constexpr T& get(C& instance) const { return instance.*ptr_; }

  /**
   * Returns the value of the member.
   *
   * @param instance the instance
   * @return the value of the member
   */
  [[nodiscard]] constexpr const T& get(const C& instance) const { return instance.*ptr_; }

  /**
   * Returns the name of the member.
   *
   * @return the name of the member
   */
  [[nodiscard]] constexpr std::string_view name() const { return name_; }

private:

  std::string_view name_; ///< The name of the member.
  T C::*ptr_; ///< The pointer to the member.
};

} // namespace rocket::reflect

// EOF
