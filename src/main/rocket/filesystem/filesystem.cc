/*
 * filesystem.cc
 */

#include "filesystem.h"

#include "rocket/math/random/random.h"

using namespace std::filesystem;

namespace rocket::filesystem {

// Functions ------------------------------------------------------------------------------------------------

path
tempDir() {
  path ret = tempFile();

  // Create the directory
  if (not create_directory(ret)) {
    ROCKET_FAIL("Cannot create temporary directory `{}`", ret.string());
  }

  // Remove the directory on exit, but not on termination
  Process::atExit([=] {
    std::error_code error;
    remove_all(ret, error);
    if (error) {
      ROCKET_FAIL("Cannot remove temporary directory `{}`: {}", ret.string(), error.message());
    }
  });

  return ret;
}

path
tempFile() {
  auto gen = math::random::gen();
  const path name = fmt::format("rocket-{}.tmp", math::random::hex(gen, 32));
  const path dir = temp_directory_path();
  if (not is_directory(dir)) {
    ROCKET_FAIL("Temporary directory `{}` is not a directory", dir.string());
  }
  path ret = dir / name;

  // Remove the file on exit and on termination
  Process::atExit([=] { remove(ret); }, true);

  return ret;
}

} // namespace rocket::filesystem

// EOF
