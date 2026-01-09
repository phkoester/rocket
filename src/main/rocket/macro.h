/**
 * @file macro.h
 *
 * General-purpose macros.
 */

#pragma once

#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/facilities/check_empty.hpp>
#include <boost/preprocessor/seq/cat.hpp>

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Generates a file-unique identifier.
 */
#define ROCKET_ID() BOOST_PP_SEQ_CAT((rocketId)(__LINE__)(__))

/**
 * If @p v is empty, expands to @p _true, otherwise to @p _false.
 */
#define ROCKET_IF_EMPTY(v, _true, _false) BOOST_PP_IF(BOOST_PP_CHECK_EMPTY(v), _true, _false)

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
 * int x = 0;
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
 * @param v the mutex to lock
 */
#define ROCKET_MUTEX_LOCK(v) auto BOOST_PP_SEQ_CAT((rocketLock)(__LINE__)(__)) = ::std::scoped_lock(v)

/**
 * If `ns` is empty, expands to nothing. Otherwise, expands to `namespace ns {`.
 *
 * @param ns the namespace to begin, or empty if there is no namespace
 */
#define ROCKET_NS_BEGIN(ns) ROCKET_IF_EMPTY(ns, , namespace ns {)

/**
 * If `ns` is empty, expands to nothing. Otherwise, expands to `}`.
 *
 * @param ns the namespace to end, or empty if there is no namespace
 */
#define ROCKET_NS_END(ns) ROCKET_IF_EMPTY(ns, , })

// EOF
