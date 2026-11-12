/*
 * main.cc
 *
 * The `toy` executable links to Rocket and is a playground for quick and dirty experiments.
 */

#include "rocket/Process.h"
#include "rocket/enum.h"
#include "rocket/scope.h"
#include "rocket/version.h"
#include "rocket/cl/cl.h"
#include "rocket/log/log.h"
#include "rocket/system/system.h"
#include "rocket/system/terminal/terminal.h"

#include <string>

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

ROCKET_LOG_DEFINE(thisIsARatherLongLogId);
ROCKET_LOG_DEFINE(toy);

#define COPYRIGHT "Copyright © 2024–2026 Philip Köster"

// `Color` --------------------------------------------------------------------------------------------------

enum class Color : u8 { Red, Green, Blue };

ROCKET_ENUM_DECLARE(, Color, Color); // NOLINT(*-internal-linkage)
ROCKET_ENUM_DEFINE(, Color, Color, (Red)(Green)(Blue));

namespace {

struct A {
  int a;

  virtual ~A() = default;
};

struct B : A {
  int b;

  virtual ~B() = default;
};

struct C : B {
  int c;

  virtual ~C() = default;
};

[[maybe_unused]]
void handle(const A&) { nio::out.println("handle.A"); }

[[maybe_unused]]
void handle(const B&) { nio::out.println("handle.B"); }

// Local functions ------------------------------------------------------------------------------------------

void
throwInvalidState() {
  throw InvalidState("Oops");
}

void
myExit() {
  nio::out.println("myExit");
  // ROCKET_FAIL("Oopsers!");
}

void
myTerminate() {
  nio::out.println("myTerminate");
}

void
toy() {
  ROCKET_SCOPE_EXIT([] { nio::out.println("Hello from ROCKET_SCOPE_EXIT"); });

  nio::out.println("Hello from toy");

  nio::out.println("terminal size: {}", system::terminal::size(nio::out));

  C c;
  handle(c);

  auto result = system::exec("echo 'Hello");
  nio::out.println("result.ok(): {}", result.ok());
  nio::out.println("result.status: {}", result.status);
  nio::out.println("result.signal: {}", result.signal);
  nio::out.println("result.error: {}", result.error);
  nio::out.println("result.err: {}", result.err);
  nio::out.println("result.out: {}", result.out);
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  ROCKET_PROCESS_ERROR(0, "Testing error before `process().init` ...");

  process().atExit(myExit);
  process().atExit(myTerminate, true);

  nio::out.println("invocation path: {}", process().invocationPath());
  nio::out.println("invocation stem: {}", process().invocationStem());

  process().init(argc, argv, "toy");

  optional<vector<Color>> colors;
  optional<bool> foo;
  optional<bool> help;
  optional<u64> verbose;
  optional<bool> version;
  optional<vector<string>> args;

  const cl::OptionGroup general("General control");
  const cl::CommandLineConfig config {
    .usages={ "[OPTION]... [ARG]..." },
    .version=fmt::format("{} {}\n\n{}", "Rocket", ROCKET_VERSION_NAME, COPYRIGHT)
  };
  cl::CommandLine cl({
    cl::Option::custom({
      .choices=set<string> { "Red", "Green", "Blue" },
      .description="add a color",
      .group=&general,
      .maxOccurs=3,
      .name="color",
      .shortName="c"_c
    }, colors),
    cl::Option::custom({
      .description="delve into foo mode",
      .group=&general,
      .name="foo",
      .shortName="f"_c,
      .verboseDescription="delve into the fabulous furry foo mode"
    }, foo),
    cl::Option::help(&general, help),
    cl::Option::verbose(&general, 3, verbose),
    cl::Option::version(&general, version),
  }, {
    cl::Parameter::make({ .description="a command-line argument", .name="ARG" }, args)
  }, config);

  cl.parse(process().args());

  {
    ROCKET_LOG(toy);
    ROCKET_LOG_INFO("Hey {}", "there");
    nio::out.println("This is {}", process().name());
    nio::out.println("colors: {}", colors);
    nio::out.println("foo: {}", foo);
    nio::out.println("verbose: {}", verbose);
    nio::out.println("args: {}", args);
  }

  toy();

  throwInvalidState();

  nio::out.println("Exiting ...");
  process().exit(EXIT_SUCCESS);
}

// EOF
