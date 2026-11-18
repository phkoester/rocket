/*
 * filesystem.cc
 */

#include "filesystem.h"

#include "rocket/math/random/random.h"
#include "rocket/system/system.h"

#include <fmt/std.h>

using namespace rocket;
using namespace std;

namespace fs = std::filesystem;

namespace rocket::filesystem {

// Functions ------------------------------------------------------------------------------------------------

fs::path
systemTempDir() {
  fs::path ret;
#ifdef ROCKET_OS_WINDOWS
  auto windir = system::env::get<string>("WINDIR");
  if (windir) {
    ret = fs::path(*windir) / "Temp";
  } else {
    ret = fs::temp_directory_path();
  }
#else
  ret = fs::temp_directory_path();
#endif
  if (not fs::is_directory(ret)) {
    ROCKET_FAIL("path `{}` does not point to a directory", ret);
  }
  return ret;
}

fs::path
tempDir() {
  fs::path ret = tempFile();

  // Create the directory
  if (not create_directory(ret)) {
    ROCKET_FAIL("cannot create temporary directory `{}`", ret);
  }

  // Remove the directory on exit, but not on termination
  process().atExit([=] {
    process().info(nio::err, "removing temporary directory `{}` ...", ret);
    error_code ec;
    remove_all(ret, ec);
    if (ec) {
      process().error(nio::err, 0, "cannot remove temporary directory `{}`: {:s}", ret, ec);
    }
  }, false);

  return ret;
}

fs::path
tempFile() {
  auto gen = math::random::gen();
  const fs::path name = fmt::format("rocket-{}.tmp", math::random::hex(gen, 32));
  const fs::path dir = fs::temp_directory_path();
  if (not is_directory(dir)) {
    ROCKET_FAIL("path `{}` does not point to a directory", dir);
  }
  fs::path ret = dir / name;

  // Remove the file on exit and on termination
  process().atExit([=] {
    process().info(nio::err, "removing temporary file `{}` ...", ret);
    error_code ec;
    remove(ret, ec);
    if (ec) {
      process().error(nio::err, 0, "cannot remove temporary file `{}`: {:s}", ret, ec);
    }
  }, true);

  return ret;
}

} // namespace rocket::filesystem

// EOF
