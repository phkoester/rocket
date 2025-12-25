/**
 * @file macro.h
 *
 * General-purpose macros.
 */

#pragma once

#include "nio.h"

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/facilities/check_empty.hpp>
#include <boost/preprocessor/logical/not.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/cat.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

#include <string>

// Macros ---------------------------------------------------------------------------------------------------

/**
 * If `__VA_ARGS__` is empty, expands to nothing. Otherwise, expands to `, __VA_ARGS__`.
 *
 * Usage: `ROCKET_COMMA_IF_VA_ARGS(__VA_ARGS__)`
 */
#define ROCKET_COMMA_IF_VA_ARGS(...) \
    BOOST_PP_COMMA_IF(BOOST_PP_NOT(BOOST_PP_CHECK_EMPTY(BOOST_PP_TUPLE_ELEM(0, (__VA_ARGS__))))) \
    __VA_ARGS__

/**
 * Issues an error message on standard error.
 *
 * Usage: `ROCKET_ERROR(fmt, [args]...])`
 */
#define ROCKET_ERROR(fmt, ...) { \
  ::std::string msg; \
  ::rocket::nio::StringSink sink(msg); \
  sink.print("{}:{}: ", __FILE__, __LINE__); \
  sink.print( \
      fmt \
      ROCKET_COMMA_IF_VA_ARGS(__VA_ARGS__)); \
  ::rocket::nio::stderr.writeln(msg); \
}

/**
 * Generates a unique identifier.
 */
#define ROCKET_ID BOOST_PP_SEQ_CAT((rocketId)(__LINE__)(__))

/**
 * This macro executes a given function as a static initializer.
 *
 * If curly braces pose a problem, enclose the parameter in parentheses.
 *
 * ## Examples
 *
 * ```
 * int x = 0;
 * ROCKET_INIT(([&] { x = 1; }));
 * ```
 */
#define ROCKET_INIT(f) \
    namespace { \
      struct BOOST_PP_SEQ_CAT((RocketInit)(__LINE__)(__)) { \
        BOOST_PP_SEQ_CAT((RocketInit)(__LINE__)(__))() { f(); } \
      }; \
      BOOST_PP_SEQ_CAT((RocketInit)(__LINE__)(__)) BOOST_PP_SEQ_CAT((rocketInit)(__LINE__)(__)); \
    }

// EOF
