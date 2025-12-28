/*
 * enum.cc
 */

#include "enum.h"

using namespace std;

// Internal -------------------------------------------------------------------------------------------------

namespace rocket::_enum::internal {

string
getEnumString(istream& is, const std::set<std::string_view>& values) {
  cout << "LEBENSZEICHEN!\n";
  return "hello from enum";
}

} // namespace rocket::_enum::internal

// EOF
