/**
 * @file assert.h
 *
 * Assertions, argument checks, and expectations.
 *
 * This file may be included several times. If @c NDEBUG is not defined, the assertion macros are active.
 * Otherwise, they expand to a call of #rocket::nop().
 */

// No '#pragma once' here!

#ifndef ROCKET_ASSERT_H
#define ROCKET_ASSERT_H

#include "basic.h" // 'rocket::nop()'

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

#include <optional>
#include <source_location>
#include <string>

// Internal -------------------------------------------------------------------------------------------------

namespace rocket::assert::internal {

[[noreturn]] void onAssertFailed(
    const char* expr,
    const std::optional<std::string>& msg = std::nullopt,
    const std::source_location& sourceLoc = std::source_location::current());

[[noreturn]] void onCheckFailed(
    const char* name,
    const char* expr,
    const std::optional<std::string>& msg = std::nullopt,
    const std::source_location& sourceLoc = std::source_location::current());

[[noreturn]] void onExpectFailed(
    const char* expr,
    const std::optional<std::string>& msg = std::nullopt,
    const std::source_location& sourceLoc = std::source_location::current());

} // namespace rocket::assert::internal

// Macros ---------------------------------------------------------------------------------------------------

/// @cond undocumented

#define ROCKET_ASSERT_1__(expr) \
    if (not (expr)) \
      ::rocket::assert::internal::onAssertFailed(BOOST_PP_STRINGIZE(expr))
#define ROCKET_ASSERT_2__(expr, msg) \
    if (not (expr)) \
      ::rocket::assert::internal::onAssertFailed(BOOST_PP_STRINGIZE(expr), msg)
#define ROCKET_ASSERT__(...) \
    BOOST_PP_TUPLE_ELEM(2, (__VA_ARGS__, ROCKET_ASSERT_2__, ROCKET_ASSERT_1__))

/// @endcond

/**
 * Terminates.
 */
#define ROCKET_TERMINATE_INVALID_CALL() ROCKET_ASSERT(false, ::rocket::S << "Invalid call of function '" << __PRETTY_FUNCTION__ << "'")

/**
 * Terminates.
 */
#define ROCKET_TERMINATE_NOT_IMPLEMENTED() ROCKET_ASSERT(false, "Not implemented")

/**
 * Terminates.
 */
#define ROCKET_TERMINATE_UNREACHABLE_CODE() ROCKET_ASSERT(false, "Unreachable code")

/// @cond undocumented

#define ROCKET_CHECK_2__(name, expr) \
    if (not (expr)) \
      ::rocket::assert::internal::onCheckFailed( \
          BOOST_PP_STRINGIZE(name), \
          BOOST_PP_STRINGIZE(expr))
#define ROCKET_CHECK_3__(name, expr, msg) \
    if (not (expr)) \
      ::rocket::assert::internal::onCheckFailed( \
          BOOST_PP_STRINGIZE(name), \
          BOOST_PP_STRINGIZE(expr), \
          msg)
#define ROCKET_CHECK__(...) \
    BOOST_PP_TUPLE_ELEM(3, (__VA_ARGS__, ROCKET_CHECK_3__, ROCKET_CHECK_2__))

#define ROCKET_EXPECT_1__(expr) \
    if (not (expr)) \
      ::rocket::assert::internal::onExpectFailed(BOOST_PP_STRINGIZE(expr))
#define ROCKET_EXPECT_2__(expr, msg) \
    if (not (expr)) \
      ::rocket::assert::internal::onExpectFailed(BOOST_PP_STRINGIZE(expr), msg)
#define ROCKET_EXPECT__(...) \
    BOOST_PP_TUPLE_ELEM(2, (__VA_ARGS__, ROCKET_EXPECT_2__, ROCKET_EXPECT_1__))

/// @endcond

/**
 * Throws #rocket::except::InvalidState.
 *
 * @throw #rocket::except::InvalidState
 */
#define ROCKET_FAIL_INVALID_CALL() ROCKET_EXPECT(false, ::rocket::S << "Invalid call of function '" << __PRETTY_FUNCTION__ << "'")

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

#define ROCKET_ASSERT(...) ::rocket::nop()
#define ROCKET_CHECK(...) ::rocket::nop()
#define ROCKET_EXPECT(...) ::rocket::nop()

#else

/**
 * Terminates if @p expr evaluates to @c false.
 *
 * Usage: <tt>ROCKET_ASSERT(expr)</tt>
 *
 * Usage: <tt>ROCKET_ASSERT(expr, msg)</tt>
 *
 * Use this macro only in order to handle program states that result from a flawed implementation and make
 * further execution impossible or dangerous. Do not abuse it to catch states that may reasonably occur in
 * normal program execution. If an assertion fails, code needs to be fixed.
 */
#define ROCKET_ASSERT(...) ROCKET_ASSERT__(__VA_ARGS__)(__VA_ARGS__)

/**
 * Throws #rocket::except::InvalidArgument if @p expr evaluates to @c false.
 *
 * Usage: <tt>ROCKET_CHECK(name, expr)</tt>
 *
 * Usage: <tt>ROCKET_CHECK(name, expr, msg)</tt>
 *
 * @throw #rocket::except::InvalidArgument if @p expr evaluates to @c false
 *
 * Use this macro only in order to check function arguments. The first parameter of this macro is always the
 * name of the function parameter the argument of which is to be checked.
 */
#define ROCKET_CHECK(...) ROCKET_CHECK__(__VA_ARGS__)(__VA_ARGS__)

/**
 * Throws #rocket::except::InvalidState if @p expr evaluates to @c false.
 *
 * Usage: <tt>ROCKET_EXPECT(expr)</tt>
 *
 * Usage: <tt>ROCKET_EXPECT(expr, msg)</tt>
 *
 * @throw #rocket::except::InvalidState if @p expr evaluates to @c false
 *
 * Use this macro only in order to handle program states that result from a flawed implementation but may
 * be dealt with by throwing an exception. Do not abuse it to catch states that may reasonably occur in
 * normal program execution. If an expectation fails, code needs to be fixed.
 */
#define ROCKET_EXPECT(...) ROCKET_EXPECT__(__VA_ARGS__)(__VA_ARGS__)

#endif // NDEBUG

// EOF
