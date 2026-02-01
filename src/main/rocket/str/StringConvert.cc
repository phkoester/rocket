/*
 * StringConvert.cc
 */

#include "str.h"
#include "StringConvert.h"

#include <algorithm>
#include <set>

using namespace std;

// Local constants ------------------------------------------------------------------------------------------

namespace {

const set<string> FALSE_VALUES = {
  "false", "nan", "nil", "no", "null", "none", "off", "undefined"
};

} // namespace

namespace rocket::str {

// Functions ------------------------------------------------------------------------------------------------

bool
StringConvert<bool>::isFalse(string_view str) {
  if (str.empty() || str == "0") {
    return true;
  }
  string lower(str);
  transform(lower.begin(), lower.end(), lower.begin(), [](char c) { return tolower(c); });
  return FALSE_VALUES.contains(lower);
}

} // namespace rocket::str

// EOF
