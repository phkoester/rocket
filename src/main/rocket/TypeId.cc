/*
 * TypeId.cc
 */

#include "TypeId.h"

#include <cstdlib>
#include <memory>

#ifndef ROCKET_OS_WINDOWS
  #include <cxxabi.h>
#endif

using namespace std;

namespace rocket {

// `TypeId` -------------------------------------------------------------------------------------------------

string
TypeId::readableName() const {
  const char* const name = type_.name();
#ifdef ROCKET_OS_WINDOWS
  return name;
#else
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
