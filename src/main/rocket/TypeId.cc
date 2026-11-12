/*
 * TypeId.cc
 */

#include "TypeId.h"

#include "rocket/str/str.h"

#include <cctype>
#include <cstdlib>
#include <memory>
#include <string_view>

#ifndef ROCKET_OS_WINDOWS
  #include <cxxabi.h>
#endif

using namespace std;

namespace rocket {

// `TypeId` -------------------------------------------------------------------------------------------------

string
TypeId::prettyName() const {
  const char* const name = type_.name();
#ifdef ROCKET_OS_WINDOWS
  // Windows: the names are not mangled, but still need some sanitizing
  string ret(name);
  str::replaceIn(ret, {
    { "class ", "" },
    { "enum ", "" },
    { "struct ", "" },
    { " * __ptr64", "*" },
    { ",", ", " }
  });
  // Replace "> >" with ">>" in a loop
  while (str::replaceIn(ret, { { "> >", ">>" } }) > 0);
  return ret;
#else
  // Linux, macOS
  i32 status = 0;
  const unique_ptr<char, void (*)(void*)> ptr {
    abi::__cxa_demangle(name, nullptr, nullptr, &status),
    free
  };
  return status == 0 ? ptr.get() : name;
#endif
}

} // namespace rocket

// EOF
