/*
 * test-log.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/log/log.h"

using namespace rocket;
using namespace rocket::log;

ROCKET_LOG_DEFINE(test_log);

// `TEST` ---------------------------------------------------------------------------------------------------

#define NDEBUG 1
#include "rocket/log/log.h"
TEST(log, log_NDEBUG) {
  ROCKET_LOG(test_log);
  ROCKET_LOG_INFO("This must not log anything");
}
#undef NDEBUG
#include "rocket/log/log.h"

TEST(log, log) {
  ROCKET_LOG(test_log);
  ROCKET_LOG_INFO("Some info");
}

// EOF
