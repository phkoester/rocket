/*
 * strings.cc
 */

#include "strings.h"

#include "rocket/unicode.h"
#include "unicode.h"
#include "unicode-iterator.h"

#include <algorithm>

using namespace rocket;
using namespace rocket::strings;
using namespace std;

namespace rocket::strings {

// Functions ------------------------------------------------------------------------------------------------

string
capitalize(string_view s) {
  if (s.empty())
    return string();
  unicode::CodePointIterator<char> it(s);
  unicode::CodePoint upper = (*it++).upper();
  string result = static_cast<string>(upper);
  result.append(s.substr(it.position()));
  return result;
}

u32string
capitalize(u32string_view s) {
  if (s.empty())
    return u32string();
  u32string result(s);
  result[0] = unicode::CodePoint(s[0]).upper();
  return result;
}

string
lower(string_view s) {
  u32string localS = unicode::utf8To32(s);
  lowerIn(localS);
  return unicode::utf32To8(localS);
}

u32string
lower(u32string_view s) {
  u32string result(s);
  lowerIn(result);
  return result;
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
  u32string result(s);
  upperIn(result);
  return result;
}

void
upperIn(u32string& s) {
  transform(s.begin(), s.end(), s.begin(), [](char32_t c) { return unicode::CodePoint(c).upper(); });
}

} // namespace rocket::strings

// EOF
