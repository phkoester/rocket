/*
 * Noun.cc
 */

#include "Noun.h"

#include "rocket/format.h"

using namespace std;

namespace rocket::str {

// `Noun` ---------------------------------------------------------------------------------------------------

string
Noun::amount(i64 n) const {
  return fmt::format("{} {}", n, operator()(n));
}

// Predefined nouns -----------------------------------------------------------------------------------------

namespace noun {

ROCKET_PUBLIC const Noun byte { "byte", "bytes" };
ROCKET_PUBLIC const Noun character { "character", "characters" };
ROCKET_PUBLIC const Noun line { "line", "lines" };

} // namespace noun

} // namespace rocket::str

// EOF
