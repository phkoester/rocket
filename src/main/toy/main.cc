/*
 * main.cc
 */

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

  ROCKET_LOG_INFO("hi");
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

int
main(int argc, char **argv) {
  try {
    ROCKET_PROCESS_ERROR("Test process error");

    process.init(argc, argv, "toy");

    cl::CommandLine cl;
    vector<string> args;
    try {
      args = cl.parse(process.args());
    } catch (const exception& ex) {
      cl.handleException(ex, nio::stderr);
    }

    {
      ROCKET_LOG(toy);
      auto& out = nio::stdout;
      out.println("This is {}", process.name());
      out.println("args: {}", args);
      toy();
    }

    process.exit(EXIT_SUCCESS);
  } catch (...) {
    terminate();
  }
}

// EOF
