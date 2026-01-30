/*
 * main.cc
 */

#define ROCKET_TEST

#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/format/std.h"
#include "rocket/std/chrono.h"
#include "rocket/log/log.h"

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

auto& out = nio::out;
auto& err = nio::err;

ROCKET_LOG_DEFINE(logger);

// Functions ------------------------------------------------------------------------------------------------

int
run(const vector<string>& args) {
  ROCKET_LOG(logger);
  ROCKET_LOG_INFO("args: {}", args);
  return EXIT_SUCCESS;
}

// #main ----------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  process.init(argc, argv, "logger");

  // Parse command line

  optional<bool> help;
  optional<i32> hours;

  cl::OptionGroup general("General control");
  cl::CommandLineParams params { .usages={ "[OPTION]... [ARG]..." }} ;
  cl::CommandLine cl({
    cl::Option::helpOf(&general, help),
    cl::Option::of(&general, "offset", nullopt, "HOURS", "hour offset", hours)
  }, {}, params);

  vector<string> args;
  try {
    args = cl.parse(process.args());
    if (help.value_or(false)) {
      cl.help(out, true);
    }
  } catch (const exception& ex) {
    cl.handleException(ex, err);
  }

  // Apply hour offset

  if (hours) {
    rocket::chrono::internal::setClockOffset(std::chrono::hours(*hours));
  }

  // Run

  ROCKET_LOG(logger);
  out.println("This is {}", process.name());
  return run(args);
}

// EOF
