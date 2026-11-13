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
  const auto logger = binExecutable("logger");
  const auto loggerDir = logger.parent_path();

  constexpr auto LOG_OUT = "<dir>/<name>-<date>.log<zip>"sv;

  // Run `logger`, check log file

  auto result = system::exec( {
    logger.string(), "--log", "all=trace", "--log-out", LOG_OUT, "Hi", "folks!"
  }, true );
  EXPECT_TRUE(result.ok());
  // nio::out.println("OUT1:[\n{}]", result.out);

  auto time = rocket::chrono::now();
  const string logFile1 = expandLogOut(LOG_OUT, loggerDir, logger.stem().string(), 0, time);
  EXPECT_TRUE(fs::is_regular_file(logFile1));

  // Run `logger` again, with 3 days (72 hours) offset, check zip file and new log file

  result = system::exec( {
    logger.string(), "--log", "all=trace", "--log-out", LOG_OUT, "--offset", "72", "Hi", "again!"
  }, true );
  EXPECT_TRUE(result.ok());
  // nio::out.println("OUT2:[\n{}]", result.out);

  EXPECT_FALSE(fs::is_regular_file(logFile1)); // Old log file must be gone
  const string gzFile = logFile1 + ".gz";
  EXPECT_TRUE(fs::is_regular_file(gzFile)); // Zip file must be present

  time += 72h;
  const string logFile2 = expandLogOut(LOG_OUT, loggerDir, logger.stem().string(), 0, time);
  EXPECT_TRUE(fs::is_regular_file(logFile2)); // New log file must be present

  // Clean up

  error_code ignored;
  fs::remove(gzFile, ignored);
  fs::remove(logFile2, ignored);
}

// EOF
