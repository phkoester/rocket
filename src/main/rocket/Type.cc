/*
 * Type.cc
 */

#include "Type.h"

#include <cxxabi.h>
#include <memory>

using namespace rocket;
using namespace std;

namespace {

// Local functions ------------------------------------------------------------------------------------------

string
typeInfoName(const type_info& info) {
  int status = 0;
  unique_ptr<char, decltype(free)*> p(abi::__cxa_demangle(info.name(), nullptr, nullptr, &status), free);
  if (p && status == 0) {
    string ret = p.get();
    // Eliminate spaces before '>'
    str::replaceIn<char>(ret, " >", ">");
    return ret;
  } else
    return info.name();
}

} // namespace

namespace rocket {

// `Type` ---------------------------------------------------------------------------------------------------

Type::Type(const type_info& info) :
    info_(info),
    name_([&] { return typeInfoName(info_); }),
    index_(info),
    hash_([&] { return index_.hash_code(); }) {}

} // namespace rocket

// EOF
