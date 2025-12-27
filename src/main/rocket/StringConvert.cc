/*
 * StringConvert.cc
 */

#include "StringConvert.h"
#include "codec.h"

using namespace std;

namespace rocket {

// `bool` ---------------------------------------------------------------------------------------------------

bool
StringConvert<bool>::stringToType(string_view s) const {
  auto is = io::is(s);
  Type ret = codec::getBool(is);
  if (is.fail() || io::tellg(is) != s.size()) {
    throw except::ParseFailure<char>(is, 0, { 0, s.size() },
        except::message::cannotParseAs(s, rocket::Type::of<Type>()));
  }
  return ret;
}

} // namespace rocket

// EOF
