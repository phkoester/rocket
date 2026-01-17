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

  bool help = false;

  cl::OptionGroup general("General control");
  cl::CommandLineParams params { .usages={ "[OPTION]..." }, .otherOutput=true };
  cl::CommandLine cl({
    cl::Option::of(&general, "help", "h"_cv, nullopt, "display this help text and exit", help)
  }, params);

  try {
    cl.parse(process.args());
    if (help) {
      cl.help(nio::stdout, true);
    }
  } catch (const exception& ex) {
    cl.handleException(ex, nio::stderr);
  }

  i32 status = RUN_ALL_TESTS();
  process.exit(status);
}

// EOF
