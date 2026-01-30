/*
 * main.cc
 */

#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/log/log.h"

#include "rocket/literal.h"
#include "rocket/unicode/unicode.h"
#include "rocket/version.h"

#include <cstring>

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
  out.println("src file name: {}", ROCKET_SRC_FILE);
  zzz();
}

// #main ----------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  ROCKET_PROCESS_ERROR(0, "Testing error before `process.init` ...");

  process.atExit(myExit);
  process.atExit(myTerminate, true);

  process.init(argc, argv, "toy");

  optional<bool> help;
  bool foo; // required!
  optional<vector<string>> files; // required!

  cl::OptionGroup general("General control");
  cl::CommandLineConfig config { .usages={ "[OPTION]... [FILE]..." }} ;
  cl::CommandLine cl({
    cl::Option::helpOf(&general, help),
    cl::Option::of(&general, "foo", "f"_c, nullopt, "delve into foo mode", foo),
  }, {
    cl::Parameter::of("FILE", "file", "an input file", files)
  }, config);

  cl.parse(process.args());

  {
    ROCKET_LOG(toy);
    ROCKET_LOG_INFO("Hey {}", "there");
    out.println("This is {}", process.name());
    out.println("{}", generated());
    out.println("files: {}", files);
    toy();
  }

  out.println("Exiting ...");
  process.exit(EXIT_SUCCESS);
}

// EOF
