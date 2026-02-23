/**
 * @file VarRef.h
 *
 * C++ reflection support: variable references.
 */

#pragma once

#include "rocket/type-traits.h"

namespace rocket::reflect {

// #VarRef --------------------------------------------------------------------------------------------------

/**
 * References on variables.
 *
 * Instances of this class are returned by #ROCKET_REFLECT_VARS.
 *
 * @param T the type of the variable
 */
template<typename T>
struct VarRef {
  using ValueType = Purge<T>; ///< @type_alias

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
