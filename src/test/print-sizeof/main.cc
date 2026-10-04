/*
 * main.cc
 */

#include <rocket/Process.h>
#include <rocket/str/Noun.h>

#include <fmt/std.h> // `fmt::formatter<std::type_info>`

using namespace rocket;
using namespace std;

namespace {

// Local functions ------------------------------------------------------------------------------------------

template<typename T>
void printSize() {
  string typeName= fmt::format("{}", typeid(T));
  i32 size = sizeof(T);
  nio::out.println("{: <19} | {:>2} {}", typeName, size, str::noun::byte(sizeof(T)));
}

}

// `main` ---------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  process.init(argc, argv, "print-sizeof");

  printSize<bool>();
  printSize<wchar_t>();
  printSize<short>();
  printSize<int>();
  printSize<long>();
  printSize<long long>();
  printSize<i128>();
  printSize<float>();
  printSize<double>();
  printSize<long double>();
  printSize<void*>();

  process.exit(EXIT_SUCCESS);
}

// EOF
