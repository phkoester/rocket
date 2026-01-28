/*
 * test-Process.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Process.h"
#include "rocket/nio/nio.h"

// #TEST ----------------------------------------------------------------------------------------------------

TEST(Process, error0) {
  nio::StringSink buf;
  process.error(buf, 0, "Test error");
  EXPECT_EQ(buf.str(), "test-rocket-Process: error: Test error\n");
}

TEST(ProcessDeathTest, error2) {
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_EXIT(
      { process.error(nio::err, 2, "Test error"); },
      EXITED_WITH_CODE(2), EXIT_MESSAGE("test-rocket-Process: fatal error: Test error\n"));
}

TEST(ProcessDeathTest, exit) {
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_EXIT(
      { nio::err.println("Exiting ..."); process.exit(7); },
      EXITED_WITH_CODE(7), EXIT_MESSAGE("Exiting \\.\\.\\."));
}

// EOF
