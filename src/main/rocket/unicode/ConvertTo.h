/**
 * @file ConvertTo.h
 *
 * Templatized string conversions.
 */

#pragma once

#include "rocket/rocket.h"

#include <string>

namespace rocket::unicode {

std::u32string utf8To32(std::string_view s);

std::string utf32To8(std::u32string_view s);

// `ConvertTo` ----------------------------------------------------------------------------------------------

template<typename C> requires Character<C>
struct ConvertTo;

template<>
struct ConvertTo<char> {
  inline std::string_view apply(std::string_view s) const { return s; }

  inline std::string apply(std::u32string_view s) const { return utf32To8(s); }
};

template<>
struct ConvertTo<char32_t> {
  inline std::u32string_view apply(std::u32string_view s) const { return s; }

  inline std::u32string apply(std::string_view s) const { return utf8To32(s); }
};

} // namespace rocket::unicode

// EOF
