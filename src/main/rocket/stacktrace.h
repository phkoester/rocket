/**
 * @file stacktrace.h
 *
 * A portable stack-trace type and a macro for capturing the current stack trace.
 *
 * If the standard library provides `<stacktrace>` (C++23), #rocket::stacktrace is #std::stacktrace.
 * Otherwise, #rocket::stacktrace is `boost::stacktrace::stacktrace`.
 */

#pragma once

#if defined(ROCKET_OS_MACOS)
  #define ROCKET_HAS_BOOST_STACK_TRACE
#endif

#ifdef ROCKET_HAS_BOOST_STACK_TRACE
  #include <boost/stacktrace/stacktrace.hpp>
#else
  #include <stacktrace>
#endif

namespace rocket {

// `stacktrace` ---------------------------------------------------------------------------------------------

#ifdef ROCKET_HAS_BOOST_STACK_TRACE

/// Returns the current stack trace.
#define ROCKET_STACK_TRACE_CURRENT() ::boost::stacktrace::stacktrace()

/// A stack trace.
using stacktrace = boost::stacktrace::stacktrace;

#else

/// Returns the current stack trace.
#define ROCKET_STACK_TRACE_CURRENT() ::std::stacktrace::current()

/// A stack trace.
using stacktrace = std::stacktrace;

#endif // ROCKET_HAS_BOOST_STACK_TRACE

} // namespace rocket

// EOF
