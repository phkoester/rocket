/*
 * test-Process.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/Process.h"
#include "rocket/nio/nio.h"

using namespace rocket;
using namespace std;
using namespace testing;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(Process, error) {
  nio::StringSink buf;
  process.error(buf, EXIT_SUCCESS, "Test error");
  EXPECT_EQ(buf.str(), "test-rocket-Process: error: Test error\n");
}

TEST(ProcessDeathTest, exit) {
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_EXIT(
      { nio::stderr.println("Exiting ..."); process.exit(7); },
      ExitedWithCode(7), "Exiting \\.\\.\\.");
}

// EOF
