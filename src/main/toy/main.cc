/*
 * main.cc
 */

#include "rocket/codec-std-decl.h"
#include "rocket/codec-std.h"

#include "rocket/Process.h"
#include "rocket/cl.h"
#include "rocket/log.h"
#include "rocket/macro.h"

#include <fmt/args.h>
#include <fmt/format.h>
#include <fmt/os.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

#include <chrono>
#include <vector>

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

using rocket::S;

ROCKET_LOG_DEFINE(toy);

// Macros ---------------------------------------------------------------------------------------------------

#define ROCKET_BENCH(name, n, f) { \
  using namespace std::chrono; \
  \
  auto t1 = steady_clock::now(); \
  for (size_t i = 0; i < n; ++i) { \
    f(); \
  } \
  auto t2 = steady_clock::now(); \
  auto ms = duration_cast<milliseconds>(t2 - t1); \
  auto ns = duration_cast<nanoseconds>(t2 - t1); \
  fmt::println("Bench {:?}: {:L} executions in {:L} ms ({:L} ns each)", name, n, ms.count(), ns.count() / n); \
}

// Local functions ------------------------------------------------------------------------------------------

namespace {

void
toy() {
  ROCKET_LOG(toy);

  const auto N = 3'000'000;

  {
    ostringstream oss;
    ROCKET_BENCH("fmt::print", N, [&] {
      fmt::print(oss, "{}", vector { 'h', 'e', 'l', 'l', 'o', '\n' });
    });
  }

  {
    ostringstream oss;
    ROCKET_BENCH("S", N, [&] {
      oss << (S << vector { 'h', 'e', 'l', 'l', 'o', '\n' }) << '\n';
    });
  }
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

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
