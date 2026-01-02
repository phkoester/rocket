/*
 * main.cc
 */

#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/format/std.h"
#include "rocket/log/log.h"

#include <fmt/xchar.h>

using namespace rocket;
using namespace std;

ROCKET_LOG_DEFINE(toy);

// Local functions ------------------------------------------------------------------------------------------

namespace {

void
toy() {
  ROCKET_LOG(toy);

  ROCKET_LOG_INFO("hi {}", 5);

  string text =
      "oeiwoeiw oeipqowe pqwoiepqoeiqpwoei qoeiq pwoeiqpoiqwpei poeqpoiepei qpweopwqei qpeipoqe qpeiqpwe\n"
      "udiuwieoqwoieuqwuewqieu iqwueqowi euqoweu qwioeuwo qe eu qowieu qieuiqowue qiweuwo eueu ioeu\n"
      "asdf";
  ROCKET_LOG_TRACE("{}", text);

  try {
    throw InvalidState("oops");
  } catch (const InvalidState& ex) {
    fmt::print("Here is an ex: ");
    printException(nio::stdout, ex);
    throw;
  }
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

int
main(int argc, char **argv) {
  try {
    ROCKET_PROCESS_ERROR("Test process error");

    process.init(argc, argv, "toy");

    cl::CommandLine cl;
    vector<string> args;
    try {
      args = cl.parse(process.args());
    } catch (const exception& ex) {
      cl.handleException(ex, nio::stderr);
    }

    {
      ROCKET_LOG(toy);
      ROCKET_LOG_INFO("hey {}", "there");
      auto& out = nio::stdout;
      out.println("This is {}", process.name());
      out.println("args: {}", args);
      toy();
    }

    process.exit(EXIT_SUCCESS);
  } catch (...) {
    terminate();
  }
}

// EOF
