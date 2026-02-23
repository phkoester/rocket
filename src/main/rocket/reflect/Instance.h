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
  using Type = Purge<T>; ///< @type_alias
  using InnerType = Inner; ///< @type_alias

  /// @ctor_default
  Instance() = default;

  /**
   * @ctor
   *
   * @param instance the instance
   */
  explicit Instance(const Type& instance) : instance_(instance) {}

  /**
   * @ctor
   *
   * @param instance the instance
   */
  explicit Instance(Type&& instance) : instance_(std::move(instance)) {}

  /**
   * Returns a reference to the instance.
   *
   * @return a reference to the instance
   */
  [[nodiscard]] Type& get() { return instance_; }

  /**
   * Returns a reference to the instance.
   *
   * @return a reference to the instance
   */
  [[nodiscard]] const Type& get() const { return instance_; }

private:

  /// The instance.
  Type instance_;
};

} // namespace rocket::reflect

// EOF
