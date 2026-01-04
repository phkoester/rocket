/**
 * @file Noun.h
 *
 * Nouns with singular and plural forms.
 */

#pragma once

#include "rocket/format/format.h"

namespace rocket::str {

// `Noun` ---------------------------------------------------------------------------------------------------

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
   * @return a string containing the amount, followed by the singular if @p count is 1, followed by the
   *     plural otherwise
   */
  std::string
  operator()(size_t count) const {
    if (count == 1) {
      return fmt::format("{} {}", count, singular);
    } else {
      return fmt::format("{} {}", count, plural);
    }
  }
};

// Constants ------------------------------------------------------------------------------------------------

/**
 * A noun.
 */
extern const Noun character;

} // namespace rocket::str

// EOF
