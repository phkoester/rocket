/*
 * main.cc
 *
 * The `toy` test executable links to Rocket and is a playground for quick and dirty experiments.
 */

#include "rocket/codec/codec.h"
#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/log/log.h"
#include "rocket/reflect/reflect.h"

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

using rocket::codec::ValueType;
using rocket::codec::ValueTypes;

ROCKET_LOG_DEFINE(thisIsARatherLongLogId);
ROCKET_LOG_DEFINE(toy);

// Variables -----------------------------------------------------------------------------------------------

auto& out = nio::out;
auto& err = nio::err;

// Bla ------------------------------------------------------------------------------------------------------

namespace rocket::reflect {

template<typename T>
struct MemberRefProvider : false_type {};

} // namespace rocket::reflect

struct MyType {
  i32 a;
  string b;
  bool c;

  ROCKET_REFLECT_MEMBERS(MyType, index, (a)(b)(c));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyType, index);
ROCKET_REFLECT_MEMBERS_DEFINE(, MyType, index);

namespace rocket::reflect {

template<>
struct MemberRefProvider<MyType> : true_type {
  static constexpr auto& refs = MyType::index();
};

} // namespace rocket::reflect

static_assert(rocket::reflect::MemberRefProvider<MyType>::value, "MyType must provide a member-reference container");

// Functions ------------------------------------------------------------------------------------------------

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
toy() {
  ROCKET_LOG(toy);

  ROCKET_LOG_TRACE("Hey {}", "there");
}

// #main ----------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  ROCKET_PROCESS_ERROR(0, "Testing error before `process.init` ...");

  Process::atExit(myExit);
  Process::atExit(myTerminate, true);

  process.init(argc, argv, "toy");

  optional<bool> foo;
  optional<bool> help;
  optional<vector<string>> args;

  const cl::OptionGroup general("General control");
  const cl::CommandLineConfig config { .usages={ "[OPTION]... [ARG]..." }} ;
  cl::CommandLine cl({
    cl::Option::helpOf(&general, help),
    cl::Option::of(&general, "foo", "f"_c, nullopt, "delve into foo mode", foo),
  }, {
    cl::Parameter::of("ARG", nullopt, "a command-line argument", args)
  }, config);

  cl.parse(process.args());

  {
    ROCKET_LOG(toy);
    ROCKET_LOG_INFO("Hey {}", "there");
    out.println("This is {}", process.name());
    out.println("args: {}", args);
    toy();
  }

  out.println("Exiting ...");
  process.exit(EXIT_SUCCESS);
}

// EOF
