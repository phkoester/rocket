/**
 * @file rocket-gtest.h
 *
 * This header must always be the first included file in a test.
 */

#pragma once

/**
 * A macro indicating we are in a test.
 *
 * This must be defined before including any other header file.
 */
#define ROCKET_TESTING

#include "rocket/rocket.h"
#include "rocket/io/io.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>

// Macros ---------------------------------------------------------------------------------------------------

#undef ROCKET_TESTING_PROTECTED
/// Use this macro instead of `protected` to allow access to protected members of a class when testing.
#define ROCKET_TESTING_PROTECTED public
#undef ROCKET_TESTING_PRIVATE
/// Use this macro instead of `private` to allow access to private members of a class when testing.
#define ROCKET_TESTING_PRIVATE public

/**
 * Checks if an environment variable is set to `true`. If it is not, the test is skipped.
 *
 * @param name the name of the environment variable
 */
#define EXPECT_ENV(name) { \
  const char* p = getenv(name); \
  if (not p || (strcmp(p, "1") != 0 && strcmp(p, "true") != 0)) { \
    GTEST_SKIP_("Skipping test because `" name "` is not set\n"); \
  } \
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

/**
 * Makes a new unique `filesystem::path` that points to a temporary test file.
 *
 * When the test process finishes, the file is automatically removed.
 */
#define ROCKET_GTEST_TEMP_PATH() ::rocket::gtest::internal::tempPath(__FILE__)

namespace rocket::gtest::internal {

// Internal -------------------------------------------------------------------------------------------------

std::filesystem::path tempPath(const char* file);

} // namespace rocket::gtest::internal

// EOF
