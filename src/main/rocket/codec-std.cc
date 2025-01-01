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

ostream&
printRon(ostream& os, byte v) {
  return ::printRon(os, static_cast<unsigned char>(v));
}

istream&
parseRon(istream& is, string& v) {
  // Skip
  ron::parsing::skip(is);

  // Unescape
  escape::CString::Params params { .enclosed=true, .quote='"' };
  string input;
  is >> escape::escaped<escape::CString>(input, params);
  v = input;
  return is;
}

ostream&
printRon(ostream& os, const string& v) {
  ostringstream oss;
  oss << escape::escaped<escape::CString>(v, { .enclosed=true, .quote='"' });
  return os << oss.str();
}

ostream&
printRon(ostream& os, string_view v) {
  return printRon(os, string(v));
}

istream&
parseRon(istream& is, u32string& v) {
  // Skip
  ron::parsing::skip(is);

  // Unescape
  escape::CString::Params params { .enclosed=true, .quote='"' };
  string input;
  is >> escape::escaped<escape::CString>(input, params);
  v = unicode::utf8To32(input);
  return is;
}

ostream&
printRon(ostream& os, const u32string& v) {
  u32ostringstream oss;
  oss << escape::escaped<escape::CString>(v, { .enclosed=true, .quote='"' });
  return os << unicode::utf32To8(oss.str());
}

ostream&
printRon(ostream& os, u32string_view v) {
  return printRon(os, u32string(v));
}

} // namespace std

// EOF
