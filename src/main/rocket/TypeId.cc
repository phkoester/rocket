/*
 * TypeId.cc
 */

#include "TypeId.h"

#include <cstdlib>
#include <cxxabi.h>
#include <memory>

using namespace std;

namespace rocket {

// `TypeId` -------------------------------------------------------------------------------------------------

string
TypeId::readableName() const {
  const char* const name = type_.name();
  i32 status = 0;
  const unique_ptr<char, void (*)(void*)> ptr {
    abi::__cxa_demangle(name, nullptr, nullptr, &status),
    free
  };
  return status == 0 ? ptr.get() : name;
}

} // namespace rocket

// EOF
