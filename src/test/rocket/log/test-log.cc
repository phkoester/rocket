/*
 * test-log.cc
 *
 * Run this test with
 *
 *   TESTS="rocket-log-log" make tests ARGS="--log test_log=info"
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/log/log.h"

using namespace rocket::log;

ROCKET_LOG_DEFINE(test_log);

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(log, log) {
  ROCKET_LOG(test_log);
  ROCKET_LOG_INFO("Some info");
}

#define NDEBUG 1
#include "rocket/log/log.h"
TEST(log, logNDEBUG) {
  ROCKET_LOG(test_log);
  ROCKET_LOG_INFO("This must not log anything");
}
#undef NDEBUG
#include "rocket/log/log.h"

TEST(log, LogLevelFormat) {
  EXPECT_EQ(fmt::format("{}", LogLevel::none), "none");
  EXPECT_EQ(fmt::format(U"{}", LogLevel::none), U"none");
}

// EOF
