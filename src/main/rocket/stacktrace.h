/**
 * @file stacktrace.h
 *
 * A portable stack-trace type and a macro for capturing the current stack trace.
 *
 * If the standard library provides `<stacktrace>` (C++23), #rocket::stacktrace is #std::stacktrace.
 * Otherwise, #rocket::stacktrace is `boost::stacktrace::stacktrace`.
 */

#pragma once

#include <version>

#if defined(ROCKET_OS_MACOS)
  #ifdef __cpp_lib_stacktrace
    #error This compiler version supports stack traces
  #endif
#else
  #ifndef __cpp_lib_stacktrace
    #error This compiler version does not support stack traces
  #endif
  #define ROCKET_HAS_STD_STACK_TRACE
#endif

#ifdef ROCKET_HAS_STD_STACK_TRACE
  #include <stacktrace>
#else
  #include <boost/stacktrace/stacktrace.hpp>
#endif

namespace rocket {

// `stacktrace` ---------------------------------------------------------------------------------------------

#ifdef ROCKET_HAS_STD_STACK_TRACE

/// Returns the current stack trace.
#define ROCKET_STACK_TRACE_CURRENT() ::std::stacktrace::current()

/// A stack trace.
using std::stacktrace;

#else

/// Returns the current stack trace.
#define ROCKET_STACK_TRACE_CURRENT() ::boost::stacktrace::stacktrace()

/// A stack trace.
using boost::stacktrace::stacktrace;

#endif // ROCKET_HAS_STD_STACK_TRACE

} // namespace rocket

// EOF
