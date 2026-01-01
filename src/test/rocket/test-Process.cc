/*
 * test-Process.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/Process.h"
#include "rocket/nio/nio.h"

using namespace rocket;
using namespace std;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(Process, error) {
  nio::StringSink buf;
  process.error(buf, EXIT_SUCCESS, "Test error");
  EXPECT_EQ(buf.str(), "test-rocket-Process: error: Test error\n");
}

// EOF
