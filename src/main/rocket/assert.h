/// @file assert.h
///
/// Assertions, argument checks, and expectations.

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
  rocket::process().error(nio::err, 0, "{}:{}: {}", sl.file_name(), sl.line(),
    rocket::format::Subformat<char>([&] {
    auto params = rocket::format::Subformat<char>::params("\\\x01");
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
  terminate(sl, "assertion `{}` failed{}", expr, rocket::format::Subformat<char>([&] {
    if (fmt.get().size() > 0) {
      auto params = rocket::format::Subformat<char>::params(": \\\x01");
      params.tag("\\\x01", fmt, std::forward<T>(args)...);
      return params;
    }
    return rocket::format::Subformat<char>::params();
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
  flop(sl, name, "check `{}` failed{}", expr, rocket::format::Subformat<char>([&] {
    if (fmt.get().size() > 0) {
      auto params = rocket::format::Subformat<char>::params(": \\\x01");
      params.tag("\\\x01", fmt, std::forward<T>(args)...);
      return params;
    }
    return rocket::format::Subformat<char>::params();
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
  fail(sl, "expectation `{}` failed{}", expr, rocket::format::Subformat<char>([&] {
    if (fmt.get().size() > 0) {
      auto params = rocket::format::Subformat<char>::params(": \\@0");
      params.tag("\\@0", fmt, std::forward<T>(args)...);
      return params;
    }
    return rocket::format::Subformat<char>::params();
  }));
}

} // namespace rocket::internal

// Macros ---------------------------------------------------------------------------------------------------

// Assertions ...............................................................................................

/// Terminates with a message.
///
/// Usage: `ROCKET_TERMINATE(fmt, [args]...)`
///
/// Use this when the program has detected a bug or a corrupted state from which it cannot safely continue,
/// so that throwing an exception and unwinding the stack is not an option. This also applies where
/// exceptions must not escape, such as in destructors and `noexcept` functions. If the caller can reasonably
/// recover, use #ROCKET_FAIL instead.
#define ROCKET_TERMINATE(fmt, ...) \
  ::rocket::internal::terminate(ROCKET_EXCEPTION_SL, fmt __VA_OPT__(,) __VA_ARGS__)

/// Terminates because of an invalid function call.
///
/// Usage: `ROCKET_TERMINATE_INVALID_CALL()`
///
/// Use this in a function that must never be called, for example a virtual function override that a class
/// does not support, if such a call can only be a bug. If the caller can reasonably recover, use
/// #ROCKET_FAIL_INVALID_CALL instead.
#define ROCKET_TERMINATE_INVALID_CALL() \
  ROCKET_TERMINATE("invalid call of function `{}`", ROCKET_PRETTY_FUNCTION)

/// Terminates because of a missing implementation.
///
/// Usage: `ROCKET_TERMINATE_NOT_IMPLEMENTED()`
///
/// Use this as a placeholder for code that is not written yet, if reaching it must stop the program. If the
/// caller can reasonably recover, use #ROCKET_FAIL_NOT_IMPLEMENTED instead.
#define ROCKET_TERMINATE_NOT_IMPLEMENTED() ROCKET_TERMINATE("not implemented")

/// Terminates because code was reached that was supposed to be unreachable.
///
/// Usage: `ROCKET_TERMINATE_UNREACHABLE_CODE()`
///
/// Use this in a branch that only a bug in the surrounding code can reach, such as the `default` case of a
/// `switch` that covers all enumerators. If invalid input or external state may reach the branch, use
/// #ROCKET_FAIL_UNREACHABLE_CODE instead.
#define ROCKET_TERMINATE_UNREACHABLE_CODE() ROCKET_TERMINATE("unreachable code")

/// Terminates if @p expr evaluates to `false`.
///
/// Usage: `ROCKET_ASSERT(expr, [fmt, [args]...])`
///
/// Use this to check internal invariants, preconditions, and postconditions whose violation means a bug in
/// the code itself, not invalid input from the caller. The check stays active in release builds, so @p expr
/// should be cheap. To reject invalid arguments, use #ROCKET_CHECK; to reject a state the caller can handle,
/// use #ROCKET_EXPECT.
#define ROCKET_ASSERT(expr, ...) \
if (not (expr)) { \
  ::rocket::internal::assertFailed( \
    ROCKET_EXCEPTION_SL, \
    BOOST_PP_STRINGIZE(expr) \
    __VA_OPT__(,) __VA_ARGS__); \
}

#ifdef NDEBUG
/// Only in debug code, where `NDEBUG` is not defined, terminates if @p expr evaluates to `false`.
///
/// Usage: `ROCKET_DEBUG_ASSERT(expr, [fmt, [args]...])`
///
/// Use this instead of #ROCKET_ASSERT when the check is too expensive for release builds or sits on a hot
/// path. Because @p expr is not evaluated in release builds, it must not have side effects.
#define ROCKET_DEBUG_ASSERT(expr, ...)
#else
/// Only in debug code, where `NDEBUG` is not defined, terminates if @p expr evaluates to `false`.
///
/// Usage: `ROCKET_DEBUG_ASSERT(expr, [fmt, [args]...])`
///
/// Use this instead of #ROCKET_ASSERT when the check is too expensive for release builds or sits on a hot
/// path. Because @p expr is not evaluated in release builds, it must not have side effects.
#define ROCKET_DEBUG_ASSERT(expr, ...) ROCKET_ASSERT(expr __VA_OPT__(,) __VA_ARGS__)
#endif // NDEBUG

// Argument checks ..........................................................................................

/// Throws #rocket::InvalidArgument with a message.
///
/// Usage: `ROCKET_FLOP(name, fmt, [args]...)`
///
/// Use this to reject an invalid argument when a single expression cannot express the check, for example in
/// the `default` case of a `switch` over an enumeration parameter. @p name is the name of the offending
/// parameter. For a simple condition, use #ROCKET_CHECK.
#define ROCKET_FLOP(name, fmt, ...) \
  ::rocket::internal::flop(ROCKET_EXCEPTION_SL, BOOST_PP_STRINGIZE(name), fmt __VA_OPT__(,) __VA_ARGS__)

/// Throws #rocket::InvalidArgument if @p expr evaluates to `false`.
///
/// Usage: `ROCKET_CHECK(name, expr, [fmt, [args]...])`
///
/// Use this to validate the arguments of a function, typically a public one, where an invalid value is the
/// caller’s mistake and the caller can catch the exception. @p name is the name of the parameter being
/// checked. For bugs in the code itself, use #ROCKET_ASSERT.
#define ROCKET_CHECK(name, expr, ...) \
  if (not (expr)) { \
    ::rocket::internal::checkFailed( \
      ROCKET_EXCEPTION_SL, \
      BOOST_PP_STRINGIZE(name), \
      BOOST_PP_STRINGIZE(expr) \
      __VA_OPT__(,) __VA_ARGS__); \
  }

#ifdef NDEBUG
/// Only in debug code, where `NDEBUG` is not defined, throws #rocket::InvalidArgument if @p expr evaluates
/// to `false`.
///
/// Usage: `ROCKET_DEBUG_CHECK(name, expr, [fmt, [args]...])`
///
/// Use this instead of #ROCKET_CHECK when validating the argument is too expensive for release builds or
/// sits on a hot path. Because @p expr is not evaluated in release builds, it must not have side effects,
/// and callers must not rely on the exception being thrown.
#define ROCKET_DEBUG_CHECK(name, expr, ...)
#else
/// Only in debug code, where `NDEBUG` is not defined, throws #rocket::InvalidArgument if @p expr evaluates
/// to `false`.
///
/// Usage: `ROCKET_DEBUG_CHECK(name, expr, [fmt, [args]...])`
///
/// Use this instead of #ROCKET_CHECK when validating the argument is too expensive for release builds or
/// sits on a hot path. Because @p expr is not evaluated in release builds, it must not have side effects,
/// and callers must not rely on the exception being thrown.
#define ROCKET_DEBUG_CHECK(name, expr, ...) ROCKET_CHECK(name, expr __VA_OPT__(,) __VA_ARGS__)
#endif

// Expectations .............................................................................................

/// Throws #rocket::InvalidState with a message.
///
/// Usage: `ROCKET_FAIL(fmt, [args]...)`
///
/// Use this when an operation cannot proceed because an object or the environment is in an unsuitable state,
/// for example when a stream is already closed or input data is malformed, and a single expression cannot
/// express the check. For a simple condition, use #ROCKET_EXPECT.
#define ROCKET_FAIL(fmt, ...) ::rocket::internal::fail(ROCKET_EXCEPTION_SL, fmt __VA_OPT__(,) __VA_ARGS__)

#ifdef NDEBUG
/// Only in debug code, where `NDEBUG` is not defined, throws #rocket::InvalidState with a message.
///
/// Usage: `ROCKET_DEBUG_FAIL(fmt, [args]...)`
///
/// Use this instead of #ROCKET_FAIL in a branch that should be reported during development but tolerated in
/// release builds, where the macro expands to nothing. The code after it must therefore handle the state in
/// question.
#define ROCKET_DEBUG_FAIL(fmt, ...)
#else
/// Only in debug code, where `NDEBUG` is not defined, throws #rocket::InvalidState with a message.
///
/// Usage: `ROCKET_DEBUG_FAIL(fmt, [args]...)`
///
/// Use this instead of #ROCKET_FAIL in a branch that should be reported during development but tolerated in
/// release builds, where the macro expands to nothing. The code after it must therefore handle the state in
/// question.
#define ROCKET_DEBUG_FAIL(fmt, ...) ROCKET_FAIL(fmt __VA_OPT__(,) __VA_ARGS__)
#endif

/// Throws #rocket::InvalidState because of an invalid function call.
///
/// Usage: `ROCKET_FAIL_INVALID_CALL()`
///
/// Use this in a function that must not be called on the current object or in the current state, for example
/// a virtual function override that a class does not support, if the caller can reasonably recover. If such
/// a call can only be a bug, use #ROCKET_TERMINATE_INVALID_CALL instead.
#define ROCKET_FAIL_INVALID_CALL() ROCKET_FAIL("invalid call of function `{}`", ROCKET_PRETTY_FUNCTION)

/// Throws #rocket::InvalidState because of a missing implementation.
///
/// Usage: `ROCKET_FAIL_NOT_IMPLEMENTED()`
///
/// Use this as a placeholder for a feature or case that is not implemented yet, if the caller can reasonably
/// recover, for example by falling back to another approach. If reaching it must stop the program, use
/// #ROCKET_TERMINATE_NOT_IMPLEMENTED instead.
#define ROCKET_FAIL_NOT_IMPLEMENTED() ROCKET_FAIL("not implemented")

/// Throws #rocket::InvalidState because code was reached that was supposed to be unreachable.
///
/// Usage: `ROCKET_FAIL_UNREACHABLE_CODE()`
///
/// Use this in a branch that valid states never reach but invalid input or external state could, for example
/// the `default` case of a `switch` over an enumeration value read from a file. If only a bug in the
/// surrounding code can reach the branch, use #ROCKET_TERMINATE_UNREACHABLE_CODE instead.
#define ROCKET_FAIL_UNREACHABLE_CODE() ROCKET_FAIL("unreachable code")

/// Throws #rocket::InvalidState if @p expr evaluates to `false`.
///
/// Usage: `ROCKET_EXPECT(expr, [fmt, [args]...])`
///
/// Use this to verify that an object or the environment is in the state an operation requires, for example
/// that a stream is open or that decoded data is well-formed. Unlike #ROCKET_ASSERT, a failure is not
/// necessarily a bug and the caller can catch the exception; unlike #ROCKET_CHECK, it is not about a
/// particular argument.
#define ROCKET_EXPECT(expr, ...) \
  if (not (expr)) { \
    ::rocket::internal::expectFailed( \
      ROCKET_EXCEPTION_SL, \
      BOOST_PP_STRINGIZE(expr) \
      __VA_OPT__(,) __VA_ARGS__); \
  }

#ifdef NDEBUG
/// Only in debug code, where `NDEBUG` is not defined, throws #rocket::InvalidState if @p expr evaluates to
/// `false`.
///
/// Usage: `ROCKET_DEBUG_EXPECT(expr, [fmt, [args]...])`
///
/// Use this instead of #ROCKET_EXPECT when the check is too expensive for release builds or sits on a hot
/// path. Because @p expr is not evaluated in release builds, it must not have side effects, and callers must
/// not rely on the exception being thrown.
#define ROCKET_DEBUG_EXPECT(expr, ...)
#else
/// Only in debug code, where `NDEBUG` is not defined, throws #rocket::InvalidState if @p expr evaluates to
/// `false`.
///
/// Usage: `ROCKET_DEBUG_EXPECT(expr, [fmt, [args]...])`
///
/// Use this instead of #ROCKET_EXPECT when the check is too expensive for release builds or sits on a hot
/// path. Because @p expr is not evaluated in release builds, it must not have side effects, and callers must
/// not rely on the exception being thrown.
#define ROCKET_DEBUG_EXPECT(expr, ...) ROCKET_EXPECT(expr __VA_OPT__(,) __VA_ARGS__)
#endif

// EOF
