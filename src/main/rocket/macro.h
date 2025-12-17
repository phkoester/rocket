/**
 * @file macro.h
 *
 * General-purpose macros.
 */

#pragma once

#include <boost/preprocessor/seq/cat.hpp>

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Issues an error message on standard error.
 */
#define ROCKET_ERROR(msg) std::cerr << __FILE__ << ':' << __LINE__ << ": " << msg << '\n';

/**
 * Generates a unique identifier.
 */
#define ROCKET_ID BOOST_PP_SEQ_CAT((_rocketId)(__LINE__))

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
      struct BOOST_PP_SEQ_CAT((_RocketInit)(__LINE__)) { \
        BOOST_PP_SEQ_CAT((_RocketInit)(__LINE__))() { f(); } \
      }; \
      BOOST_PP_SEQ_CAT((_RocketInit)(__LINE__)) BOOST_PP_SEQ_CAT((_instance)(__LINE__)); \
    }

// EOF
