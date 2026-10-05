/**
 * @file ConvertTo.h
 *
 * Templatized string conversions.
 */

#pragma once

#include "rocket/rocket.h"
#include "rocket/type-traits.h"
#include "rocket/unicode/unicode-fwd.h"

#include <string>

namespace rocket::unicode {

// `ConvertTo` ----------------------------------------------------------------------------------------------

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
  [[nodiscard]] static std::string apply(std::u32string_view str) { return convertUtf32To8(str); }
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
  [[nodiscard]] static std::u32string apply(std::string_view str) { return convertUtf8To32(str); }
};

} // namespace rocket::unicode

// EOF
