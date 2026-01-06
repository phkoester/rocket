/*
 * main.cc
 */

#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/log/log.h"

#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/facilities/check_empty.hpp>

using namespace rocket;
using namespace std;

ROCKET_LOG_DEFINE(toy);

ROCKET_NS_BEGIN();
void foo();
ROCKET_NS_END();

ROCKET_NS_BEGIN(myns);
void foo();
ROCKET_NS_END(myns);

// Local functions ------------------------------------------------------------------------------------------

namespace {

void
toy() {
  ROCKET_LOG(toy);

  ROCKET_LOG_TRACE("Hey {}", "there");
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
      ROCKET_LOG_INFO("Hey {}", "there");
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
