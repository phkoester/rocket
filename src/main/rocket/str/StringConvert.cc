/*
 * StringConvert.cc
 */

#include "StringConvert.h"

#include <algorithm>
#include <set>

using namespace std;

// Local constants ------------------------------------------------------------------------------------------

namespace {

const set<string> LOWER_FALSE_VALUES = {
  "false", "nan", "nil", "no", "null", "none", "off", "undefined"
};

} // namespace

namespace rocket::str::internal {

// Functions ------------------------------------------------------------------------------------------------

bool
StringConvert<bool>::isFalse(string_view str) {
  if (str.empty() || str == "0") {
    return true;
  }
  string lower(str);
  ranges::transform(lower, lower.begin(), [](char c) { return tolower(c); });
  return LOWER_FALSE_VALUES.contains(lower);
}

} // namespace rocket::str::internal

// EOF
