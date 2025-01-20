/**
 * @file codec-rocket-decl.h
 *
 * Rocket codec: declarations.
 */

#pragma once

#include "Type.h"
#include "enum-decl.h"
#include "math.h"
#include "text.h"
#include "unicode-decl.h"

#ifdef ROCKET_CODEC_H
#error `codec.h` must be included after this file
#endif

namespace rocket {

// Functions ------------------------------------------------------------------------------------------------

/// @fn_printRon{#rocket::Type}
std::ostream& printRon(std::ostream& os, const Type& v);

namespace text {

/// @enum_declare{#rocket::text::Position::Type}
ROCKET_ENUM_DECLARE(Position::Type);

} // namespace text

namespace unicode {

/// @fn_parseRon{#rocket::unicode::CodePoint}
std::istream& parseRon(std::istream& is, CodePoint& v);

/// @fn_printRon{#rocket::unicode::CodePoint}
std::ostream& printRon(std::ostream& os, CodePoint v);

/// @fn_parseRon{#rocket::unicode::Grapheme}
std::istream& parseRon(std::istream& is, Grapheme& v);

/// @fn_printRon{#rocket::unicode::Grapheme}
std::ostream& printRon(std::ostream& os, const Grapheme& v);

} // namespace text

// Template declarations ------------------------------------------------------------------------------------

namespace math {

/// @cond undocumented

template<typename T, typename Left, typename Right>
std::istream& parseRon(std::istream&, IntervalImpl<T, Left, Right>&);

template<typename T, typename Left, typename Right>
std::ostream& printRon(std::ostream&, const IntervalImpl<T, Left, Right>&);

/// @endcond

} // namespace math

} // namespace rocket

// EOF
