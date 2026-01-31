/*
 * filesystem.cc
 */

#include "filesystem.h"

#include "rocket/math/random.h"

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
  process.atExit([=] {
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
  auto gen = math::gen();
  path name = fmt::format("rocket-{}.tmp", math::randomHex(gen, 32));
  path dir = temp_directory_path();
  if (not is_directory(dir)) {
    ROCKET_FAIL("Temporary directory `{}` is not a directory", dir.string());
  }
  path ret = dir / name;

  // Remove the file on exit and on termination
  process.atExit([=] { remove(ret); }, true);

  return ret;
}

} // namespace rocket::filesystem

// EOF
