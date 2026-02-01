/**
 * @file Guard.h
 *
 * Guards, utilizing C++ constructors and destructors.
 */

#pragma once

#include "macro.h"

#include <functional>

namespace rocket {

// #Guard ---------------------------------------------------------------------------------------------------

/**
 * An object that executes a function in its destructor, i.e. when it goes out of scope.
 *
 * Use the #ROCKET_GUARD macro for your convenience.
 */
struct Guard {
  /**
   * @ctor
   *
   * @param fn the function to execute upon scope exit
   */
  explicit Guard(std::function<void()>&& fn) : fn_(std::move(fn)) {}

  /**
   * @dtor
   *
   * This destructor executes the function that was passed to the constructor.
   */
  ~Guard() { fn_(); }

private:

  std::function<void()> fn_;
};

/**
 * Makes a #rocket::Guard instance implicitly.
 *
 * @param fn the function to execute upon scope exit
 */
#define ROCKET_GUARD(fn) const ::rocket::Guard ROCKET_ID()(fn)

// #ValueGuard ----------------------------------------------------------------------------------------------

/**
 * An object that immediately assigns a new value to a variable and restores the old value in its destructor,
 * i.e. when it goes out of scope.
 *
 * Use the #ROCKET_VALUE_GUARD macro for your convenience.
 *
 * @tparam T the type of the value
 */
template<typename T>
struct ValueGuard {
  /**
   * @ctor
   *
   * @param ref a reference to the variable that is to be assigned
   * @param newValue the new value to assign immediately
   */
  ValueGuard(T& ref, T&& newValue) : // NOLINT(*-param-not-moved)
      ref_(ref),
      oldValue_(ref) {
    ref = std::forward<T>(newValue);
  }

  /**
   * @dtor
   *
   * This destructor reassigns the old value to `ref` that was passed to the constructor.
   */
  ~ValueGuard() { ref_ = oldValue_; }

private:

  T& ref_; // NOLINT
  T oldValue_;
};

/**
 * Makes a #rocket::ValueGuard instance implicitly.
 *
 * @param ref a reference to the variable that is to be assigned
 * @param newValue the new value to assign immediately
 */
#define ROCKET_VALUE_GUARD(ref, newValue) const ::rocket::ValueGuard ROCKET_ID()(ref, newValue)

} // namespace rocket

// EOF
