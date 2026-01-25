/*
 * rocket-test.cc
 */

#include "rocket-test.h"

#include "rocket/math/random.h"
#include "rocket/str/str.h"
#include "rocket/system/system.h"

using namespace std::filesystem;

namespace rocket::test {

// Constants ------------------------------------------------------------------------------------------------

const bool TEST_TERMINAL = system::env::get<bool>(ROCKET_TEST_TERMINAL).value_or(false);

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

path
tempPath() {
  auto info = ::testing::UnitTest::GetInstance()->current_test_info();
  ROCKET_ASSERT(info);
  auto gen = math::gen();
  path name = fmt::format(
      "rocket-test-{}-{}-{}.tmp",
      info->test_suite_name(), info->name(), math::randomHex(gen, 16));
  path ret = temp_directory_path() / name;

  process.atExit([=] { remove(ret); }, true);

  return ret;
}

} // namespace internal

} // namespace rocket::test

// EOF
