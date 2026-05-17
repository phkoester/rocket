/*
 * filesystem.cc
 */

#include "filesystem.h"

#include "rocket/math/random/random.h"
#include "rocket/system/system.h"

namespace fs = std::filesystem;

using namespace rocket;
using namespace std;

namespace rocket::filesystem {

// Functions ------------------------------------------------------------------------------------------------

fs::path
systemTempDir() {
  fs::path ret;
#ifdef ROCKET_OS_WINDOWS
  auto windir = system::env::get<string>("WINDIR");
  if (windir) {
    ret fs::path(*windir) / "Temp";
  } else {
    ret = fs::temp_directory_path();
  }
#else
  ret = fs::temp_directory_path();
#endif
  if (not fs::is_directory(ret)) {
    ROCKET_FAIL("Path `{}` does not point to a directory", ret.string());
  }
  return ret;
}

fs::path
tempDir() {
  fs::path ret = tempFile();

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

fs::path
tempFile() {
  auto gen = math::random::gen();
  const fs::path name = fmt::format("rocket-{}.tmp", math::random::hex(gen, 32));
  const fs::path dir = fs::temp_directory_path();
  if (not is_directory(dir)) {
    ROCKET_FAIL("Path `{}` does not point to a directory", dir.string());
  }
  fs::path ret = dir / name;

  // Remove the file on exit and on termination
  Process::atExit([=] { remove(ret); }, true);

  return ret;
}

} // namespace rocket::filesystem

// EOF
