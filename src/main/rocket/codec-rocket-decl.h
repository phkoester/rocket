/**
 * @file codec-rocket-decl.h
 *
 * Rocket codec: declarations.
 */

#pragma once

#include "enum-decl.h"
#include "math.h" // XXX
#include "text.h" // XXX
#include "unicode-decl.h"

#ifdef ROCKET_CODEC_H
#error `codec.h` must be included after this file
#endif

namespace rocket {

// Functions ------------------------------------------------------------------------------------------------

namespace text {

/// @enum_declare{#rocket::text::Position::Type}
ROCKET_ENUM_DECLARE(Position::Type);

} // namespace text

namespace unicode {

/// @fn_parseRon{#rocket::unicode::CodePoint}
std::istream& parseRon(std::istream& is, CodePoint& v);

/// @fn_parseRon{#rocket::unicode::Grapheme}
std::istream& parseRon(std::istream& is, Grapheme& v);

} // namespace text

// Template declarations ------------------------------------------------------------------------------------

namespace math {

/// @cond undocumented

template<typename T, typename Left, typename Right>
std::istream& parseRon(std::istream&, IntervalImpl<T, Left, Right>&);

/// @endcond

} // namespace math

} // namespace rocket

// EOF
