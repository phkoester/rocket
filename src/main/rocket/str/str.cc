/*
 * str.cc
 */

#include "str.h"

#include "rocket/unicode/unicode.h"
#include "rocket/unicode/iterator.h"

#include <algorithm>

using namespace rocket;
using namespace std;

namespace rocket::str {

// Functions ------------------------------------------------------------------------------------------------

string
capitalize(string_view s) {
  if (s.empty())
    return string();
  unicode::CodePointIterator<char> it(s);
  unicode::CodePoint upper = (*it++).upper();
  string ret = static_cast<string>(upper);
  ret.append(s.substr(it.position()));
  return ret;
}

u32string
capitalize(u32string_view s) {
  if (s.empty())
    return u32string();
  u32string ret(s);
  ret[0] = unicode::CodePoint(s[0]).upper();
  return ret;
}

string
lower(string_view s) {
  u32string localS = unicode::utf8To32(s);
  lowerIn(localS);
  return unicode::utf32To8(localS);
}

u32string
lower(u32string_view s) {
  u32string ret(s);
  lowerIn(ret);
  return ret;
}

void
lowerIn(u32string& s) {
  transform(s.begin(), s.end(), s.begin(), [](char32_t c) { return unicode::CodePoint(c).lower(); });
}

string
upper(string_view s) {
  u32string localS = unicode::utf8To32(s);
  upperIn(localS);
  return unicode::utf32To8(localS);
}

u32string
upper(u32string_view s) {
  u32string ret(s);
  upperIn(ret);
  return ret;
}

void
upperIn(u32string& s) {
  transform(s.begin(), s.end(), s.begin(), [](char32_t c) { return unicode::CodePoint(c).upper(); });
}

} // namespace rocket::str

// EOF
