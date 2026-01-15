/*
 * rocket-gtest.cc
 */

#include "rocket-gtest.h"

#include "rocket/math/random.h"
#include "rocket/str/str.h"
#include "rocket/system/system.h"

using namespace std::filesystem;

namespace rocket::gtest {

// Constants ------------------------------------------------------------------------------------------------

const bool TEST_TERMINAL = system::env::get<bool>(ROCKET_TEST_TERMINAL).value_or(false);

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

path
tempPath(const char* file) {
  string s = file;
  str::replaceIn<char>(s, "src/test/", "", 1);
  str::replaceIn<char>(s, "test-", "", 1);
  str::replaceIn<char>(s, "src/bench/", "", 1);
  str::replaceIn<char>(s, "bench-", "", 1);
  str::replaceIn<char>(s, ".cc", "", 1);
  str::replaceIn<char>(s, "/", "_");

  auto info = ::testing::UnitTest::GetInstance()->current_test_info(); \
  auto gen = math::gen();
  path name = fmt::format("rocket-gtest-{}-{}-{}-{}.tmp",
      s, info->test_suite_name(), info->name(), math::randomHex(gen, 16));
  path ret = temp_directory_path() / name;

  process.atExit([=] { remove(ret); }, true);

  return ret;
}

} // namespace internal

} // namespace rocket::gtest

// EOF
