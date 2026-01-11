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
 */
template<typename T>
struct Cow {
  /**
   * @ctor
   *
   * @param ref the value to reference. The reference must remain valid for the lifetime of the `Cow`
   */
  Cow(const T& ref) : ptr_(&ref) {}

  Cow(const Cow& rhs) = delete;

  Cow(Cow&& rhs) = default;

  Cow& operator=(const Cow& rhs) = delete;

  Cow& operator=(Cow&& rhs) = default;

  /**
   * Assigns an owned value to the `Cow`.
   *
   * The value is copied into the `Cow` as an owned value.
   *
   * @param value the value to assign
   * @return_this
   */
  Cow&
  operator=(const T& value) {
    ptr_ = nullptr;
    owned_ = value;
    return *this;
  }

  /**
   * Assigns an owned value to the `Cow`.
   *
   * The value is moved into the `Cow` as an owned value.
   *
   * @param value the value to assign
   * @return_this
   */
  Cow&
  operator=(T&& value) {
    ptr_ = nullptr;
    owned_ = std::forward<T>(value);
    return *this;
  }

  /**
   * Returns a const reference to either the referenced or the owned value.
   *
   * @return a const reference to the referenced or the owned object
   */
  const T& operator*() const { return ptr_ ? *ptr_ : *owned_; }

  /**
   * Returns a const pointer to either the referenced or the owned value.
   *
   * @return a const pointer to the referenced or the owned object
   */
  const T* operator->() const { return ptr_ ? ptr_ : &*owned_; }

  /**
   * Returns a nonconst reference to the owned object.
   *
   * @note This requires that the `Cow` is no longer read-only, i.e. it has been assigned an owned value
   *     before.
   *
   * @return a reference to the owned object
   */
  T& get() { ROCKET_EXPECT(not readOnly()); return *owned_; }

  /**
   * Returns `true` if the `Cow` is read-only, i.e. it is referencing a value and has not been assigned an
   * owned value.
   *
   * @return `true` if the `Cow` is read-only
   */
  bool readOnly() const { return ptr_ != nullptr; }

private:

  const T* ptr_;
  std::optional<T> owned_;
};

} // namespace rocket

// EOF
