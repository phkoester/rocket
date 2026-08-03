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

// #TEST ----------------------------------------------------------------------------------------------------

TEST(log, log) {
  ROCKET_LOG(test_log);
  ROCKET_LOG_INFO("Some info");
}

TEST(log, LogLevelFormat) {
  EXPECT_EQ(fmt::format("{}", LogLevel::none), "none");
  EXPECT_EQ(fmt::format(U"{}", LogLevel::none), U"none");
}

TEST(log, loggerZip) {
  const auto path = testExecutable("logger");
  const string executable = path.string();

  const auto logFilePattern = fs::temp_directory_path() / "logger-@[date].log@[zip]";

  // Run `logger`, check log file

  system::exec( { executable,
    "--log", "all=trace", "--log-out", logFilePattern.string(), "Hi", "folks!" } );

  auto time = rocket::chrono::now<log::internal::Clock>();
  string logFile1 = log::internal::expandLogFilePattern(logFilePattern.string(), time);
  EXPECT_TRUE(fs::is_regular_file(logFile1));

  // Run `logger` again, with 24 hours offset, check zip file and new log file

  system::exec( { executable,
    "--log", "all=trace", "--log-out", logFilePattern.string(), "--offset", "24", "Hi", "again!" } );

  EXPECT_FALSE(fs::is_regular_file(logFile1));
  logFile1 += ".gz";
  EXPECT_TRUE(fs::is_regular_file(logFile1));

  time += 24h;
  const string logFile2 = log::internal::expandLogFilePattern(logFilePattern.string(), time);
  EXPECT_TRUE(fs::is_regular_file(logFile2));

  // Clean up

  Process::atExit([=] {
    fs::remove(logFile1);
    fs::remove(logFile2);
  }, true);
}

// EOF
