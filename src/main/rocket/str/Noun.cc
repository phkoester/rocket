/*
 * Noun.cc
 */

#include "Noun.h"

#include "rocket/format.h"

using namespace std;

namespace rocket::str {

// `Noun` ---------------------------------------------------------------------------------------------------

string
Noun::amount(i64 val) const {
  return fmt::format("{} {}", val, operator()(val));
}

// Predefined nouns -----------------------------------------------------------------------------------------

namespace noun {

ROCKET_PUBLIC const Noun byte { "byte", "bytes" };
ROCKET_PUBLIC const Noun character { "character", "characters" };

} // namespace noun

} // namespace rocket::str

// EOF
