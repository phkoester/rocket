/*
 * main.cc
 *
 * The `toy` test executable links to Rocket and is a playground for quick and dirty experiments.
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

#define COPYRIGHT "Copyright © 2024–2026 Philip Köster"

// Variables ------------------------------------------------------------------------------------------------

auto& out = nio::out;
auto& err = nio::err;

// Functions ------------------------------------------------------------------------------------------------

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
toy() {
  ROCKET_LOG(toy);

  ROCKET_LOG_TRACE("Hey {}", "there");
}

// #main ----------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  ROCKET_PROCESS_ERROR(0, "Testing error before `process.init` ...");

  Process::atExit(myExit);
  Process::atExit(myTerminate, true);

  process.init(argc, argv, "toy");

  optional<bool> foo;
  optional<bool> help;
  optional<i32> verbose;
  optional<bool> version;
  optional<vector<string>> args;

  const cl::OptionGroup general("General control");
  const cl::CommandLineConfig config { .usages={ "[OPTION]... [ARG]..." }} ;
  cl::CommandLine cl({
    cl::Option::help(&general, help),
    cl::Option::verbose(&general, verbose, 3),
    cl::Option::version(&general, version),
    cl::Option::custom({
      .description="delve into foo mode",
      .group=&general,
      .name="foo",
      .shortName="f"_c
    }, foo),
  }, {
    cl::Parameter::of("ARG", nullopt, "a command-line argument", args)
  }, config);

  cl.parse(process.args());

  {
    ROCKET_LOG(toy);
    ROCKET_LOG_INFO("Hey {}", "there");
    out.println("This is {}", process.name());
    out.println("args: {}", args);
    toy();
  }

  out.println("Exiting ...");
  process.exit(EXIT_SUCCESS);
}

// EOF
