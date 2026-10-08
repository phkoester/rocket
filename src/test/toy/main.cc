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

// Functions ------------------------------------------------------------------------------------------------

void
populate_map() {
  /* Preallocate the map structure */
  cbor_item_t* root = cbor_new_definite_map(2);
  /* Add the content */
  bool success = cbor_map_add(
      root, (struct cbor_pair){
                .key = cbor_move(cbor_build_string("Is CBOR awesome?")),
                .value = cbor_move(cbor_build_bool(true))});
  success &= cbor_map_add(
      root, (struct cbor_pair){
                .key = cbor_move(cbor_build_uint8(42)),
                .value = cbor_move(cbor_build_string("Is the answer"))});
  if (!success) return;
  /* Output: `length` bytes of data in the `buffer` */
  unsigned char* buffer;
  size_t buffer_size;
  cbor_serialize_alloc(root, &buffer, &buffer_size);

  fwrite(buffer, 1, buffer_size, stdout);
  free(buffer);

  fflush(stdout);
  cbor_decref(&root);
}

void item_examples() {
  // A cbor_item_t can contain any CBOR data type
  cbor_item_t* float_item = cbor_build_float4(3.14f);
  cbor_item_t* string_item = cbor_build_string("Hello World!");
  cbor_item_t* array_item = cbor_new_indefinite_array();

  // They can be inspected
  assert(cbor_is_float(float_item));
  assert(cbor_typeof(string_item) == CBOR_TYPE_STRING);
  assert(cbor_array_is_indefinite(array_item));
  assert(cbor_array_size(array_item) == 0);

  // The data can be accessed
  assert(cbor_float_get_float4(float_item) == 3.14f);
  assert(memcmp(cbor_string_handle(string_item), "Hello World!",
                cbor_string_length(string_item)) == 0);

  // And they can be modified
  assert(cbor_array_push(array_item, float_item));
  assert(cbor_array_push(array_item, string_item));
  assert(cbor_array_size(array_item) == 2);

  // At the end of their lifetime, items must be freed
  cbor_decref(&float_item);
    cbor_decref(&string_item);
  cbor_decref(&array_item);
}
// Part 1: End

// Part 2: Begin
void encode_decode() {
  cbor_item_t* item = cbor_build_uint8(42);

  // Serialize the item to a buffer (it will be allocated by libcbor)
  unsigned char* buffer;
  size_t buffer_size;
  cbor_serialize_alloc(item, &buffer, &buffer_size);
  assert(buffer_size == 2);
  assert(buffer[0] == 0x18);  // Encoding byte for uint8
  assert(buffer[1] == 42);    // The value itself

  // And deserialize bytes back to an item
  struct cbor_load_result result;
  cbor_item_t* decoded_item = cbor_load(buffer, buffer_size, &result);
  assert(result.error.code == CBOR_ERR_NONE);
  assert(cbor_isa_uint(decoded_item));
  assert(cbor_get_uint8(decoded_item) == 42);

  // Free the allocated buffer and items
  free(buffer);
  cbor_decref(&decoded_item);
  cbor_decref(&item);
}
// Part 2: End

// Part 3: Begin
void reference_counting() {
  // cbor_item_t is a reference counted pointer under the hood
  cbor_item_t* item = cbor_build_uint8(42);

  // Reference count starts at 1
  assert(cbor_refcount(item) == 1);

  // Most operations have reference semantics
  cbor_item_t* array_item = cbor_new_definite_array(1);
  assert(cbor_array_push(array_item, item));
  assert(cbor_refcount(item) == 2);  // item and array_item reference it
  cbor_item_t* first_array_element = cbor_array_get(array_item, 0);
  assert(first_array_element == item);  // same item under the hood
  assert(cbor_refcount(item) ==
         3);  // and now first_array_element also points to it

  // To release the reference, use cbor_decref
  cbor_decref(&first_array_element);

  // When reference count reaches 0, the item is freed
  assert(cbor_refcount(array_item) == 1);
  cbor_decref(&array_item);
  assert(array_item == NULL);
  assert(cbor_refcount(item) == 1);

  // Be careful, loops leak memory!

  // Deep copy copies the whole item tree
  cbor_item_t* item_copy = cbor_copy(item);
  assert(cbor_refcount(item) == 1);
  assert(cbor_refcount(item_copy) == 1);
  assert(item_copy != item);
  cbor_decref(&item);
  cbor_decref(&item_copy);
}
// Part 3: End

// Part 4: Begin
void moving_values() {
  {
    // Move the "42" into an array.
    cbor_item_t* array_item = cbor_new_definite_array(1);
    // The line below leaks memory!
    assert(cbor_array_push(array_item, cbor_build_uint8(42)));
    cbor_item_t* first_array_element = cbor_array_get(array_item, 0);
    assert(cbor_refcount(first_array_element) == 3);  // Should be 2!
    cbor_decref(&first_array_element);
    cbor_decref(&array_item);
    assert(cbor_refcount(first_array_element) == 1);  // Shouldn't exist!
    // Clean up
    cbor_decref(&first_array_element);
  }

  {
    // A correct way to move values is to decref them in the caller scope.
    cbor_item_t* array_item = cbor_new_definite_array(1);
    cbor_item_t* item = cbor_build_uint8(42);
    assert(cbor_array_push(array_item, item));
    assert(cbor_refcount(item) == 2);
    // "Give up" the item
    cbor_decref(&item);
    cbor_decref(&array_item);
    // item is a dangling pointer at this point
  }

  {
    // cbor_move avoids the need to decref and the dangling pointer
    cbor_item_t* array_item = cbor_new_definite_array(1);
    assert(cbor_array_push(array_item, cbor_move(cbor_build_uint8(42))));
    cbor_item_t* first_array_element = cbor_array_get(array_item, 0);
    assert(cbor_refcount(first_array_element) == 2);
    cbor_decref(&first_array_element);
    cbor_decref(&array_item);
  }
}
// Part 4: End

// Part 5: Begin
// Refcount can be managed in conjunction with ownership
static cbor_item_t* global_item = NULL;

// This function takes shared ownership of the item
void borrow_item(cbor_item_t* item) {
  global_item = item;
  // Mark the extra reference
  cbor_incref(item);
}

void return_item() {
  cbor_decref(&global_item);
  global_item = NULL;
}

void reference_ownership() {
  cbor_item_t* item = cbor_build_uint8(42);

  // Lend the item
  borrow_item(item);
  assert(cbor_refcount(item) == 2);
  cbor_decref(&item);

  // Release the shared ownership. return_item will deallocate the item.
  return_item();
}
// Part 5: End

void mine() {
  nio::out.println("mine");

  cbor_item_t* array_item = cbor_new_definite_array(2);
  ROCKET_ASSERT(cbor_array_push(array_item, cbor_move(cbor_build_float4(3.14f))));
  ROCKET_ASSERT(cbor_array_push(array_item, cbor_move(cbor_build_float4(3.14f))));
  cbor_decref(&array_item);
}

namespace {

// Local variables ------------------------------------------------------------------------------------------

auto& out = nio::out;

// Local functions ------------------------------------------------------------------------------------------

bool
doTheCbor() {
  nio::out.println("Doing the CBOR ...");

#if 0
  item_examples();
  encode_decode();
  reference_counting();
  moving_values();
  reference_ownership();
#endif
  mine();

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
