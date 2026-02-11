/*
 * main.cc
 *
 * The `toy` test executable links to Rocket and is a playground for quick and dirty experiments.
 */

#include "rocket/hash.h"
#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/log/log.h"

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

ROCKET_LOG_DEFINE(thisIsARatherLongLogId);
ROCKET_LOG_DEFINE(toy);

// #FieldType -----------------------------------------------------------------------------------------------

enum class FieldType {
  boolean,
  character,
  enumeration,
  integer,
  floatingPoint,
  pointer,
  string,
  optional,
  tuple,
  array,
  set,
  map
};

// #FieldTypes ----------------------------------------------------------------------------------------------

template<typename T>
struct FieldTypes;

template<>
struct FieldTypes<bool> {
  static constexpr auto value = FieldType::boolean;
};

template<typename C> requires IsChar<C>
struct FieldTypes<C> {
  static constexpr auto value = FieldType::character;
};

template<typename I> requires IsInteger<I>
struct FieldTypes<I> {
  static constexpr auto value = FieldType::integer;
};

template<typename T>
struct FieldTypes<std::optional<T>> {
  static constexpr auto value = FieldType::optional;
};

template<typename A, typename B>
struct FieldTypes<std::pair<A, B>> {
  static constexpr auto value = FieldType::tuple;
};

template<typename... T>
struct FieldTypes<std::tuple<T...>> {
  static constexpr auto value = FieldType::tuple;
};

template<typename T, u64 N>
struct FieldTypes<std::array<T, N>> {
  static constexpr auto value = FieldType::array;
};

template<typename T>
struct FieldTypes<std::vector<T>> {
  static constexpr auto value = FieldType::array;
};

// #Encoder -------------------------------------------------------------------------------------------------

template<typename Consumer>
struct Encoder {
  template<typename T, typename... Arg>
  auto encode(const T& val, auto&& args) {
    constexpr auto fieldType = FieldTypes<T>::value;
    using ConsumerType = Consumer::template Type<fieldType, T>;
    ConsumerType consumer;
    return consumer.consume(val, std::forward<decltype(args)>(args));
  }
};

// #HashConsumerImpl ----------------------------------------------------------------------------------------

template<FieldType, typename T>
struct HashConsumerImpl;

template<>
struct HashConsumerImpl<FieldType::boolean, bool> {
  u64 consume(bool val, nio::StringSink& out) {
    out.println("consuming boolean: {}", val);
    return std::hash<bool>()(val);
  }
};

template<typename I>
struct HashConsumerImpl<FieldType::integer, I> {
  u64 consume(I val, nio::StringSink& out) {
    out.println("consuming integer: {}", val);
    return std::hash<I>()(val);
  }
};

template<typename T>
struct HashConsumerImpl<FieldType::optional, T> {
  u64 consume(const T& val, nio::StringSink& out) {
    out.println("consuming optional: {}", val);
    if (not val) {
      return 0;
    }
    u64 ret = 1;
    using Elem = typename T::value_type;
    constexpr auto fieldType = FieldTypes<Elem>::value;
    u64 hash = HashConsumerImpl<fieldType, Elem>().consume(*val, out);
    combineHash(ret, hash);
    return ret;
  }
};

template<typename T>
struct HashConsumerImpl<FieldType::tuple, T> {
  u64 consume(const T& val, nio::StringSink& out) {
    const auto size = std::tuple_size<T>::value;
    out.println("consuming tuple: {}", size);
    u64 ret = size;
    std::apply([&](auto&&... arg) {
      (consumeElem(ret, std::forward<decltype(arg)>(arg), out), ...);
    }, val);
    return ret;
  }

private:

  template<typename Elem>
  u64 consumeElem(u64 seed, const Elem& elem, nio::StringSink& out) {
    out.println("consuming tuple elem: {}", elem);
    constexpr auto elemFieldType = FieldTypes<Elem>::value;
    u64 hash = HashConsumerImpl<elemFieldType, Elem>().consume(elem, out);
    combineHash(seed, hash);
    return seed;
  }
};

template<typename T>
struct HashConsumerImpl<FieldType::array, T> {
  u64 consume(const T& val, nio::StringSink& out) {
    const auto size = val.size();
    out.println("consuming array: {}", size);
    u64 ret = size;
    for (auto it = val.begin(), end = val.end(); it != end; ++it) {
      using Elem = typename T::value_type;
      constexpr auto elemFieldType = FieldTypes<Elem>::value;
      u64 hash = HashConsumerImpl<elemFieldType, Elem>().consume(*it, out);
      combineHash(ret, hash);
    }
    return ret;
  }
};

// #HashConsumer --------------------------------------------------------------------------------------------

struct HashConsumer {
  template<FieldType fieldType, typename T>
  using Type = HashConsumerImpl<fieldType, T>;
};

// Variables -----------------------------------------------------------------------------------------------

auto& out = nio::out;
auto& err = nio::err;

// Functions -----------------------------------------------------------------------------------------------

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

  Encoder<HashConsumer> encoder;

  nio::StringSink out;
  encoder.encode(true, out);
  optional<bool> optBool;
  encoder.encode(optBool, out);
  optBool = true;
  encoder.encode(optBool, out);
  encoder.encode(42, out);
  encoder.encode(optional<i32>(43), out);

  vector vec = { 1, 2, 3 };
  encoder.encode(vec, out);

  auto arr = array<i32, 3> { 5, 6, 7 };
  encoder.encode(arr, out);

  pair p = { true, -7 };
  encoder.encode(p, out);

  tuple t = { true, -7, 42 };
  encoder.encode(t, out);

  nio::out.println("encoded:\n{}", out.str());
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
