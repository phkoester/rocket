/*
 * main.cc
 */

#include <rocket/Process.h>
#include <rocket/TypeId.h>
#include <rocket/str/Noun.h>
#include <rocket/format.h>

using namespace rocket;
using namespace std;

namespace {

// Local constants ------------------------------------------------------------------------------------------

constexpr u32 LEFT_COLUMN_WIDTH = 23;

// Local functions ------------------------------------------------------------------------------------------

template<typename T>
void printSize() {
  const string name = fmt::format("{}", TypeId::of<T>());
  nio::out.println("{: <{}} | {:>2} {}", name, LEFT_COLUMN_WIDTH, sizeof(T), str::noun::byte(sizeof(T)));
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  process().init(argc, argv, "print-sizeof");

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

  process().exit(EXIT_SUCCESS);
}

// EOF
