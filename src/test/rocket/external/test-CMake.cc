/*
 * test-CMake.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/system/system.h"
#include "rocket/version.h" // Test that the version header is available

using namespace rocket::system;
using namespace std;

extern const char* generated();

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(CMake, env) {
  EXPECT_EQ(system::env::get<string>("A"), "A A");
  EXPECT_EQ(system::env::get<string>("B"), "B B");
}

TEST(CMake, generated) {
  EXPECT_EQ(string_view(generated()), "Hello from `generated.cc`!"sv);
}

// EOF
