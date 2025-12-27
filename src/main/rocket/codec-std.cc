/*
 * codec-std.cc
 */

#include "codec-std-decl.h"
#include "codec-std.h"

#include "escape.h"

using namespace rocket;
using namespace rocket::codec;
using namespace std;

namespace std {

// Functions ------------------------------------------------------------------------------------------------

istream&
parseRon(istream& is, byte& v) {
  return ::parseRon(is, reinterpret_cast<unsigned char&>(v));
}

istream&
parseRon(istream& is, string& v) {
  // Skip
  ron::parsing::skip(is);

  // Unescape
  escape::CString::Params params { .enclosed=true, .quote='"' };
  string input;
  auto escaped = escape::escaped<escape::CString>(input, params);
  is >> escaped;
  v = input;
  return is;
}

istream&
parseRon(istream& is, u32string& v) {
  // Skip
  ron::parsing::skip(is);

  // Unescape
  escape::CString::Params params { .enclosed=true, .quote='"' };
  string input;
  auto escaped = escape::escaped<escape::CString>(input, params);
  is >> escaped;
  v = unicode::utf8To32(input);
  return is;
}

} // namespace std

// EOF
