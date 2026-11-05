/**
 * @file StackTrace.h
 *
 * A portable stack-trace type.
 */

#pragma once

#if defined(ROCKET_OS_MACOS) && defined(ROCKET_CXX_COMPILER_LLVM)
  #define ROCKET_HAS_BOOST_STACK_TRACE
#endif

#ifndef ROCKET_HAS_BOOST_STACK_TRACE
  #include <stacktrace>
#else
  #include <boost/stacktrace/stacktrace.hpp>
#endif

namespace rocket {

// `StackTrace` ---------------------------------------------------------------------------------------------

#ifndef ROCKET_HAS_BOOST_STACK_TRACE

/// Returns the current stack trace.
#define ROCKET_STACK_TRACE_CURRENT() ::std::stacktrace::current()

/// A stack trace.
using StackTrace = std::stacktrace;

#else

/// Returns the current stack trace.
#define ROCKET_STACK_TRACE_CURRENT() ::boost::stacktrace::stacktrace()

/**
 * A stack trace.
 *
 * Not every standard library implements #std::stacktrace (e.g., libc++ does not), so Boost's is used
 * instead.
 */
using StackTrace = boost::stacktrace::stacktrace;

#endif // ROCKET_HAS_BOOST_STACK_TRACE

} // namespace rocket

// EOF
