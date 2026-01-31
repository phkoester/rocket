/*
 * main.cc
 */

#include "rocket/Process.h"
#include "rocket/cl/cl.h"

#include <gmock/gmock.h>

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

GTEST_API_ i32
main(i32 argc, char** argv) {
  testing::InitGoogleMock(&argc, argv);
  process.init(argc, argv, nullopt, process.codeLocale());

  optional<bool> help;

  cl::OptionGroup general("General control");
  cl::CommandLineConfig config { .usages={ "[OPTION]..." }, .otherOutput=true };
  cl::CommandLine cl({
    cl::Option::helpOf(&general, help)
  }, {}, config);
  cl.parse(process.args());

  i32 status = RUN_ALL_TESTS();
  process.exit(status);
}

// EOF
