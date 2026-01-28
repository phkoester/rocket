/**
 * @file Noun.h
 *
 * Nouns with singular and plural forms.
 */

#pragma once

#include "rocket/rocket.h"

#include <string_view>

namespace rocket::str {

// #Noun ----------------------------------------------------------------------------------------------------

/**
 * A noun that knows its singular and plural form, in US English.
 */
struct Noun {
  /**
   * The singular form.
   */
  std::string_view singular;
  /**
   * The plural form.
   */
  std::string_view plural;

  /**
   * Text expansion function.
   *
   * @param count the amount
   * @return if @p count is 1, the singular, otherwise the plural
   */
  std::string_view
  operator()(u64 count) const {
    return count == 1 ? singular : plural;
  }
};

// Constants ------------------------------------------------------------------------------------------------

/**
 * A noun.
 */
ROCKET_PUBLIC extern const Noun character;

} // namespace rocket::str

// EOF
