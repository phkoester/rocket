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
