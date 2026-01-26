/**
 * @file macro.h
 *
 * General-purpose macros.
 */

#pragma once

#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/facilities/check_empty.hpp>
#include <boost/preprocessor/seq/cat.hpp>

#include <mutex>

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Generates a file-unique identifier.
 */
#define ROCKET_ID() BOOST_PP_SEQ_CAT((rocketId)(__LINE__)(__))

/**
 * This macro executes a given function as a static initializer.
 *
 * If curly braces pose a problem, enclose the parameter in parentheses.
 *
 * @param fn the function to execute as a static initializer
 *
 * ## Examples
 *
 * ```
 * i32 x = 0;
 * ROCKET_INIT(([&] { x = 1; }));
 * ```
 */
#define ROCKET_INIT(fn) \
  namespace { \
    struct BOOST_PP_SEQ_CAT((RocketInit)(__LINE__)(__)) { \
      BOOST_PP_SEQ_CAT((RocketInit)(__LINE__)(__))() { fn(); } \
    }; \
    BOOST_PP_SEQ_CAT((RocketInit)(__LINE__)(__)) BOOST_PP_SEQ_CAT((rocketInit)(__LINE__)(__)); \
  }

/**
 * Generates a scoped mutex lock.
 *
 * @param val the mutex to lock
 */
#define ROCKET_MUTEX_LOCK(val) auto BOOST_PP_SEQ_CAT((rocketLock)(__LINE__)(__)) = ::std::scoped_lock(val)

/**
 * If `ns` is empty, expands to nothing. Otherwise, expands to `namespace ns {`.
 *
 * @param ns the namespace to begin, or empty if there is no namespace
 */
#define ROCKET_NAMESPACE_BEGIN(ns) ROCKET_NAMESPACE_BEGIN__(ns)

/**
 * If `ns` is empty, expands to nothing. Otherwise, expands to `}`.
 *
 * @param ns the namespace to end, or empty if there is no namespace
 */
#define ROCKET_NAMESPACE_END(ns) ROCKET_NAMESPACE_END__(ns)

/// @cond undocumented

#define ROCKET_NAMESPACE_BEGIN__(...) __VA_OPT__(namespace) __VA_ARGS__ __VA_OPT__({)
#define ROCKET_NAMESPACE_END__(...) __VA_OPT__(})

/// @endcond

// EOF
