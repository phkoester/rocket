/*
 * quote.cc
 */

#include "quote.h"

#include "escape.h"

using namespace std;

namespace rocket::quote {

string
bt(string_view s) {
  ostringstream os;
  string copy(s);
  os << escape::escaped<escape::CString>(copy);
  return '`' + os.str() + '`';
}

string
cd(string_view s) {
  ostringstream os;
  string copy(s);
  os << escape::escaped<escape::CString>(copy);
  return "“" + os.str() + "”";
}

string
cs(string_view s) {
  ostringstream os;
  string copy(s);
  os << escape::escaped<escape::CString>(copy);
  return "‘" + os.str() + "’";
}

string
sd(string_view s) {
  ostringstream os;
  string copy(s);
  os << escape::escaped<escape::CString>(copy, { .enclosed=true, .quote='"' });
  return os.str();
}

string
ss(string_view s) {
  ostringstream os;
  string copy(s);
  os << escape::escaped<escape::CString>(copy, { .enclosed=true, .quote='\'' });
  return os.str();
}

} // namespace rocket::quote

// EOF
