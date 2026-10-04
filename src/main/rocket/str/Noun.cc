/*
 * Noun.cc
 */

#include "Noun.h"

#include <fmt/format.h>

using namespace std;

namespace rocket::str {

// `Noun` ---------------------------------------------------------------------------------------------------

string
Noun::amount(i64 count) const {
  return fmt::format("{} {}", count, operator()(count));
}

// Predefined nouns -----------------------------------------------------------------------------------------

namespace noun {

ROCKET_PUBLIC const Noun byte { "byte", "bytes" };
ROCKET_PUBLIC const Noun character { "character", "characters" };

} // namespace noun

} // namespace rocket::str

// EOF
