/*
 * main.cc
 */

#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/log/log.h"

using namespace rocket;
using namespace std;

ROCKET_LOG_DEFINE(thisIsARatherLongName);
ROCKET_LOG_DEFINE(toy);

// Variables -----------------------------------------------------------------------------------------------

auto& out = nio::stdout;

ROCKET_INIT(([&] {
  out.println("ROCKET_INIT: {}, {}", __FUNCTION__, __PRETTY_FUNCTION__);
}));

// Functions -----------------------------------------------------------------------------------------------

extern const char* generated();

void
myExit() {
  out.println("myExit");
  // throw InvalidState("Oopsers!");
}

void
myTerminate() {
  out.println("myTerminate");
}

void tox() {
  ROCKET_LOG(thisIsARatherLongName);

  ROCKET_LOG_INFO("Hello from tox!\nWe're going multi-line ...\nAnd again.");
}

void
toy() {
  ROCKET_LOG(toy);

  tox();

  ROCKET_LOG_TRACE("Hey {}", "there");
}

// `main` ---------------------------------------------------------------------------------------------------

int
main(int argc, char **argv) {
  ROCKET_PROCESS_ERROR("Testing error before `process.init` ...");

  process.atExit(myExit);
  process.atExit(myTerminate, true);

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
    ROCKET_LOG_INFO("Hey {}", "there");
    out.println("This is {}", process.name());
    out.println("{}", generated());
    out.println("args: {}", args);
    toy();
  }

  process.exit(EXIT_SUCCESS);
}

// EOF
