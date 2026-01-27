/*
 * main.cc
 */

#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/log/log.h"

#include "rocket/version.h"

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

ROCKET_LOG_DEFINE(thisIsARatherLongLogId);
ROCKET_LOG_DEFINE(toy);

// Variables -----------------------------------------------------------------------------------------------

auto& out = nio::out;
auto& err = nio::err;

// Functions -----------------------------------------------------------------------------------------------

extern const char* generated();

void
myExit() {
  out.println("myExit");
  // ROCKET_FAIL("Oopsers!");
}

void
myTerminate() {
  // out.println("myTerminate");
}

void
yyy() {
  ROCKET_LOG(thisIsARatherLongLogId);
  ROCKET_LOG_TRACE("Hey {}", "there");
}

void
zzz() {
  ROCKET_LOG(thisIsARatherLongLogId);
  for (int i = 0; i < 10; ++i) {
    yyy();
  }
}

void
toy() {
  ROCKET_LOG(toy);
  ROCKET_LOG_TRACE("Hey {}", "there");
  zzz();
}

// `main` ---------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  ROCKET_PROCESS_ERROR(3, "Testing error before `process.init` ...");

  process.atExit(myExit);
  process.atExit(myTerminate, true);

  process.init(argc, argv, "toy");

  bool help = false;

  cl::OptionGroup general("General control");
  cl::CommandLineParams params { .usages={ "[OPTION]..." }} ;
  cl::CommandLine cl({
    cl::Option::of(&general, "help", "?"_cv, nullopt, "display this help text and exit", help)
  }, params);

  vector<string> args;
  try {
    args = cl.parse(process.args());
    if (help) {
      cl.help(out, true);
    }
  } catch (const exception& ex) {
    cl.handleException(ex, err);
  }

  {
    ROCKET_LOG(toy);
    ROCKET_LOG_INFO("Hey {}", "there");
    out.println("This is {}", process.name());
    out.println("{}", generated());
    out.println("args: {}", args);
    toy();
  }

  out.println("Exiting ...");
  process.exit(EXIT_SUCCESS);
}

// EOF
