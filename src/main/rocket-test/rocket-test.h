/**
 * @file rocket-test.h
 *
 * This header must always be the first included file in a test.
 *
 * Parameters:
 *
 * - `ROCKET_TEST_NO_USING_NAMESPACE`: If defined, the `using namespace` directives are not included.
 */

#pragma once

#include "rocket/rocket.h"
#include "rocket/io/io.h"

#include "rocket-test/matcher/matcher.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>

#ifndef ROCKET_TEST_NO_USING_NAMESPACE

using namespace rocket;
using namespace rocket::test;
using namespace rocket::test::matcher;
using namespace std;
using namespace testing;

#endif // ROCKET_TEST_NO_USING_NAMESPACE

// Macros ---------------------------------------------------------------------------------------------------

// `ROCKET_...` .............................................................................................

#undef ROCKET_TEST_PROTECTED
/// Use this macro instead of `protected` to allow access to protected members of a class when testing.
#define ROCKET_TEST_PROTECTED public
#undef ROCKET_TEST_PRIVATE
/// Use this macro instead of `private` to allow access to private members of a class when testing.
#define ROCKET_TEST_PRIVATE public

/**
 * Makes a new unique `filesystem::path` that points to a temporary test file.
 *
 * When the test process finishes, the file is automatically removed.
 */
#define ROCKET_TEST_TEMP_PATH() ::rocket::test::internal::tempPath()

/// An environment variable indicating whether terminal output is tested.
#define ROCKET_TEST_TERMINAL "ROCKET_TEST_TERMINAL"

// Others ...................................................................................................

#ifdef ROCKET_OS_WINDOWS
/// To use with `EXPECT_EXIT`.
#define EXIT_MESSAGE(msg) ""
/// To use with `EXPECT_EXIT`.
#define EXITED_WITH_CODE(code) ExitedWithCode(-1073740791)
/// To use with `EXPECT_EXIT`.
#define KILLED_BY_SIGNAL(signal) ExitedWithCode(-1073740791)
#else
/// To use with `EXPECT_EXIT`.
#define EXIT_MESSAGE(msg) msg
/// To use with `EXPECT_EXIT`.
#define EXITED_WITH_CODE(code) ExitedWithCode(code)
/// To use with `EXPECT_EXIT`.
#define KILLED_BY_SIGNAL(signal) KilledBySignal(signal)
#endif

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
 * @param _fail the expected value of `is.fail()`
 * @param _eof the expected value of `is.eof()`
 * @param _tell the expected value of `rocket::io::tellg(is)`
 */
#define EXPECT_ISTREAM(is, _fail, _eof, _tell) \
  EXPECT_EQ(is.fail(), _fail); \
  EXPECT_EQ(is.eof(), _eof); \
  EXPECT_EQ(::rocket::io::tellg(is), _tell)

namespace rocket::test {

// Constants ------------------------------------------------------------------------------------------------

/// Reflects the environment variable `ROCKET_TEST_TERMINAL`.
extern const bool TEST_TERMINAL;

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

std::filesystem::path tempPath();

} // namespace internal

} // namespace rocket::test

// EOF
