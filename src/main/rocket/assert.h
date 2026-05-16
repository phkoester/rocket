/**
 * @file assert.h
 *
 * Assertions, argument checks, and expectations.
 */

# pragma once

#include "rocket/Exception.h"
#include "rocket/macro.h"
#include "rocket/Process.h"
#include "rocket/format/Subformat.h"

#include <boost/preprocessor/stringize.hpp>

#include <source_location>

// Internal -------------------------------------------------------------------------------------------------

namespace rocket::internal {

// Assertions ...............................................................................................

template<typename... T>
[[noreturn]] void
terminate( // NOLINT(*-recursion)
  const std::source_location& sl,
  fmt::format_string<T...> fmt,
  T&&... args) {
  process.error(nio::err, 0, "{}:{}: {}", sl.file_name(), sl.line(), format::Subformat<char>([&] {
    auto params = format::Subformat<char>::params("\\\x01");
    params.tag("\\\x01", fmt, std::forward<T>(args)...);
    return params;
  }));
  std::terminate();
}

template<typename... T>
[[noreturn]] void
assertFailed( // NOLINT(*-recursion)
  const std::source_location& sl,
  const char* expr,
  fmt::format_string<T...> fmt = "",
  T&&... args) {
  terminate(sl, "Assertion `{}` failed{}", expr, format::Subformat<char>([&] {
    if (fmt.get().size() > 0) {
      auto params = format::Subformat<char>::params(": \\\x01");
      params.tag("\\\x01", fmt, std::forward<T>(args)...);
      return params;
    }
    return format::Subformat<char>::params();
  }));
}

// Argument checks ..........................................................................................

template<typename... T>
[[noreturn]] void
flop(
  const std::source_location& sl,
  const char* name,
  fmt::format_string<T...> fmt,
  T&&... args) {
  throw InvalidArgument(name, fmt::format(fmt, std::forward<T>(args)...), sl);
}

template<typename... T>
[[noreturn]] void
checkFailed(
  const std::source_location& sl,
  const char* name,
  const char* expr,
  fmt::format_string<T...> fmt = "",
  T&&... args) {
  flop(sl, name, "Check `{}` failed{}", expr, format::Subformat<char>([&] {
    if (fmt.get().size() > 0) {
      auto params = format::Subformat<char>::params(": \\\x01");
      params.tag("\\\x01", fmt, std::forward<T>(args)...);
      return params;
    }
    return format::Subformat<char>::params();
  }));
}

// Expectations .............................................................................................

template<typename... T>
[[noreturn]] void
fail(
  const std::source_location& sl,
  fmt::format_string<T...> fmt,
  T&&... args) {
  throw InvalidState(fmt::format(fmt, std::forward<T>(args)...), sl);
}

template<typename... T>
[[noreturn]] void
expectFailed(
  const std::source_location& sl,
  const char* expr,
  fmt::format_string<T...> fmt = "",
  T&&... args) {
  fail(sl, "Expectation `{}` failed{}", expr, format::Subformat<char>([&] {
    if (fmt.get().size() > 0) {
      auto params = format::Subformat<char>::params(": \\@0");
      params.tag("\\@0", fmt, std::forward<T>(args)...);
      return params;
    }
    return format::Subformat<char>::params();
  }));
}

} // namespace rocket::internal

// Macros ---------------------------------------------------------------------------------------------------

// Assertions ...............................................................................................

/**
 * Terminates with a message.
 *
 * Usage: `ROCKET_TERMINATE(fmt, [args]...)`
 */
#define ROCKET_TERMINATE(fmt, ...) \
  ::rocket::internal::terminate(ROCKET_EXCEPTION_SL, fmt __VA_OPT__(,) __VA_ARGS__)

/**
 * Terminates because of an invalid function call.
 */
#define ROCKET_TERMINATE_INVALID_CALL() \
  ROCKET_TERMINATE("Invalid call of function `{}`", ROCKET_PRETTY_FUNCTION)

/**
 * Terminates because of a missing implementation.
 */
#define ROCKET_TERMINATE_NOT_IMPLEMENTED() ROCKET_TERMINATE("Not implemented")

/**
 * Terminates because code was reached that was supposed to be unreachable.
 */
#define ROCKET_TERMINATE_UNREACHABLE_CODE() ROCKET_TERMINATE("Unreachable code")

/**
 * Terminates if @p expr evaluates to `false`.
 *
 * Usage: `ROCKET_ASSERT(expr, [fmt, [args]...])`
 */
#define ROCKET_ASSERT(expr, ...) \
if (not (expr)) { \
  ::rocket::internal::assertFailed( \
    ROCKET_EXCEPTION_SL, \
    BOOST_PP_STRINGIZE(expr) \
    __VA_OPT__(,) __VA_ARGS__); \
}

#ifdef NDEBUG
/**
 * Only in debug code, where `NDEBUG` is not defined, terminates if @p expr evaluates to `false`.
 *
 * Usage: `ROCKET_DEBUG_ASSERT(expr, [fmt, [args]...])`
 */
#define ROCKET_DEBUG_ASSERT(expr, ...)
#else
/**
 * Only in debug code, where `NDEBUG` is not defined, terminates if @p expr evaluates to `false`.
 *
 * Usage: `ROCKET_DEBUG_ASSERT(expr, [fmt, [args]...])`
 */
#define ROCKET_DEBUG_ASSERT(expr, ...) ROCKET_ASSERT(expr __VA_OPT__(,) __VA_ARGS__)
#endif // NDEBUG

// Argument checks ..........................................................................................

/**
 * Throws #rocket::InvalidArgument with a message.
 *
 * Usage: `ROCKET_FLOP(name, fmt, [args]...)`
 */
#define ROCKET_FLOP(name, fmt, ...) \
  ::rocket::internal::flop(ROCKET_EXCEPTION_SL, BOOST_PP_STRINGIZE(name), fmt __VA_OPT__(,) __VA_ARGS__)

/**
 * Throws #rocket::InvalidArgument if @p expr evaluates to `false`.
 *
 * Usage: `ROCKET_CHECK(name, expr, [fmt, [args]...])`
 */
#define ROCKET_CHECK(name, expr, ...) \
  if (not (expr)) { \
    ::rocket::internal::checkFailed( \
      ROCKET_EXCEPTION_SL, \
      BOOST_PP_STRINGIZE(name), \
      BOOST_PP_STRINGIZE(expr) \
      __VA_OPT__(,) __VA_ARGS__); \
  }

#ifdef NDEBUG
/**
 * Only in debug code, where `NDEBUG` is not defined, throws #rocket::InvalidArgument if @p expr evaluates to
 * `false`.
 *
 * Usage: `ROCKET_DEBUG_CHECK(name, expr, [fmt, [args]...])`
 */
#define ROCKET_DEBUG_CHECK(name, expr, ...)
#else
/**
 * Only in debug code, where `NDEBUG` is not defined, throws #rocket::InvalidArgument if @p expr evaluates to
 * `false`.
 *
 * Usage: `ROCKET_DEBUG_CHECK(name, expr, [fmt, [args]...])`
 */
#define ROCKET_DEBUG_CHECK(name, expr, ...) ROCKET_CHECK(name, expr __VA_OPT__(,) __VA_ARGS__)
#endif

// Expectations .............................................................................................

/**
 * Throws #rocket::InvalidState with a message.
 *
 * Usage: `ROCKET_FAIL(fmt, [args]...)`
 */
#define ROCKET_FAIL(fmt, ...) ::rocket::internal::fail(ROCKET_EXCEPTION_SL, fmt __VA_OPT__(,) __VA_ARGS__)

#ifdef NDEBUG
/**
 * Only in debug code, where `NDEBUG` is not defined, throws #rocket::InvalidState with a message.
 *
 * Usage: `ROCKET_DEBUG_FAIL(fmt, [args]...)`
 */
#define ROCKET_DEBUG_FAIL(fmt, ...)
#else
/**
 * Only in debug code, where `NDEBUG` is not defined, throws #rocket::InvalidState with a message.

 * Usage: `ROCKET_DEBUG_FAIL(fmt, [args]...)`
 */
#define ROCKET_DEBUG_FAIL(fmt, ...) ROCKET_FAIL(fmt __VA_OPT__(,) __VA_ARGS__)
#endif

/**
 * Throws #rocket::InvalidState because of an invalid function call.
 */
#define ROCKET_FAIL_INVALID_CALL() ROCKET_FAIL("Invalid call of function `{}`", ROCKET_PRETTY_FUNCTION)

/**
 * Throws #rocket::InvalidState because of a missing implementation.
 */
#define ROCKET_FAIL_NOT_IMPLEMENTED() ROCKET_FAIL("Not implemented")

/**
 * Throws #rocket::InvalidState because code was reached that was supposed to be unreachable.
 */
#define ROCKET_FAIL_UNREACHABLE_CODE() ROCKET_FAIL("Unreachable code")

/**
 * Throws #rocket::InvalidState if @p expr evaluates to `false`.
 *
 * Usage: `ROCKET_EXPECT(expr, [fmt, [args]...])`
 */
#define ROCKET_EXPECT(expr, ...) \
  if (not (expr)) { \
    ::rocket::internal::expectFailed( \
      ROCKET_EXCEPTION_SL, \
      BOOST_PP_STRINGIZE(expr) \
      __VA_OPT__(,) __VA_ARGS__); \
  }

#ifdef NDEBUG
/**
 * Only in debug code, where `NDEBUG` is not defined, throws #rocket::InvalidState if @p expr evaluates to
 * `false`.
 *
 * Usage: `ROCKET_DEBUG_EXPECT(expr, [fmt, [args]...])`
 */
#define ROCKET_DEBUG_EXPECT(expr, ...)
#else
/**
 * Only in debug code, where `NDEBUG` is not defined, throws #rocket::InvalidState if @p expr evaluates to
 * `false`.
 *
 * Usage: `ROCKET_DEBUG_EXPECT(expr, [fmt, [args]...])`
 */
#define ROCKET_DEBUG_EXPECT(expr, ...) ROCKET_EXPECT(expr __VA_OPT__(,) __VA_ARGS__)
#endif

// EOF
