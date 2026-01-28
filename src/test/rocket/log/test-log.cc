/*
 * test-log.cc
 *
 * Run this test with
 *
 *   TESTS="rocket-log-log" make tests ARGS="--log test_log=info"
 */

#include "rocket-test/rocket-test.h"

#include "rocket/log/log.h"

using namespace rocket::log;

ROCKET_LOG_DEFINE(test_log);

// #TEST ----------------------------------------------------------------------------------------------------

TEST(log, log) {
  ROCKET_LOG(test_log);
  ROCKET_LOG_INFO("Some info");
}

TEST(log, LogLevelFormat) {
  EXPECT_EQ(fmt::format("{}", LogLevel::none), "none");
  EXPECT_EQ(fmt::format(U"{}", LogLevel::none), U"none");
}

// EOF
