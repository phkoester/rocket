/*
 * main.cc
 */

#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/log/log.h"

#include "rocket/literal.h"
#include "rocket/unicode/unicode.h"
#include "rocket/version.h"

#include <cstring>

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

ROCKET_LOG_DEFINE(thisIsARatherLongLogId);
ROCKET_LOG_DEFINE(toy);

// Variables -----------------------------------------------------------------------------------------------

auto& out = nio::out;
auto& err = nio::err;

// Functions -----------------------------------------------------------------------------------------------

extern const char* generated();

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
yyy() {
  ROCKET_LOG(thisIsARatherLongLogId);
  ROCKET_LOG_TRACE("Hey {}", "there");
}

void
zzz() {
  ROCKET_LOG(thisIsARatherLongLogId);
  for (int i = 0; i < 10; ++i) {
    yyy();
  }
}

template<typename T, typename U>
U
make(i32 val) {
  if constexpr (std::is_same_v<U, std::optional<T>>) {
    T tval = (T) val;
    return std::optional<T>(tval);
  } else {
    static_assert(std::is_same_v<U, T>);
    return (U) val;
  }
}

void
toy() {
  ROCKET_LOG(toy);
  ROCKET_LOG_TRACE("Hey {}", "there");
  out.println("src file name: {}", ROCKET_SRC_FILE);
  zzz();

  auto m1 = make<f32, optional<f32>>(0);
  out.println("m1: {}", m1);
  auto m2 = make<f32, f32>(0);
  out.println("m2: {}", m2);
}

// #main ----------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  ROCKET_PROCESS_ERROR(0, "Testing error before `process.init` ...");

  process.atExit(myExit);
  process.atExit(myTerminate, true);

  process.init(argc, argv, "toy");

  optional<bool> help;
  optional<bool> foo;
  optional<vector<string>> files;

  cl::OptionGroup general("General control");
  cl::CommandLineConfig config { .usages={ "[OPTION]... [FILE]..." }} ;
  cl::CommandLine cl({
    cl::Option::helpOf(&general, help),
    cl::Option::of(&general, "foo", "f"_c, nullopt, "delve into foo mode", foo),
  }, {
    cl::Parameter::of("FILE", "file", "an input file", files)
  }, config);

  cl.parse(process.args());

  {
    ROCKET_LOG(toy);
    ROCKET_LOG_INFO("Hey {}", "there");
    out.println("This is {}", process.name());
    out.println("{}", generated());
    out.println("files: {}", files);
    toy();
  }

  out.println("Exiting ...");
  process.exit(EXIT_SUCCESS);
}

// EOF
