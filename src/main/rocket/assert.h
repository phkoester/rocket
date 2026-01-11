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
assertFailed(
    std::source_location&& sl,
    const char* expr,
    fmt::format_string<T...> fmt = "",
    T&&... args) {
  process.error(
      nio::stderr,
      EXIT_SUCCESS,
      "{}:{}: Assertion `{}` failed{}", sl.file_name(), sl.line(), expr, format::Format<char>([&] {
    if (fmt.get().size() > 0) {
      auto params = format::Format<char>::params(": \\@0");
      params.tag("\\@0", fmt, std::forward<T>(args)...);
      return params;
    } else {
      return format::Format<char>::params();
    }
  }));
  std::terminate();
}

template<typename... T>
[[noreturn]] void
checkFailed(
    std::source_location&& sl,
    const char* name,
    const char* expr,
    fmt::format_string<T...> fmt = "",
    T&&... args) {
  throw InvalidArgument(name, fmt::format("Check `{}` failed{}", expr, format::Format<char>([&] {
    if (fmt.get().size() > 0) {
      auto params = format::Format<char>::params(": \\@0");
      params.tag("\\@0", fmt, std::forward<T>(args)...);
      return params;
    } else {
      return format::Format<char>::params();
    }
  })), sl);
}

template<typename... T>
[[noreturn]] void
expectFailed(
    std::source_location&& sl,
    const char* expr,
    fmt::format_string<T...> fmt = "",
    T&&... args) {
  throw InvalidState(fmt::format("Expectation `{}` failed{}", expr, format::Format<char>([&] {
    if (fmt.get().size() > 0) {
      auto params = format::Format<char>::params(": \\@0");
      params.tag("\\@0", fmt, std::forward<T>(args)...);
      return params;
    } else {
      return format::Format<char>::params();
    }
  })), sl);
}

template<typename... T>
[[noreturn]] void
fail(
    std::source_location&& sl,
    fmt::format_string<T...> fmt,
    T&&... args) {
  throw InvalidState(fmt::format(fmt, std::forward<T>(args)...), sl);
}

template<typename... T>
[[noreturn]] void
terminate(
    std::source_location&& sl,
    fmt::format_string<T...> fmt,
    T&&... args) {
  process.error(
      nio::stderr,
      EXIT_SUCCESS,
      fmt,
      std::forward<T>(args)...);
  std::terminate();
}

} // namespace rocket::assert::internal

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Throws #rocket::InvalidState.
 *
 * @throw #rocket::InvalidState
 */
#define ROCKET_FAIL_INVALID_CALL() ROCKET_FAIL("Invalid call of function `{}`", __PRETTY_FUNCTION__)

/**
 * Throws #rocket::InvalidState.
 *
 * @throw #rocket::InvalidState
 */
#define ROCKET_FAIL_NOT_IMPLEMENTED() ROCKET_FAIL("Not implemented")

/**
 * Throws #rocket::InvalidState.
 *
 * @throw #rocket::InvalidState
 */
#define ROCKET_FAIL_UNREACHABLE_CODE() ROCKET_FAIL("Unreachable code")

/**
 * Terminates because of an invalid function call.
 */
#define ROCKET_TERMINATE_INVALID_CALL() ROCKET_TERMINATE("Invalid call of function `{}`", __PRETTY_FUNCTION__)

/**
 * Terminates because of a missing implementation.
 */
#define ROCKET_TERMINATE_NOT_IMPLEMENTED() ROCKET_TERMINATE("Not implemented")

/**
 * Terminates because code was reached that was supposed to be unreachable.
 */
#define ROCKET_TERMINATE_UNREACHABLE_CODE() ROCKET_TERMINATE("Unreachable code")

#endif // ROCKET_ASSERT_H

// End of header guard --------------------------------------------------------------------------------------

#undef ROCKET_ASSERT
#undef ROCKET_CHECK
#undef ROCKET_EXPECT
#undef ROCKET_FAIL
#undef ROCKET_TERMINATE

#ifdef NDEBUG

#define ROCKET_ASSERT(expr, ...) ::rocket::nop()
#define ROCKET_CHECK(name, expr, ...) ::rocket::nop()
#define ROCKET_EXPECT(expr, ...) ::rocket::nop()
#define ROCKET_FAIL(fmt, ...) ::rocket::nop()
#define ROCKET_TERMINATL(fmt, ...) ::rocket::nop()

#else

/**
 * Terminates if @p expr evaluates to `false`.
 *
 * Usage: `ROCKET_ASSERT(expr, [fmt, [args]...])`
 *
 * Use this macro in order to handle invalid program states that make further execution impossible or
 * dangerous. If an assertion fails, possibly code needs to be fixed.
 */
#define ROCKET_ASSERT(expr, ...) \
    if (not (expr)) { \
      ::rocket::assert::internal::assertFailed( \
          ROCKET_EXCEPTION_SL, \
          BOOST_PP_STRINGIZE(expr) \
          __VA_OPT__(,) __VA_ARGS__); \
    }

/**
 * Throws #rocket::InvalidArgument if @p expr evaluates to `false`.
 *
 * Usage: `ROCKET_CHECK(name, expr, [fmt, [args]...])`
 *
 * @throw #rocket::InvalidArgument if @p expr evaluates to `false`
 *
 * Use this macro in order to check function arguments. The first parameter @p name is always the name of the
 * function parameter the argument of which is to be checked.
 */
#define ROCKET_CHECK(name, expr, ...) \
    if (not (expr)) { \
      ::rocket::assert::internal::checkFailed( \
          ROCKET_EXCEPTION_SL, \
          BOOST_PP_STRINGIZE(name), \
          BOOST_PP_STRINGIZE(expr) \
          __VA_OPT__(,) __VA_ARGS__); \
    }

/**
 * Throws #rocket::InvalidState if @p expr evaluates to `false`.
 *
 * Usage: `ROCKET_EXPECT(expr, [fmt, [args]...])`
 *
 * @throw #rocket::InvalidState if @p expr evaluates to `false`
 *
 * Use this macro in order to handle invalid program states that may be dealt with by throwing an exception.
 */
#define ROCKET_EXPECT(expr, ...) \
    if (not (expr)) { \
      ::rocket::assert::internal::expectFailed( \
          ROCKET_EXCEPTION_SL, \
          BOOST_PP_STRINGIZE(expr) \
          __VA_OPT__(,) __VA_ARGS__); \
    }

/**
 * Throws #rocket::InvalidState.
 *
 * Usage: `ROCKET_FAIL(fmt, [args]...)`
 *
 * @throw #rocket::InvalidState
 *
 * Use this macro in order to handle invalid program states that may be dealt with by throwing an exception.
 */
#define ROCKET_FAIL(fmt, ...) \
    ::rocket::assert::internal::fail( \
        ROCKET_EXCEPTION_SL, \
        fmt \
        __VA_OPT__(,) __VA_ARGS__)

/**
 * Terminates.
 *
 * Usage: `ROCKET_TERMINATE(fmt, [args]...)`
 *
 * Use this macro in order to handle invalid program states that make further execution impossible or
 * dangerous.
 */
#define ROCKET_TERMINATE(fmt, ...) \
    ::rocket::assert::internal::terminate( \
        ROCKET_EXCEPTION_SL, \
        fmt \
        __VA_OPT__(,) __VA_ARGS__)

#endif // NDEBUG

// EOF
