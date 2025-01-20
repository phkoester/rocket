/*
 * main.cc
 */

#include "rocket/Process.h"
#include "rocket/cl.h"

#include <gmock/gmock.h>

using namespace rocket;
using namespace std;

GTEST_API_ int
main(int argc, char** argv) {
  try {
    testing::InitGoogleMock(&argc, argv);
    process.init(argc, argv);
    
    bool help = false;
    
    cl::OptionGroup general("General control");
    cl::CommandLineParams params { .usages={ "[OPTION]..." }, .otherOutput=true };
    cl::CommandLine cl({
      cl::Option::of(&general, "help", 'h', nullopt, "display this help text and exit", help)
    }, params);
    
    try {
      cl.parse(process.args());
      if (help) {
        cl.help(cout, true);
      }
    } catch (const exception& ex) {
      cl.handleException(ex);
    }
    
    int status = RUN_ALL_TESTS();
    process.exit(status);
  } catch (...) {
    terminate();
  }
}

// EOF
