/**
 * @file noun.h
 *
 * Nouns with singular and plural forms.
 */

#pragma once

#include "S.h"

namespace rocket::noun {

// `Noun` ---------------------------------------------------------------------------------------------------

/**
 * A noun that knows its singular and plural form, in US English.
 *
 * Example:
 *
 * @code{.cc}
 * cout << rocket::nouns::character(1) << '\n'; // Output: "1 character\n"
 * cout << rocket::nouns::character(2) << '\n'; // Output: "2 characters\n"
 * @endcode
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
    if (count == 1)
      return S << count << " " << raw(singular);
    else
      return S << count << " " << raw(plural);
  }
};

// Constants ------------------------------------------------------------------------------------------------

/**
 * A noun.
 */
extern const Noun character;

} // namespace rocket::noun

// EOF
