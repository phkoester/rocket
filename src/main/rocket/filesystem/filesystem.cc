// filesystem.cc

#include "filesystem.h"

#include "rocket/math/random/random.h"
#include "rocket/system/system.h"

#include <fmt/std.h>

using namespace rocket;
using namespace rocket::filesystem;
using namespace std;

namespace fs = std::filesystem;

namespace {

// Local functions ------------------------------------------------------------------------------------------

fs::path
newTempPath() {
  fs::path ret = userTempDir();
  if (not fs::is_directory(ret)) {
    ROCKET_FAIL("path `{}` does not point to a directory", ret);
  }

  auto gen = math::random::gen();
  const fs::path name = fmt::format("rocket-{}.tmp", math::random::hex(gen, 32));
  ret /= name;

  return ret;
}

} // namespace

namespace rocket::filesystem {

// Functions ------------------------------------------------------------------------------------------------

fs::path
newTempDir() {
  fs::path ret = newTempPath();

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
newTempFile() {
  const fs::path ret = newTempPath();

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

fs::path
systemTempDir() {
  fs::path ret;
#ifdef ROCKET_OS_WINDOWS
  auto windir = system::env::get<string>("WINDIR");
  if (windir) {
    ret = fs::path(*windir) / "Temp";
  } else {
    ret = userTempDir();
  }
#else
  ret = userTempDir();
#endif
  if (not fs::is_directory(ret)) {
    ROCKET_FAIL("path `{}` does not point to a directory", ret);
  }
  return ret;
}

fs::path
userTempDir() {
  fs::path ret = fs::temp_directory_path();
  if (ret.filename().empty()) {
    ret = ret.parent_path();
  }
  if (not fs::is_directory(ret)) {
    ROCKET_FAIL("path `{}` does not point to a directory", ret);
  }
  return ret;
}

} // namespace rocket::filesystem

// EOF
