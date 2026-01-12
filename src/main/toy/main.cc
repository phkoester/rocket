/*
 * main.cc
 */

#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/log/log.h"

#include <iostream>
#include <vector>

using namespace rocket;
using namespace std;

ROCKET_LOG_DEFINE(toy);

// Local functions ------------------------------------------------------------------------------------------

namespace {

void
toy() {
  ROCKET_LOG(toy);

  ROCKET_LOG_TRACE("Hey {}", "there");

  string s = "Hello there";

  Cow<string_view, string> cow(s);
  cout << "cow=" << cow.get() << endl;
  cow = "I changed my mind";
  cout << "cow=" << cow.get() << endl;
  cout << "cow.owned=" << cow.owned() << endl;
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

void
myExit() {
  cout << "myExit" << endl;
  // throw InvalidState("Oopsers!");
}

void
myTerminate() {
  cout << "myTerminate" << endl;
  // throw 7;
}

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
    auto& out = nio::stdout;
    out.println("This is {}", process.name());
    out.println("args: {}", args);
    toy();
  }

  process.exit(EXIT_SUCCESS);
}

// EOF
