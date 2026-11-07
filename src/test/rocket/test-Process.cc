/*
 * test-Process.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Process.h"
#include "rocket/nio/nio.h"

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(Process, error0) {
  nio::StringSink buf;
  process().error(buf, 0, "Test error");
  EXPECT_EQ(buf.str(), "test-rocket-Process: error: Test error\n");
}

TEST(ProcessDeathTest, error2) {
  EXPECT_EXIT(
    { process().error(nio::err, 2, "Test error"); },
    testing::ExitedWithCode(2), "test-rocket-Process: fatal error: Test error\n");
}

TEST(ProcessDeathTest, exit) {
  EXPECT_EXIT(
    { process().error(nio::err, 7, "Exiting ..."); },
    testing::ExitedWithCode(7), "test-rocket-Process: fatal error: Exiting ...\n");
}

// EOF
