/**
 * @file Instance.h
 *
 * C++ reflection support: instances with member references.
 */

#pragma once

#include "rocket/type-traits.h"

namespace rocket::reflect {

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

  /// The member references, taken from the inner type.
  static constexpr auto& refs = Inner::refs;

  /**
   * @ctor
   *
   * @param instance the instance
   */
  Instance(T& instance) : ptr_(&instance) {}

  /**
   * @ctor
   *
   * @param instance the instance
   */
  Instance(const T& instance) : ptr_(const_cast<T*>(&instance)) {}

  T& get() { return *ptr_; }

  const T& get() const { return *ptr_; }

private:

  /// A pointer to the instance.
  T* ptr_;
};

} // namespace rocket::reflect

// EOF
