/**
 * @file Cow.h
 *
 * Copy-on-write values.
 */

#pragma once

#include <optional>

#include "rocket/assert.h"

namespace rocket {

// `Cow` ----------------------------------------------------------------------------------------------------

/**
 * A copy-on-write value.
 *
 * @tparam T the type of the value to reference
 * @tparam U the type of the owned value
 */
template<typename T, typename U = T>
struct Cow {
  /**
   * @ctor
   *
   * @param ref the value to reference. The reference must remain valid for the lifetime of the `Cow`
   */
  Cow(const T& ref) : ptr_(&ref) {}

  /// @ ctor_copy
  Cow(const Cow& rhs) = default;

  /// @ ctor_move
  Cow(Cow&& rhs) = default;

  /// @ member_op_assign_copy
  Cow& operator=(const Cow& rhs) = default;

  /// @ member_op_assign_move
  Cow& operator=(Cow&& rhs) = default;

  /**
   * Assigns an owned value to the `Cow`, rendering the instance as "modified".
   *
   * The value is copied into the `Cow` as an owned value.
   *
   * @param value the value to assign
   * @return_this
   */
  Cow&
  operator=(const U& value) {
    ptr_ = nullptr;
    owned_ = value;
    return *this;
  }

  /**
   * Assigns an owned value to the `Cow`, rendering the instance as "modified".
   *
   * The value is moved into the `Cow` as an owned value.
   *
   * @param value the value to assign
   * @return_this
   */
  Cow&
  operator=(U&& value) {
    ptr_ = nullptr;
    owned_ = std::forward<T>(value);
    return *this;
  }

  /**
   * Returns a const reference to either the referenced or the owned value.
   *
   * This overload only exists when the types @p T and @p U are the same.
   *
   * @return a const reference to the referenced or the owned object
   */
  template<typename V = T> requires std::is_same_v<V, T> && std::is_same_v<T, U>
  const V&
  get() const {
    return ptr_ ? *ptr_ : *owned_;
  }

  /**
   * Returns a view to either the referenced or the owned value.
   *
   * This overload only exists when the types @p T and @p U are different.
   *
   * @return a view to either the referenced or the owned object
   */
  template<typename V = T> requires std::is_same_v<V, T> && (not std::is_same_v<T, U>)
  V
  get() const {
    return ptr_ ? *ptr_ : V(*owned_);
  }

  /**
   * Returns `true` if the `Cow` has been assigned an owned value.
   *
   * @return `true` if the `Cow` has been assigned an owned value
   */
  bool modified() const { return ptr_ == nullptr; }

  /**
   * Returns a nonconst reference to the owned value.
   *
   * @note This requires that the `Cow` is "modified", i.e. it has been assigned an owned value.
   *
   * @return a nonconst reference to the owned value
   * @throw #rocket::InvalidState if the `Cow` has not been assigned an owned value
   */
  U& owned() { ROCKET_EXPECT(modified()); return *owned_; }

private:

  const T* ptr_;
  std::optional<U> owned_;
};

} // namespace rocket

// EOF
