/**
 * @file scoped.h
 *
 * Managed objects, utilizing C++ constructors and destructors.
 */

#pragma once

#include "macro.h"

#include <functional>

namespace rocket::scoped {

// 'Scoped' -------------------------------------------------------------------------------------------------

/**
 * An object that executes a function in its destructor, i\. e.\ when it goes out of scope.
 *
 * Use the #ROCKET_SCOPED macro for your convenience.
 */
struct Scoped {
  /**
   * @ctor
   *
   * @param f the function to execute upon scope exit
   */
  inline explicit Scoped(std::function<void()>&& f) : f_(std::move(f)) {}

  /**
   * @dtor
   *
   * This destructor executes the function @c f that was passed to the constructor.
   */
  inline ~Scoped() noexcept { f_(); }

private:

  std::function<void()> f_;
};

/**
 * Makes a #rocket::scoped::Scoped instance implicitly.
 *
 * @param f the function to execute upon scope exit
 */
#define ROCKET_SCOPED(f) ::rocket::scoped::Scoped ROCKET_ID(f)

// 'ScopedValue' --------------------------------------------------------------------------------------------

/**
 * An object that immediately assigns a new value to a variable and restores the old value in its destructor,
 * i.\ e.\ when it goes out of scope.
 *
 * Use the #ROCKET_SCOPED_VALUE macro for your convenience.
 *
 * @tparam T the type of the value
 */
template<typename T>
struct ScopedValue {
  /**
   * @ctor
   *
   * @param ref a reference to the variable that is to be assigned
   * @param newValue the new value to assign immediately
   */
  inline ScopedValue(T& ref, T&& newValue) :
      ref_(ref),
      oldValue_(ref) {
    ref = std::forward<T>(newValue);
  }

  /**
   * @dtor
   *
   * This destructor reassigns the old value to @c ref that was passed to the constructor.
   */
  inline ~ScopedValue() noexcept { ref_ = oldValue_; }

private:

  T& ref_;
  T oldValue_;
};

/**
 * Makes a #rocket::scoped::ScopedValue instance implicitly.
 *
 * @param ref a reference to the variable that is to be assigned
 * @param newValue the new value to assign immediately
 */
#define ROCKET_SCOPED_VALUE(ref, newValue) ::rocket::scoped::ScopedValue ROCKET_ID(ref, newValue)

} // namespace rocket::scoped

// EOF
