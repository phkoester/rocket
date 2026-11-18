/*
 * test-CMake.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/system/system.h"
#include "rocket/version.h" // Test that the version header is available

using namespace rocket::system;
using namespace std;

#define MODULE rocketExternalCMake

extern const char* generated();

// Tests ====================================================================================================

TEST(MODULE, env) {
  EXPECT_EQ(system::env::get<string>("A"), "A A");
  EXPECT_EQ(system::env::get<string>("B"), "B B");
}

TEST(MODULE, generated) {
  EXPECT_EQ(string_view(generated()), "Hello from `generate-hello`!"sv);
}

TEST(MODULE, version) {
  EXPECT_GT(rocket::VERSION, 0);
}

// EOF
