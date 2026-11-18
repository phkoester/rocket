/// @file stacktrace.h
///
/// Portability layer for #std::stacktrace.
///
/// This header declares #rocket::stacktrace.
///
/// If the standard library provides `<stacktrace>` (C++23), #rocket::stacktrace is `std::stacktrace`.
/// Otherwise, it is `boost::stacktrace::stacktrace`.

#pragma once

#ifdef ROCKET_HAS_STD_STACKTRACE
  #include <stacktrace>
#else
  #include <boost/stacktrace/stacktrace.hpp>
#endif

namespace rocket {

// `stacktrace` ---------------------------------------------------------------------------------------------

#ifdef ROCKET_HAS_STD_STACKTRACE

/// Returns the current stack trace.
#define ROCKET_STACK_TRACE_CURRENT() ::std::stacktrace::current()

/// A stack trace.
using stacktrace = std::stacktrace;

#else

/// Returns the current stack trace.
#define ROCKET_STACK_TRACE_CURRENT() ::boost::stacktrace::stacktrace()

/// A stack trace.
using stacktrace = boost::stacktrace::stacktrace;

#endif // ROCKET_HAS_STD_STACKTRACE

} // namespace rocket

// EOF
