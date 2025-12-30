/*
 * testing.cc
 */

#include "testing.h"

#include "rocket/random.h"

using namespace rocket;
using namespace std;
using namespace std::filesystem;

namespace rocket::gtest {

path
tempPath() {
  auto info = ::testing::UnitTest::GetInstance()->current_test_info(); \
  auto gen = random::gen();
  path name = fmt::format("test-{}-{}-{}.tmp", info->test_suite_name(), info->name(), random::randomHex(gen, 16));
  path ret = temp_directory_path() / name;

  process.atExit([=] {
    remove(ret);
  });

  return ret;
}

} // namespace rocket::gtest

// EOF
