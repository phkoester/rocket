/*
 * main.cc
 *
 * The `toy` test executable links to Rocket and is a playground for quick and dirty experiments.
 */

#include "rocket/Process.h"
#include "rocket/std-codec.h"
#include "rocket/cl/cl.h"
#include "rocket/log/log.h"

#include <functional>
#include <ranges>

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

ROCKET_LOG_DEFINE(thisIsARatherLongLogId);
ROCKET_LOG_DEFINE(toy);

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

  vector<i32> v1 = { 1, 2, 3 };
  ranges::sort(v1);

  map<i32, i32> m1 = { { 1, 1 }, { 2, 2 }, { 3, 3 } };
  span<const i32> s1  = v1;
  const vector<i32> v2 = { 4, 5, 6 };
  span<const i32> s2  = v2;
  const vector<i32> v3 = { 7, 8, 9 };
  span<const i32> s3  = v3;

  std::set<span<const i32>> spanSet;
  spanSet.insert(s1);
  spanSet.insert(s2);
  spanSet.insert(s3);
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
  optional<vector<string>> args;

  const cl::OptionGroup general("General control");
  const cl::CommandLineConfig config { .usages={ "[OPTION]... [ARG]..." }} ;
  cl::CommandLine cl({
    cl::Option::helpOf(&general, help),
    cl::Option::of(&general, "foo", "f"_c, nullopt, "delve into foo mode", foo),
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
