/*
 * rocket-test.cc
 */

#include "rocket-test.h"

#include "rocket/assert.h"
#include "rocket/system/system.h"

namespace fs = std::filesystem;

namespace rocket::test {

// Constants ------------------------------------------------------------------------------------------------

const optional<string> BINARY_DIR = system::env::get<string>("BINARY_DIR");
const optional<string> CONFIG = system::env::get<string>("CONFIG");
const optional<string> CONFIGS = system::env::get<string>("CONFIGS");
const optional<string> SOURCE_DIR = system::env::get<string>("SOURCE_DIR");
const bool TEST_TERMINAL = system::env::get<bool>(ROCKET_TEST_TERMINAL).value_or(false);

// Functions ------------------------------------------------------------------------------------------------

fs::path
testExcecutable(string_view name) {
  if (not BINARY_DIR) {
    ROCKET_FAIL("`BINARY_DIR` is not set");
  }
  const string fileName = fmt::format("{}{}", name, system::executableSuffix());
  fs::path ret = fs::path(*BINARY_DIR) / fileName;
  if (is_regular_file(ret)) {
    return ret;
  }
  if (not CONFIG) {
    ROCKET_FAIL("`CONFIG` is not set");
  }
  ret = fs::path(*BINARY_DIR) / *CONFIG / fileName;
  if (is_regular_file(ret)) {
    return ret;
  }
  ROCKET_FAIL("Cannot find test executable `{}`", name);
  return ret;
}

fs::path
testSource(string_view name) {
  if (not SOURCE_DIR) {
    ROCKET_FAIL("`SOURCE_DIR` is not set");
  }
  fs::path ret = fs::path(*SOURCE_DIR) / name;
  if (is_regular_file(ret)) {
    return ret;
  }
  ROCKET_FAIL("Cannot find test source `{}`", name);
  return ret;
}

} // namespace rocket::test

// EOF
