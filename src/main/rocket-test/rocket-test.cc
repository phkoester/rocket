/*
 * rocket-test.cc
 */

#include "rocket-test.h"

#include "rocket/math/random.h"
#include "rocket/system/system.h"

using namespace std::filesystem;

namespace rocket::test {

// Constants ------------------------------------------------------------------------------------------------

const string BINARY_DIR = *system::env::get<string>("BINARY_DIR");
const string CONFIG = *system::env::get<string>("CONFIG");
const bool TEST_TERMINAL = system::env::get<bool>(ROCKET_TEST_TERMINAL).value_or(false);

// Functions ------------------------------------------------------------------------------------------------

path
testExcecutable(string_view name) {
  string fileName = fmt::format("{}{}", name, system::executableSuffix());
  path ret = path(BINARY_DIR) / fileName;
  if (is_regular_file(ret)) {
    return ret;
  }
  ret = path(BINARY_DIR) / CONFIG / fileName;
  if (is_regular_file(ret)) {
    return ret;
  }
  ROCKET_FAIL("Cannot find test executable `{}`", name);
  return ret;
}

} // namespace rocket::test

// EOF
