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

#include "Process.h"
#include "basic.h" // `rocket::nop()`
#include "except.h"
#include "nio.h"

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/facilities/check_empty.hpp>
#include <boost/preprocessor/logical/not.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

#include <fmt/format.h>

#include <source_location>
#include <string>

// Internal -------------------------------------------------------------------------------------------------

namespace rocket::assert::internal {

template<typename... T>
[[noreturn]] void
onAssertFailed(
    const std::source_location& sl,
    const char* expr,
    fmt::format_string<T...> fmt = "",
    T&&... args) {
  std::string msg;
  if (fmt.get().size() > 0) {
    msg = ": ";
    nio::StringSink sink(msg);
    sink.print(fmt, std::forward<T>(args)...);
  }
  auto stderr = nio::stderr();
  process.error(stderr, EXIT_SUCCESS, "{}:{}: Assertion `{}` failed{}", sl.file_name(), sl.line(), expr, msg);
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
  std::string msg;
  if (fmt.get().size() > 0) {
    msg = ": ";
    nio::StringSink sink(msg);
    sink.print(fmt, std::forward<T>(args)...);
  }
  except::throwInvalidArgument(sl, name, "Check `{}` failed{}", expr, msg);
}

template<typename... T>
[[noreturn]] void onExpectFailed(
    const std::source_location&sl,
    const char* expr,
    fmt::format_string<T...> fmt = "",
    T&&... args) {
  std::string msg;
  if (fmt.get().size() > 0) {
    msg = ": ";
    nio::StringSink sink(msg);
    sink.print(fmt, std::forward<T>(args)...);
  }
  except::throwInvalidState(sl, "Expectation `{}` failed{}", expr, msg);
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
 * Throws #rocket::except::InvalidState.
 *
 * @throw #rocket::except::InvalidState
 */
#define ROCKET_FAIL_INVALID_CALL() ROCKET_EXPECT(false, "Invalid call of function `{}`", __PRETTY_FUNCTION__)

/**
 * Throws #rocket::except::InvalidState.
 *
 * @throw #rocket::except::InvalidState
 */
#define ROCKET_FAIL_NOT_IMPLEMENTED ROCKET_EXPECT(false, "Not implemented")

/**
 * Throws #rocket::except::InvalidState.
 *
 * @throw #rocket::except::InvalidState
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
          ::std::source_location::current(), \
          BOOST_PP_STRINGIZE(expr) \
          BOOST_PP_COMMA_IF(BOOST_PP_NOT(BOOST_PP_CHECK_EMPTY(BOOST_PP_TUPLE_ELEM(0, (__VA_ARGS__))))) \
          __VA_ARGS__); \
    }

/**
 * Throws #rocket::except::InvalidArgument if @p expr evaluates to `false`.
 *
 * Usage: `ROCKET_CHECK(name, expr, [fmt, [args]...])`
 *
 * @throw #rocket::except::InvalidArgument if @p expr evaluates to `false`
 *
 * Use this macro only in order to check function arguments. The first parameter of this macro is always the
 * name of the function parameter the argument of which is to be checked.
 */
#define ROCKET_CHECK(name, expr, ...) \
    if (not (expr)) { \
      ::rocket::assert::internal::onCheckFailed( \
          ::std::source_location::current(), \
          BOOST_PP_STRINGIZE(name), \
          BOOST_PP_STRINGIZE(expr) \
          BOOST_PP_COMMA_IF(BOOST_PP_NOT(BOOST_PP_CHECK_EMPTY(BOOST_PP_TUPLE_ELEM(0, (__VA_ARGS__))))) \
          __VA_ARGS__); \
    }

/**
 * Throws #rocket::except::InvalidState if @p expr evaluates to `false`.
 *
 * Usage: `ROCKET_EXPECT(expr, [fmt, [args]...])`
 *
 * @throw #rocket::except::InvalidState if @p expr evaluates to `false`
 *
 * Use this macro only in order to handle program states that result from a flawed implementation but may
 * be dealt with by throwing an exception. Do not abuse it to catch states that may reasonably occur in
 * normal program execution. If an expectation fails, code needs to be fixed.
 */
#define ROCKET_EXPECT(expr, ...) \
    if (not (expr)) { \
      ::rocket::assert::internal::onExpectFailed( \
          ::std::source_location::current(), \
          BOOST_PP_STRINGIZE(expr) \
          BOOST_PP_COMMA_IF(BOOST_PP_NOT(BOOST_PP_CHECK_EMPTY(BOOST_PP_TUPLE_ELEM(0, (__VA_ARGS__))))) \
          __VA_ARGS__); \
    }

#endif // NDEBUG

// EOF
