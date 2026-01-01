/**
 * @file unicode-decl.h
 *
 * A Unicode API: declarations.
 */

// XXX File weg?

 #pragma once

#include <iosfwd>
#include <string>

// `rocket::unicode` ----------------------------------------------------------------------------------------

namespace rocket::unicode {

struct CodePoint;

struct Grapheme;

std::u32string utf8To32(std::string_view);

std::string utf32To8(std::u32string_view);

} // namespace rocket::unicode;

// EOF
