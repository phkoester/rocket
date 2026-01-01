/**
 * @file assert.h
 *
 * Assertions, argument checks, and expectations.
 *
 * This file may be included several times. If `NDEBUG` is not defined, the assertion macros are active.
 * Otherwise, they expand to a call of #rocket::nop.
 */

// No `#pragma once` here!

#ifndef ROCKET_ASSERT_H
#define ROCKET_ASSERT_H

#include "rocket/Exception.h"
#include "rocket/Process.h"
#ifdef NDEBUG
#include "rocket/rocket.h" // `rocket::nop()`
#endif
#include "rocket/format/format.h"

#include <boost/preprocessor/stringize.hpp>

#include <source_location>

// Internal -------------------------------------------------------------------------------------------------

namespace rocket::assert::internal {

template<typename... T>
[[noreturn]] void
onAssertFailed(
    const std::source_location& sl,
    const char* expr,
    fmt::format_string<T...> fmt = "",
    T&&... args) {
  process.error(
      nio::stderr,
      EXIT_SUCCESS,
      "{}:{}: Assertion `{}` failed{}", sl.file_name(), sl.line(), expr, format::Format([&] {
    if (fmt.get().size() > 0) {
      auto params = format::Format::params(": \\@0");
      params.tag("\\@0", fmt, std::forward<T>(args)...);
      return params;
    } else {
      return format::Format::params();
    }
  }));
  std::terminate();
}

template<typename... T>
[[noreturn]] void
onCheckFailed(
    const std::source_location& sl,
    const char* name,
    const char* expr,
    fmt::format_string<T...> fmt = "",
    T&&... args) {
  throw InvalidArgument(name, fmt::format("Check `{}` failed{}", expr, format::Format([&] {
    if (fmt.get().size() > 0) {
      auto params = format::Format::params(": \\@0");
      params.tag("\\@0", fmt, std::forward<T>(args)...);
      return params;
    } else {
      return format::Format::params();
    }
  })), sl);
}

template<typename... T>
[[noreturn]] void onExpectFailed(
    const std::source_location& sl,
    const char* expr,
    fmt::format_string<T...> fmt = "",
    T&&... args) {
  throw InvalidState(fmt::format("Expectation `{}` failed{}", expr, format::Format([&] {
    if (fmt.get().size() > 0) {
      auto params = format::Format::params(": \\@0");
      params.tag("\\@0", fmt, std::forward<T>(args)...);
      return params;
    } else {
      return format::Format::params();
    }
  })), sl);
}

} // namespace rocket::assert::internal

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Terminates.
 */
#define ROCKET_TERMINATE_INVALID_CALL() ROCKET_ASSERT(false, "Invalid call of function `{}`", __PRETTY_FUNCTION__)

/**
 * Terminates.
 */
#define ROCKET_TERMINATE_NOT_IMPLEMENTED() ROCKET_ASSERT(false, "Not implemented")

/**
 * Terminates.
 */
#define ROCKET_TERMINATE_UNREACHABLE_CODE() ROCKET_ASSERT(false, "Unreachable code")

/**
 * Throws #rocket::InvalidState.
 *
 * @throw #rocket::InvalidState
 */
#define ROCKET_FAIL_INVALID_CALL() ROCKET_EXPECT(false, "Invalid call of function `{}`", __PRETTY_FUNCTION__)

/**
 * Throws #rocket::InvalidState.
 *
 * @throw #rocket::InvalidState
 */
#define ROCKET_FAIL_NOT_IMPLEMENTED() ROCKET_EXPECT(false, "Not implemented")

/**
 * Throws #rocket::InvalidState.
 *
 * @throw #rocket::InvalidState
 */
#define ROCKET_FAIL_UNREACHABLE_CODE() ROCKET_EXPECT(false, "Unreachable code")

#endif // ROCKET_ASSERT_H

// End of header guard --------------------------------------------------------------------------------------

#undef ROCKET_ASSERT
#undef ROCKET_CHECK
#undef ROCKET_EXPECT

#ifdef NDEBUG

#define ROCKET_ASSERT(expr, ...) ::rocket::nop()
#define ROCKET_CHECK(name, expr, ...) ::rocket::nop()
#define ROCKET_EXPECT(expr, ...) ::rocket::nop()

#else

/**
 * Terminates if @p expr evaluates to `false`.
 *
 * Usage: `ROCKET_ASSERT(expr, [fmt, [args]...])`
 *
 * Use this macro only in order to handle program states that result from a flawed implementation and make
 * further execution impossible or dangerous. Do not abuse it to catch states that may reasonably occur in
 * normal program execution. If an assertion fails, code needs to be fixed.
 */
#define ROCKET_ASSERT(expr, ...) \
    if (not (expr)) { \
      ::rocket::assert::internal::onAssertFailed( \
          ROCKET_EXCEPT_SL, \
          BOOST_PP_STRINGIZE(expr) \
          ROCKET_COMMA_AND_VA_ARGS(__VA_ARGS__)); \
    }

/**
 * Throws #rocket::InvalidArgument if @p expr evaluates to `false`.
 *
 * Usage: `ROCKET_CHECK(name, expr, [fmt, [args]...])`
 *
 * @throw #rocket::InvalidArgument if @p expr evaluates to `false`
 *
 * Use this macro only in order to check function arguments. The first parameter of this macro is always the
 * name of the function parameter the argument of which is to be checked.
 */
#define ROCKET_CHECK(name, expr, ...) \
    if (not (expr)) { \
      ::rocket::assert::internal::onCheckFailed( \
          ROCKET_EXCEPT_SL, \
          BOOST_PP_STRINGIZE(name), \
          BOOST_PP_STRINGIZE(expr) \
          ROCKET_COMMA_AND_VA_ARGS(__VA_ARGS__)); \
    }

/**
 * Throws #rocket::InvalidState if @p expr evaluates to `false`.
 *
 * Usage: `ROCKET_EXPECT(expr, [fmt, [args]...])`
 *
 * @throw #rocket::InvalidState if @p expr evaluates to `false`
 *
 * Use this macro only in order to handle program states that result from a flawed implementation but may
 * be dealt with by throwing an exception. Do not abuse it to catch states that may reasonably occur in
 * normal program execution. If an expectation fails, code needs to be fixed.
 */
#define ROCKET_EXPECT(expr, ...) \
    if (not (expr)) { \
      ::rocket::assert::internal::onExpectFailed( \
          ROCKET_EXCEPT_SL, \
          BOOST_PP_STRINGIZE(expr) \
          ROCKET_COMMA_AND_VA_ARGS(__VA_ARGS__)); \
    }

#endif // NDEBUG

// EOF
