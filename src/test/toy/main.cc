/*
 * main.cc
 *
 * The `toy` test executable links to Rocket and is a playground for quick and dirty experiments.
 */

#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/enum.h"
#include "rocket/log/log.h"
#include "rocket/version.h"

#include <scn/istream.h>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

#include <cstdio>

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

ROCKET_LOG_DEFINE(thisIsARatherLongLogId);
ROCKET_LOG_DEFINE(toy);

#define COPYRIGHT "Copyright © 2024–2026 Philip Köster"

// #Color ---------------------------------------------------------------------------------------------------

enum class Color : u8 { Red, Green, Blue };

ROCKET_ENUM_DECLARE(, Color, Color); // NOLINT(*-internal-linkage)
ROCKET_ENUM_DEFINE(, Color, Color, (Red)(Green)(Blue));

namespace {

// Local variables ------------------------------------------------------------------------------------------

auto& out = nio::out;

// Local functions ------------------------------------------------------------------------------------------

void
myExit() {
  out.println("myExit");
  // ROCKET_FAIL("Oopsers!");
}

void
myTerminate() {
  // out.println("myTerminate");
}

using FileDescriptorStream = boost::iostreams::stream<boost::iostreams::file_descriptor_source>;

unique_ptr<istream>
makeIstream(FILE *f) {
  namespace bio = boost::iostreams;
  return make_unique<FileDescriptorStream>(bio::file_descriptor_source(fileno(f), bio::never_close_handle));
}

void
toy() {
  ROCKET_LOG(toy);

  const auto is = makeIstream(stdin);
  string line;
  if (getline(*is, line)) {
    nio::out.println("LINE: {}", line);
  }

  ROCKET_LOG_TRACE("Hey {}", "there");
}

} // namespace

// #main ----------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  ROCKET_PROCESS_ERROR(0, "Testing error before `process.init` ...");

  Process::atExit(myExit);
  Process::atExit(myTerminate, true);

  process.init(argc, argv, "toy");

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

  cl.parse(process.args());

  {
    ROCKET_LOG(toy);
    ROCKET_LOG_INFO("Hey {}", "there");
    out.println("This is {}", process.name());
    out.println("colors: {}", colors);
    out.println("foo: {}", foo);
    out.println("verbose: {}", verbose);
    out.println("args: {}", args);
    toy();
  }

  out.println("Exiting ...");
  process.exit(EXIT_SUCCESS);
}

// EOF
