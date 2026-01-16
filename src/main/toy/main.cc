/*
 * main.cc
 */

#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/log/log.h"
#include "rocket/unicode/Character.h"

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

ROCKET_LOG_DEFINE(thisIsARatherLongLogId);
ROCKET_LOG_DEFINE(toy);

// Variables -----------------------------------------------------------------------------------------------

auto& out = nio::stdout;
auto& err = nio::stderr;

// Functions -----------------------------------------------------------------------------------------------

extern const char* generated();

void
myExit() {
  // out.println("myExit");
  // throw InvalidState("Oopsers!");
}

void
myTerminate() {
  // out.println("myTerminate");
}

void zz(i32 level) {
  ROCKET_LOG(thisIsARatherLongLogId);
  ROCKET_LOG_INFO("zz at level {}", level);

  if (level == 4) {
    return;
  }
  this_thread::sleep_for(chrono::milliseconds(100));
  zz(level + 1);
}

void tox() {
  ROCKET_LOG(thisIsARatherLongLogId);
  ROCKET_LOG_INFO("in tox(), threadName={}, id={}", ROCKET_THREAD_NAME(), this_thread::get_id());

  zz(0);

  ROCKET_LOG_INFO("Hello from tox!\nWe're going multi-line ...\nAnd again.");
}

void
toy() {
  ROCKET_LOG(toy);

  tox();

  ROCKET_LOG_TRACE("Hey {}", "there");
}

// `main` ---------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  ROCKET_PROCESS_ERROR("Testing error before `process.init` ...");

  process.atExit(myExit);
  process.atExit(myTerminate, true);

  process.init(argc, argv, "toy");

  bool help = false;

  cl::OptionGroup general("General control");
  cl::CommandLineParams params { .usages={ "[OPTION]..." }} ;
  cl::CommandLine cl({
    cl::Option::of(&general, "help", "?"_c, nullopt, "display this help text and exit", help)
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

  process.exit(EXIT_SUCCESS);
}

// EOF
