/**
 * @file testing.h
 *
 * This header must always be the first included file in a test.
 */

#pragma once

/**
 * A macro indicating we are in a test.
 *
 * This must be defined before including any other header file.
 */
#define TESTING

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Macros ---------------------------------------------------------------------------------------------------

#undef ROCKET_PROTECTED
#define ROCKET_PROTECTED public
#undef ROCKET_PRIVATE
#define ROCKET_PRIVATE public

/**
 * Checks if an environment variable is set to `true`. If it is not, the test is skipped.
 */
#define EXPECT_ENV(name) \
    { \
      const char* p = getenv(name); \
      if (not p || (strcmp(p, "1") != 0 && strcmp(p, "true") != 0)) \
        GTEST_SKIP_("Skipping test because `" name "` is not set\n"); \
    }

/**
 * Checks the state of a `std::istream`.
 *
 * @param is the input stream
 * @param fail__ the expected value of `is.fail()`
 * @param eof__ the expected value of `is.eof()`
 * @param tell__ the expected value of `rocket::io::tellg(is)`
 */
#define EXPECT_ISTREAM(is, fail__, eof__, tell__) \
    EXPECT_EQ(is.fail(), fail__); \
    EXPECT_EQ(is.eof(), eof__); \
    EXPECT_EQ(::rocket::io::tellg(is), tell__)

// EOF
