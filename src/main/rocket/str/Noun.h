/**
 * @file Noun.h
 *
 * Nouns with singular and plural forms.
 */

#pragma once

#include "rocket/rocket.h"

#include <string>

namespace rocket::str {

// `Noun` ---------------------------------------------------------------------------------------------------

/**
 * A noun that knows its singular and plural form, in US English.
 */
struct Noun {
  static const Noun byte; ///< A predefined noun.
  static const Noun character; ///< A predefined noun.

  /**
   * The singular form.
   */
  std::string_view singular;
  /**
   * The plural form.
   */
  std::string_view plural;

  /**
   * Text expansion function, returns either the singular or the plural form.
   *
   * @param count the amount
   * @return if @p count is -1 or 1, the singular, otherwise the plural
   */
  std::string_view
  operator()(i64 count) const {
    return count == 1 || count == -1 ? singular : plural;
  }

  /**
   * Text expansion function, returns @p count, followed by either the singular or the plural form.
   *
   * @param count the amount
   * @return the amount, followed by either the singular or the plural
   */
  std::string amount(i64 count) const;
};

} // namespace rocket::str

// EOF
