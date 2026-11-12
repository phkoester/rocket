/*
 * test-log.cc
 *
 * Run this test with
 *
 *   TESTS="rocket-log-log" make tests ARGS="--log test_log=info"
 */

#include "rocket-test/rocket-test.h"

#include "rocket/chrono/chrono.h"
#include "rocket/log/log.h"
#include "rocket/system/system.h"

namespace fs = std::filesystem;

using namespace rocket::log;

ROCKET_LOG_DEFINE(test_log);

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(log, log) {
  ROCKET_LOG(test_log);
  ROCKET_LOG_INFO("Some info");
}

TEST(log, LogLevelFormat) {
  EXPECT_EQ(fmt::format("{}", LogLevel::none), "none");
  EXPECT_EQ(fmt::format(U"{}", LogLevel::none), U"none");
}

TEST(log, loggerZip) {
  const auto loggerPath = binExecutable("logger");
  const auto loggerDir = loggerPath.parent_path();
  const string logger = loggerPath.string();

  const auto logFilePattern = "<dir>/<name>-<date>.log<zip>";

  // Run `logger`, check log file

  auto result = system::exec( {
    logger, "--log", "all=trace", "--log-out", logFilePattern, "Hi", "folks!"
  } );
  EXPECT_TRUE(result.ok());
  // nio::out.println("OUT1:[\n{}]", result.out);

  auto time = rocket::chrono::now<log::internal::Clock>();
  string logFile1 = log::internal::expandLogFilePattern(loggerDir, "logger", logFilePattern, time);
  EXPECT_TRUE(fs::is_regular_file(logFile1));

  // Run `logger` again, with 3 days (72 hours) offset, check zip file and new log file

  result = system::exec( {
    logger, "--log", "all=trace", "--log-out", logFilePattern, "--offset", "72", "Hi", "again!"
  } );
  EXPECT_TRUE(result.ok());
  // nio::out.println("OUT2:[\n{}]", result.out);

  EXPECT_FALSE(fs::is_regular_file(logFile1));
  logFile1 += ".gz";
  EXPECT_TRUE(fs::is_regular_file(logFile1));

  time += 72h;
  const string logFile2 = log::internal::expandLogFilePattern(loggerDir, "logger", logFilePattern, time);
  EXPECT_TRUE(fs::is_regular_file(logFile2));

  // Clean up

  fs::remove(logFile1);
  fs::remove(logFile2);
}

// EOF
