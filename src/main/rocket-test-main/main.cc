/*
 * main.cc
 */

#include <gmock/gmock.h>

#include <rocket/Process.h>
#include <rocket/cl/cl.h>

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

// #main ----------------------------------------------------------------------------------------------------

i32
main(i32 argc, char** argv) {
  testing::InitGoogleMock(&argc, argv);
  process.init(argc, argv, nullopt, process.codeLocale());

  optional<bool> help;

  const cl::OptionGroup general("General control");
  const cl::CommandLineConfig config { .usages={ "[OPTION]..." }, .otherOutput=true };
  cl::CommandLine cl({
    cl::Option::help(&general, help)
  }, {}, config);
  cl.parse(process.args());

  const i32 status = RUN_ALL_TESTS();
  process.exit(status);
}

// EOF
