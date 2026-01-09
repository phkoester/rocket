/*
 * rocket-gtest.cc
 */

#include "rocket-gtest.h"

#include "rocket/math/random.h"
#include "rocket/str/str.h"

using namespace rocket;
using namespace std;
using namespace std::filesystem;

namespace rocket::gtest::internal {

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

} // namespace rocket::gtest::internal

// EOF
