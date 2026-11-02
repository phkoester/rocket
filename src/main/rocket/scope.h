/**
 * @file scope.h
 *
 * Scope guards.
 */

#pragma once

#include "rocket/macro.h"
#include "rocket/rocket.h"

#include <exception>
#include <functional>

namespace rocket {

// `ScopeExit` ----------------------------------------------------------------------------------------------

/**
 * An object that executes a function in its destructor.
 *
 * Use the #ROCKET_SCOPE_EXIT macro for your convenience.
 *
 * @NotThreadSafe
 */
struct ScopeExit {
  explicit ScopeExit(std::function<void()>&& fn) : fn_(std::move(fn)) {}

  ScopeExit(const ScopeExit&) = delete;

  ~ScopeExit() {
    if (fn_) {
      fn_();
    }
  }

  ScopeExit& operator=(const ScopeExit&) = delete;

  void release() { fn_= {}; }

private:

  std::function<void()> fn_;
};

/// Makes a #rocket::ScopeExit instance implicitly.
#define ROCKET_SCOPE_EXIT(fn) ::rocket::ScopeExit ROCKET_ID()(fn)

// `ScopeFail` ----------------------------------------------------------------------------------------------

/**
 * An object that executes a function in its destructor in the face of an exception.
 *
 * Use the #ROCKET_SCOPE_FAIL macro for your convenience.
 *
 * @NotThreadSafe
 */
 struct ScopeFail {
  explicit ScopeFail(std::function<void()>&& fn) :
    uncaughtExceptions_(std::uncaught_exceptions()),
    fn_(std::move(fn))
  {}

  ScopeFail(const ScopeFail&) = delete;

  ~ScopeFail() {
    if (fn_ && std::uncaught_exceptions() > uncaughtExceptions_) {
      fn_();
    }
  }

  ScopeFail& operator=(const ScopeFail&) = delete;

  void release() { fn_ = {}; }

private:

  i32 uncaughtExceptions_;
  std::function<void()> fn_;
};

/// Makes a #rocket::ScopeFail instance implicitly.
#define ROCKET_SCOPE_FAIL(fn) ::rocket::ScopeFail ROCKET_ID()(fn)

// `ScopeSuccess` ----------------------------------------------------------------------------------------------

/**
 * An object that executes a function in its destructor in the face of a successful execution.
 *
 * Use the #ROCKET_SCOPE_SUCCESS macro for your convenience.
 *
 * @NotThreadSafe
 */
struct ScopeSuccess {
  explicit ScopeSuccess(std::function<void()>&& fn) :
    uncaughtExceptions_(std::uncaught_exceptions()),
    fn_(std::move(fn))
  {}

  ScopeSuccess(const ScopeSuccess&) = delete;

  ~ScopeSuccess() {
    if (fn_ && std::uncaught_exceptions() <= uncaughtExceptions_) {
      fn_();
    }
  }

  ScopeSuccess& operator=(const ScopeSuccess&) = delete;

  void release() { fn_ = {}; }

private:

  i32 uncaughtExceptions_;
  std::function<void()> fn_;
};

/// Makes a #rocket::ScopeSuccess instance implicitly.
#define ROCKET_SCOPE_SUCCESS(fn) ::rocket::ScopeSuccess ROCKET_ID()(fn)

} // namespace rocket

// EOF
