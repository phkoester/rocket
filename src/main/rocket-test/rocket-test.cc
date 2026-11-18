/*
 * rocket-test.cc
 */

#include "rocket-test.h"

#include "rocket/assert.h"
#include "rocket/system/system.h"

namespace fs = std::filesystem;

namespace {

// Local constants ------------------------------------------------------------------------------------------

const optional<string> BINARY_DIR = system::env::get<string>("BINARY_DIR").
  transform([](string val) { return optional<string>(std::move(val)); }).
  value_or(nullopt);
const optional<string> CONFIG = system::env::get<string>("CONFIG").
  transform([](string val) { return optional<string>(std::move(val)); }).
  value_or(nullopt);
const optional<string> CONFIGS = system::env::get<string>("CONFIGS").
  transform([](string val) { return optional<string>(std::move(val)); }).
  value_or(nullopt);
const optional<string> SOURCE_DIR = system::env::get<string>("SOURCE_DIR").
  transform([](string val) { return optional<string>(std::move(val)); }).
  value_or(nullopt);

const bool TEST_TERMINAL = system::env::get<bool>("ROCKET_TEST_TERMINAL").value_or(false);

// Local functions ------------------------------------------------------------------------------------------

fs::path
binaryDir() {
  if (not BINARY_DIR || BINARY_DIR->empty()) {
    ROCKET_FAIL("`BINARY_DIR` is not set");
  }
  return *BINARY_DIR;
}

/// Returns the path to the executable @p name in @p dir.
fs::path
executable(const fs::path& dir, string_view name) {
  fs::path ret = dir / fmt::format("{}{}", name, system::executableSuffix());
  if (not is_regular_file(ret)) {
    ROCKET_FAIL("cannot find executable `{}`", ret.string());
  }
  return ret;
}

/// Returns if the build uses a multi-config generator.
bool
isMultiConfig() {
  return CONFIGS && not CONFIGS->empty();
}

fs::path
sourceDir() {
  if (not SOURCE_DIR || SOURCE_DIR->empty()) {
    ROCKET_FAIL("`SOURCE_DIR` is not set");
  }
  return *SOURCE_DIR;
}

/// Returns @p dir, followed by the configuration subdirectory if the build uses a multi-config generator.
fs::path
withConfig(fs::path dir) {
  if (isMultiConfig()) {
    if (not CONFIG || CONFIG->empty()) {
      ROCKET_FAIL("`CONFIG` is not set");
    }
    dir /= *CONFIG;
  }
  return dir;
}

} // namespace

namespace rocket::test {

// Functions ------------------------------------------------------------------------------------------------

fs::path
binExecutable(string_view name) {
  const auto dir = withConfig(binaryDir() / ".." / "bin");
  return executable(dir, name);
}

fs::path
testExecutable(string_view name) {
  const auto dir = withConfig(binaryDir());
  return executable(dir, name);
}

fs::path
testSource(string_view name) {
  fs::path ret = sourceDir() / name;
  if (not is_regular_file(ret)) {
    ROCKET_FAIL("cannot find test source `{}`", name);
  }
  return ret;
}

bool
testTerminal() {
  return TEST_TERMINAL;
}

} // namespace rocket::test

// EOF
