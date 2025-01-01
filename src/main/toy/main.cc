/*
 * main.cc
 */

#include "rocket/codec-std-decl.h"
#include "rocket/codec-std.h"

#include "rocket/Process.h"
#include "rocket/cl.h"
#include "rocket/log.h"
#include "rocket/macro.h"
#include "rocket/terminal.h"

using namespace rocket;
using namespace std;

ROCKET_LOG_DEFINE(toy);

// 'toy' ----------------------------------------------------------------------------------------------------

void
toy() {
  ROCKET_LOG(toy);

  using namespace rocket::terminal;
  Ansi ansi(io::isatty(cout));
  cout << "This is " << ansi.style(bold | white, red) << "bold white on red" << ansi.style() << " on a terminal\n";

  ROCKET_LOG_DEBUG("Some debug logging");
  ROCKET_LOG_INFO("Some info logging");
}

// 'main' ---------------------------------------------------------------------------------------------------

int
main(int argc, char **argv) {
  try {
    ROCKET_ERROR("Test error");
    ROCKET_PROCESS_ERROR("Test process error");
    
    process.init(argc, argv, "toy", true);
    
    cl::CommandLine cl;
    vector<string> args;
    try {
      args = cl.parse(process.args());
    } catch (const exception& ex) {
      cl.handleException(ex);
    }
    
    {
      ROCKET_LOG(toy);
      cout << "This is " << process.name() << '\n';
      cout << "args: " << (S << args) << "\n";
      toy();
    }
    
    process.exit(EXIT_SUCCESS);
  } catch (...) {
    terminate();
  }
}

// EOF
