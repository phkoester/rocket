/**
 * @file macro.h
 *
 * General-purpose macros.
 */

#pragma once

#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/facilities/check_empty.hpp>
#include <boost/preprocessor/logical/not.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/cat.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

// Macros ---------------------------------------------------------------------------------------------------

/**
 * If `__VA_ARGS__` is empty, expands to nothing. Otherwise, expands to `, __VA_ARGS__`.
 *
 * Usage: `ROCKET_COMMA_AND_VA_ARGS(__VA_ARGS__)`
 */
#define ROCKET_COMMA_AND_VA_ARGS(...) \
    BOOST_PP_COMMA_IF(BOOST_PP_NOT(BOOST_PP_CHECK_EMPTY(BOOST_PP_TUPLE_ELEM(0, (__VA_ARGS__))))) \
    __VA_ARGS__

/**
 * Generates a file-unique identifier.
 */
#define ROCKET_ID() BOOST_PP_SEQ_CAT((rocketId)(__LINE__)(__))

/**
 * If @p v is empty, expands to @p t, otherwise to @p f.
 */
#define ROCKET_IF_EMPTY(v, t, f) BOOST_PP_IF(BOOST_PP_CHECK_EMPTY(v), t, f)

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
 * Usage: `ROCKET_MUTEX_LOCK(mutex)`
 */
#define ROCKET_MUTEX_LOCK(v) auto BOOST_PP_SEQ_CAT((rocketLock)(__LINE__)(__)) = ::std::scoped_lock(v)

/**
 * If `ns` is empty, expands to nothing. Otherwise, expands to `namespace ns {`.
 */
#define ROCKET_NS_BEGIN(ns) ROCKET_IF_EMPTY(ns, , namespace ns {)

/**
 * If `ns` is empty, expands to nothing. Otherwise, expands to `}`.
 */
#define ROCKET_NS_END(ns) ROCKET_IF_EMPTY(ns, , })

// EOF
