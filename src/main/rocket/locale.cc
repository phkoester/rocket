/*
 * locale.cc
 */

#include "locale.h"

#include "ctype-char32_t.h" // `ctype<char32_t>`

using namespace std;

// Functions ------------------------------------------------------------------------------------------------

namespace rocket::locale {

std::locale
withChar32Facets(const std::locale& v) {
  if (has_facet<ctype<char32_t>>(v)) {
   return v;
  } else {
    return v <<
        new ctype<char32_t> <<
        new numpunct<char32_t> <<
        new num_get<char32_t> <<
        new num_put<char32_t>;
  }
}

} // namespace rocket::locale

// EOF
