/*
 * test-Process.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/Process.h"
#include "rocket/nio.h"
#include "rocket-gtest/matcher.h"

using namespace rocket;
using namespace rocket::gtest::matcher;
using namespace std;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(Process, error) {
  string buf;
  nio::StringSink sink(buf);
  process.error(sink, EXIT_SUCCESS, "Test error");
  EXPECT_EQ(buf, "test-rocket-Process: error: Test error\n");
}

// EOF
