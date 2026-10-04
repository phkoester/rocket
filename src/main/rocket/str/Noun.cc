/*
 * Noun.cc
 */

#include "Noun.h"

#include <fmt/format.h>

using namespace std;

namespace rocket::str {

// `Noun` ---------------------------------------------------------------------------------------------------

const Noun Noun::byte { "byte", "bytes" };
const Noun Noun::character { "character", "characters" };

string
Noun::amount(i64 count) const {
  return fmt::format("{} {}", count, operator()(count));
}

} // namespace rocket::str

// EOF
