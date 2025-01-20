/**
 * @file unicode-decl.h
 *
 * A Unicode API: declarations.
 */

#pragma once

#include <iosfwd>
#include <string>

// `std` ----------------------------------------------------------------------------------------------------

namespace std {

/// A `u32istream` type.
using u32istream = basic_istream<char32_t>;
/// A `u32ostream` type.
using u32ostream = basic_ostream<char32_t>;

/// A `u32spanstream` type.
using u32spanstream = basic_spanstream<char32_t>;
/// A `u32ispanstream` type.
using u32ispanstream = basic_ispanstream<char32_t>;
/// A `u32ospanstream` type.
using u32ospanstream = basic_ospanstream<char32_t>;

/// A `u32stringstream` type.
using u32stringstream = basic_stringstream<char32_t>;
/// A `u32istringstream` type.
using u32istringstream = basic_istringstream<char32_t>;
/// A `u32ostringstream` type.
using u32ostringstream = basic_ostringstream<char32_t>;

} // namespace std

// `rocket::unicode` ----------------------------------------------------------------------------------------

namespace rocket::unicode {

struct CodePoint;

struct Grapheme;

std::u32string utf8To32(std::string_view);

std::string utf32To8(std::u32string_view);

} // namespace rocket::unicode;

// EOF
