/**
 * @file Guard.h
 *
 * Guards, utilizing C++ constructors and destructors.
 */

#pragma once

#include "macro.h"

#include <functional>

namespace rocket {

// `Guard` --------------------------------------------------------------------------------------------------

/**
 * An object that executes a function in its destructor, i.e. when it goes out of scope.
 *
 * Use the #ROCKET_GUARD macro for your convenience.
 */
struct Guard {
  /**
   * @ctor
   *
   * @param f the function to execute upon scope exit
   */
  inline explicit Guard(std::function<void()>&& f) : f_(std::move(f)) {}

  /**
   * @dtor
   *
   * This destructor executes the function `f` that was passed to the constructor.
   */
  inline ~Guard() noexcept { f_(); }

private:

  std::function<void()> f_;
};

/**
 * Makes a #rocket::Guard instance implicitly.
 *
 * @param f the function to execute upon scope exit
 */
#define ROCKET_GUARD(f) ::rocket::Guard ROCKET_ID(f)

// `ValueGuard` ---------------------------------------------------------------------------------------------

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
  inline ValueGuard(T& ref, T&& newValue) :
      ref_(ref),
      oldValue_(ref) {
    ref = std::forward<T>(newValue);
  }

  /**
   * @dtor
   *
   * This destructor reassigns the old value to `ref` that was passed to the constructor.
   */
  inline ~ValueGuard() noexcept { ref_ = oldValue_; }

private:

  T& ref_;
  T oldValue_;
};

/**
 * Makes a #rocket::ValueGuard instance implicitly.
 *
 * @param ref a reference to the variable that is to be assigned
 * @param newValue the new value to assign immediately
 */
#define ROCKET_VALUE_GUARD(ref, newValue) ::rocket::ValueGuard ROCKET_ID(ref, newValue)

} // namespace rocket

// EOF
