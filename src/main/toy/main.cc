/*
 * main.cc
 */

#include "rocket/codec-std-decl.h"
#include "rocket/codec-std.h"

#include "rocket/Process.h"
#include "rocket/cl.h"
#include "rocket/log.h"

using namespace rocket;
using namespace std;

ROCKET_LOG_DEFINE(toy);

// Local functions ------------------------------------------------------------------------------------------

namespace {

void
toy() {
  ROCKET_LOG(toy);
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

int
main(int argc, char **argv) {
  try {
    ROCKET_ERROR("Test error");
    ROCKET_PROCESS_ERROR("Test process error");

    process.init(argc, argv, "toy");

    cl::CommandLine cl;
    vector<string> args;
    try {
      args = cl.parse(process.args());
    } catch (const exception& ex) {
      cl.handleException(ex);
    }

    {
      ROCKET_LOG(toy);
      cout << "This is " << process.name() << '\n';
      cout << "args: " << (S << args) << "\n";
      toy();
    }

    process.exit(EXIT_SUCCESS);
  } catch (...) {
    terminate();
  }
}

// EOF
