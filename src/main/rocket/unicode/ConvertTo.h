/**
 * @file ConvertTo.h
 *
 * Templatized string conversions.
 */

#pragma once

#include "rocket/rocket.h"
#include "rocket/type-traits.h"

#include <string>

namespace rocket::unicode {

std::u32string utf8To32(std::string_view str); // NOLINT

std::string utf32To8(std::u32string_view str); // NOLINT

// #ConvertTo -----------------------------------------------------------------------------------------------

template<typename C> requires IsChar<C>
struct ConvertTo;

/**
 * Specialization for `char`.
 */
template<>
struct ConvertTo<char> {
  /**
   * Applies this converter to a string.
   *
   * @param str the string to convert
   * @return the converted string
   */
  [[nodiscard]] static std::string_view apply(std::string_view str) { return str; }

  /**
   * Applies this converter to a string.
   *
   * @param str the string to convert
   * @return the converted string
   */
  [[nodiscard]] static std::string apply(std::u32string_view str) { return utf32To8(str); }
};

/**
 * Specialization for `char32`.
 */
template<>
struct ConvertTo<char32> {
  /**
   * Applies this converter to a string.
   *
   * @param str the string to convert
   * @return the converted string
   */
  [[nodiscard]] static std::u32string_view apply(std::u32string_view str) { return str; }

  /**
   * Applies this converter to a string.
   *
   * @param str the string to convert
   * @return the converted string
   */
  [[nodiscard]] static std::u32string apply(std::string_view str) { return utf8To32(str); }
};

} // namespace rocket::unicode

// EOF
