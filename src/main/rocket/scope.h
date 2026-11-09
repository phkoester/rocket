/**
 * @file scope.h
 *
 * Scope guards: #rocket::scope_exit, #rocket::scope_success, and #rocket::scope_fail.
 *
 * If the standard library provides `std::experimental::scope_exit` and friends (Library Fundamentals TS v3,
 * header `<experimental/scope>`), these are used. Otherwise, a portable implementation following the same
 * specification is provided.
 *
 * - #rocket::scope_exit calls its exit function whenever the scope is left.
 * - #rocket::scope_success calls its exit function only if the scope is left normally.
 * - #rocket::scope_fail calls its exit function only if the scope is left by an exception.
 *
 * Calling `release()` on a guard prevents its exit function from being called.
 */

#pragma once

#include <version>

#if defined(ROCKET_OS_MACOS) || defined(ROCKET_OS_WINDOWS)
  #if __has_include(<experimental/scope>)
    #error This C++ version supports experimental scope guards
  #endif
#else
  #if not __has_include(<experimental/scope>)
    #error This C++ version does not support experimental scope guards
  #endif
  /// Defined if the standard library provides `<experimental/scope>`.
  #define ROCKET_HAS_STD_EXPERIMENTAL_SCOPE
#endif

#include "rocket/macro.h"

#ifdef ROCKET_HAS_STD_EXPERIMENTAL_SCOPE
  #include <experimental/scope>
#else
  #include <concepts>
  #include <exception>
  #include <limits>
  #include <type_traits>
  #include <utility>
#endif

namespace rocket {

#ifdef ROCKET_HAS_STD_EXPERIMENTAL_SCOPE

// `scope_exit`, `scope_fail`, and `scope_success` ----------------------------------------------------------

/// A scope guard that calls its exit function when the scope is left, regardless of how.
template<typename EF>
using scope_exit = std::experimental::scope_exit<EF>;

/// A scope guard that calls its exit function only if the scope is left by an exception.
template<typename EF>
using scope_fail = std::experimental::scope_fail<EF>;

/// A scope guard that calls its exit function only if the scope is left normally, i.e., not by an exception.
template<typename EF>
using scope_success = std::experimental::scope_success<EF>;

#else

// Macros ---------------------------------------------------------------------------------------------------

/// @cond undocumented
#ifdef ROCKET_CXX_COMPILER_MSVC
  #define ROCKET_SCOPE_NO_UNIQUE_ADDRESS__ [[msvc::no_unique_address]]
#else
  #define ROCKET_SCOPE_NO_UNIQUE_ADDRESS__ [[no_unique_address]]
#endif
/// @endcond

// Internal -------------------------------------------------------------------------------------------------

namespace internal::scope {

template<typename T>
concept NotLvalueRef = not std::is_lvalue_reference_v<T>;

template<typename T, typename U>
concept NotSameAs = not std::same_as<T, U>;

} // namespace internal::scope

// `scope_exit` ---------------------------------------------------------------------------------------------

/**
 * A scope guard that calls its exit function when the scope is left, regardless of how.
 *
 * @NotThreadSafe
 */
template<typename EF>
struct [[nodiscard]] scope_exit {
  /**
   * Constructs from an lvalue, or from an rvalue that cannot be moved without throwing. If copying the
   * function throws, @p fn is called before the exception propagates.
   *
   * @param fn the exit function
   */
  template<typename EFP> requires
    internal::scope::NotSameAs<std::remove_cvref_t<EFP>, scope_exit> && std::constructible_from<EF, EFP>
  explicit scope_exit(EFP&& fn) noexcept(std::is_nothrow_constructible_v<EF, EFP&>)
  try : exitFn_(fn)
  {}
  catch (...) {
    fn();
  }

  /**
   * Constructs from an rvalue that can be moved without throwing.
   *
   * @param fn the exit function
   */
  template<typename EFP> requires
    internal::scope::NotSameAs<std::remove_cvref_t<EFP>, scope_exit> &&
    std::constructible_from<EF, EFP> &&
    internal::scope::NotLvalueRef<EFP> &&
    std::is_nothrow_constructible_v<EF, EFP>
  explicit scope_exit(EFP&& fn) noexcept : exitFn_(std::forward<EFP>(fn)) {}

  /**
   * Constructs by moving the exit function of @p rhs, which is then released.
   *
   * @param rhs the guard to move from
   */
  scope_exit(scope_exit&& rhs) noexcept requires std::is_nothrow_move_constructible_v<EF> :
    exitFn_(std::forward<EF>(rhs.exitFn_)),
    executeOnDestruction_(rhs.executeOnDestruction_) {
    rhs.release();
  }

  /**
   * Constructs by copying the exit function of @p rhs, as it cannot be moved without throwing. @p rhs is
   * then released.
   *
   * @param rhs the guard to move from
   */
  scope_exit(scope_exit&& rhs) noexcept(std::is_nothrow_copy_constructible_v<EF>)
    requires (not std::is_nothrow_move_constructible_v<EF>) && std::is_copy_constructible_v<EF>
    : exitFn_(rhs.exitFn_),
      executeOnDestruction_(rhs.executeOnDestruction_) {
    rhs.release();
  }

  scope_exit(const scope_exit&) = delete;

  ~scope_exit() noexcept {
    if (executeOnDestruction_) {
      exitFn_();
    }
  }

  scope_exit& operator=(const scope_exit&) = delete;

  scope_exit& operator=(scope_exit&&) = delete;

  /// Prevents the exit function from being called.
  void release() noexcept { executeOnDestruction_ = false; }

private:

  ROCKET_SCOPE_NO_UNIQUE_ADDRESS__ EF exitFn_;
  bool executeOnDestruction_ = true;
};

/// @ctad_guide{#rocket::scope_exit}
template<typename EF>
scope_exit(EF) -> scope_exit<EF>;

// `scope_fail` ---------------------------------------------------------------------------------------------

/**
 * A scope guard that calls its exit function only if the scope is left by an exception.
 *
 * The guard compares `std::uncaught_exceptions()` at construction and destruction time, so it also works
 * correctly if it is created during stack unwinding.
 *
 * @NotThreadSafe
 */
template<typename EF>
struct [[nodiscard]] scope_fail {
  /**
   * Constructs from an lvalue, or from an rvalue that cannot be moved without throwing. If copying the
   * function throws, @p fn is called before the exception propagates.
   *
   * @param fn the exit function
   */
  template<typename EFP>
    requires internal::scope::NotSameAs<std::remove_cvref_t<EFP>, scope_fail>
          && std::constructible_from<EF, EFP>
  explicit scope_fail(EFP&& fn) noexcept(std::is_nothrow_constructible_v<EF, EFP&>)
  try
    : exitFn_(fn)
  {}
  catch (...) {
    fn();
  }

  /**
   * Constructs from an rvalue that can be moved without throwing.
   *
   * @param fn the exit function
   */
  template<typename EFP> requires
    internal::scope::NotSameAs<std::remove_cvref_t<EFP>, scope_fail> &&
    std::constructible_from<EF, EFP> &&
    internal::scope::NotLvalueRef<EFP> && std::is_nothrow_constructible_v<EF, EFP>
  explicit scope_fail(EFP&& fn) noexcept : exitFn_(std::forward<EFP>(fn)) {}

  /**
   * Constructs by moving the exit function of @p rhs, which is then released.
   *
   * @param rhs the guard to move from
   */
  scope_fail(scope_fail&& rhs) noexcept requires std::is_nothrow_move_constructible_v<EF> :
    exitFn_(std::forward<EF>(rhs.exitFn_)),
    uncaughtInit_(rhs.uncaughtInit_) {
    rhs.release();
  }

  /**
   * Constructs by copying the exit function of @p rhs, as it cannot be moved without throwing. @p rhs is
   * then released.
   *
   * @param rhs the guard to move from
   */
  scope_fail(scope_fail&& rhs) noexcept(std::is_nothrow_copy_constructible_v<EF>) requires
    (not std::is_nothrow_move_constructible_v<EF>) && std::is_copy_constructible_v<EF> :
    exitFn_(rhs.exitFn_),
    uncaughtInit_(rhs.uncaughtInit_) {
    rhs.release();
  }

  scope_fail(const scope_fail&) = delete;

  ~scope_fail() noexcept {
    if (std::uncaught_exceptions() > uncaughtInit_) {
      exitFn_();
    }
  }

  scope_fail& operator=(const scope_fail&) = delete;

  scope_fail& operator=(scope_fail&&) = delete;

  /// Prevents the exit function from being called.
  void release() noexcept { uncaughtInit_ = std::numeric_limits<int>::max(); }

private:

  ROCKET_SCOPE_NO_UNIQUE_ADDRESS__ EF exitFn_;
  int uncaughtInit_ = std::uncaught_exceptions();
};

/// @ctad_guide{#rocket::scope_fail}
template<typename EF>
scope_fail(EF) -> scope_fail<EF>;

// `scope_success` ------------------------------------------------------------------------------------------

/**
 * A scope guard that calls its exit function only if the scope is left normally, i.e., not by an
 * exception.
 *
 * The guard compares `std::uncaught_exceptions()` at construction and destruction time, so it also works
 * correctly if it is created during stack unwinding.
 *
 * Unlike the other guards, the exit function may throw.
 *
 * @NotThreadSafe
 */
template<typename EF>
struct [[nodiscard]] scope_success {
  /**
   * Constructs from an lvalue, or from an rvalue that cannot be moved without throwing. Unlike the other
   * guards, @p fn is not called if copying it throws.
   *
   * @param fn the exit function
   */
  template<typename EFP> requires
    internal::scope::NotSameAs<std::remove_cvref_t<EFP>, scope_success> &&
    std::constructible_from<EF, EFP>
  explicit scope_success(EFP&& fn) noexcept(std::is_nothrow_constructible_v<EF, EFP&>) : exitFn_(fn) {}

  /**
   * Constructs from an rvalue that can be moved without throwing.
   *
   * @param fn the exit function
   */
  template<typename EFP> requires
    internal::scope::NotSameAs<std::remove_cvref_t<EFP>, scope_success> &&
    std::constructible_from<EF, EFP> &&
    internal::scope::NotLvalueRef<EFP> &&
    std::is_nothrow_constructible_v<EF, EFP>
  explicit scope_success(EFP&& fn) noexcept : exitFn_(std::forward<EFP>(fn)) {}

  /**
   * Constructs by moving the exit function of @p rhs, which is then released.
   *
   * @param rhs the guard to move from
   */
  scope_success(scope_success&& rhs) noexcept requires
    std::is_nothrow_move_constructible_v<EF> :
    exitFn_(std::forward<EF>(rhs.exitFn_)),
    uncaughtInit_(rhs.uncaughtInit_) {
    rhs.release();
  }

  /**
   * Constructs by copying the exit function of @p rhs, as it cannot be moved without throwing. @p rhs is
   * then released.
   *
   * @param rhs the guard to move from
   */
  scope_success(scope_success&& rhs) noexcept(std::is_nothrow_copy_constructible_v<EF>) requires
    (not std::is_nothrow_move_constructible_v<EF>) &&
    std::is_copy_constructible_v<EF> :
    exitFn_(rhs.exitFn_),
    uncaughtInit_(rhs.uncaughtInit_) {
    rhs.release();
  }

  scope_success(const scope_success&) = delete;

  ~scope_success() noexcept(noexcept(std::declval<EF&>()())) {
    if (std::uncaught_exceptions() <= uncaughtInit_) {
      exitFn_();
    }
  }

  scope_success& operator=(const scope_success&) = delete;

  scope_success& operator=(scope_success&&) = delete;

  /// Prevents the exit function from being called.
  void release() noexcept { uncaughtInit_ = -std::numeric_limits<int>::max(); }

private:

  ROCKET_SCOPE_NO_UNIQUE_ADDRESS__ EF exitFn_;
  int uncaughtInit_ = std::uncaught_exceptions();
};

/// @ctad_guide{#rocket::scope_success}
template<typename EF>
scope_success(EF) -> scope_success<EF>;

#undef ROCKET_SCOPE_NO_UNIQUE_ADDRESS__

#endif // ROCKET_HAS_STD_EXPERIMENTAL_SCOPE

} // namespace rocket

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Makes an anonymous #rocket::scope_exit guard that calls @p fn whenever the current scope is left.
 */
#define ROCKET_SCOPE_EXIT(fn) const ::rocket::scope_exit ROCKET_ID()(fn)

/**
 * Makes an anonymous #rocket::scope_fail guard that calls @p fn if the current scope is left by an
 * exception.
 */
#define ROCKET_SCOPE_FAIL(fn) const ::rocket::scope_fail ROCKET_ID()(fn)

/**
 * Makes an anonymous #rocket::scope_success guard that calls @p fn if the current scope is left normally.
 */
#define ROCKET_SCOPE_SUCCESS(fn) const ::rocket::scope_success ROCKET_ID()(fn)

// EOF
