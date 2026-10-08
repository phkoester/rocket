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

#include <cbor.h>

#include <iostream>

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

ROCKET_LOG_DEFINE(thisIsARatherLongLogId);
ROCKET_LOG_DEFINE(toy);

#define COPYRIGHT "Copyright © 2024–2026 Philip Köster"

// `Color` --------------------------------------------------------------------------------------------------

enum class Color : u8 { Red, Green, Blue };

ROCKET_ENUM_DECLARE(, Color, Color); // NOLINT(*-internal-linkage)
ROCKET_ENUM_DEFINE(, Color, Color, (Red)(Green)(Blue));

namespace {

// Local variables ------------------------------------------------------------------------------------------

auto& out = nio::out;

// Local functions ------------------------------------------------------------------------------------------

bool
doTheCbor() {
  nio::out.println("Doing the CBOR ...");

  /* Preallocate the map structure */
  cbor_item_t* root = cbor_new_definite_map(2);

  /* Add the content */
  bool success = cbor_map_add(root, (struct cbor_pair) {
    .key = cbor_move(cbor_build_string("Is CBOR awesome?")),
    .value = cbor_move(cbor_build_bool(true))});
  success &= cbor_map_add(root, (struct cbor_pair) {
    .key = cbor_move(cbor_build_uint8(42)),
    .value = cbor_move(cbor_build_string("Is the answer"))});
  if (not success) {
    return false;
  }

  /* Output: `length` bytes of data in the `buffer` */
  unsigned char* buffer;
  size_t buffer_size;
  cbor_serialize_alloc(root, &buffer, &buffer_size);

  cout << "Buffer size: " << buffer_size << endl;

  /* Raw bytes */
  cout << "Buffer:";
  for (size_t i = 0; i < buffer_size; ++i) {
    cout << fmt::format(" {:02x}", buffer[i]);
  }
  cout << endl;

  /* Decode the buffer and walk the map */
  auto describe = [](const cbor_item_t* item) -> string {
    if (cbor_isa_uint(item)) {
      return fmt::format("uint {}", cbor_get_int(item));
    }
    if (cbor_isa_string(item)) {
      return fmt::format("string \"{}\"",
          string_view(reinterpret_cast<const char*>(cbor_string_handle(item)), cbor_string_length(item)));
    }
    if (cbor_is_bool(item)) {
      return fmt::format("bool {}", cbor_get_bool(item));
    }
    return fmt::format("<unhandled type {}>", static_cast<int>(cbor_typeof(item)));
  };

  cbor_load_result result;
  cbor_item_t* loaded = cbor_load(buffer, buffer_size, &result);
  if (loaded == nullptr) {
    cout << "cbor_load failed, error code " << result.error.code << " at position " << result.error.position << endl;
    free(buffer);
    cbor_decref(&root);
    return false;
  }
  cout << "Read " << result.read << " bytes" << endl;

  if (not cbor_isa_map(loaded)) {
    cout << "Top-level item is not a map" << endl;
  } else {
    const size_t size = cbor_map_size(loaded);
    cout << "Map with " << size << " pair(s):" << endl;
    const cbor_pair* pairs = cbor_map_handle(loaded);
    for (size_t i = 0; i < size; ++i) {
      cout << "  [" << i << "] key = " << describe(pairs[i].key) << ", value = " << describe(pairs[i].value) << endl;
    }
  }

  cbor_decref(&loaded);
  free(buffer);

  cbor_decref(&root);

  nio::out.println("Done the CBOR");

  return true;
}

void
myExit() {
  out.println("myExit");
  // ROCKET_FAIL("Oopsers!");
}

void
myTerminate() {
  // out.println("myTerminate");
}

recursive_mutex toyMutex;

void
toy() {
  using namespace std::chrono;

  ROCKET_LOG(toy);

  nio::out.println("tellg signed: {}", std::is_signed_v<std::ios::pos_type>);
  nio::out.println("tellg unsigned: {}", std::is_unsigned_v<std::ios::pos_type>);

  ROCKET_LOG_TRACE("Hey {}", "there");

  ROCKET_MUTEX_LOCK(toyMutex);
  ROCKET_MUTEX_LOCK(toyMutex);
  out.println("Got lock");

  bool result = doTheCbor();
  nio::out.println("doTheCbor result: {}", result);
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

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
