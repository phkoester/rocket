/*
 * main.cc
 *
 * The `toy` test executable links to Rocket and is a playground for quick and dirty experiments.
 */

#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/log/log.h"

#include <functional>

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

ROCKET_LOG_DEFINE(thisIsARatherLongLogId);
ROCKET_LOG_DEFINE(toy);

// MyStruct ------------------------------------------------------------------------------------------------

struct MyStruct {
  int n;
};

auto
operator<=>(const MyStruct& lhs, const MyStruct& rhs) {
  return lhs.n <=> rhs.n;
}

bool
operator==(const MyStruct& lhs, const MyStruct& rhs) {
  return std::is_eq(lhs <=> rhs);
}


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

  MyStruct m1 { 1 };
  MyStruct m2 { 2 };

  out.println("m1 == m2: {}", m1 == m2);
  out.println("m1 != m2: {}", m1 != m2);
  out.println("m1 < m2: {}", m1 < m2);
  out.println("m1 <= m2: {}", m1 <= m2);
  out.println("m1 >= m2: {}", m1 >= m2);

  float f1 = 1.0f;
  float f2 = 2.0f;

  auto cmp = f1 <=> f2;
  out.println("cmp.lt: {}", std::is_lt(cmp));
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
