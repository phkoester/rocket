/**
 * @file locale.h
 *
 * Utilities related to `std::locale`.
 */

#pragma once

#include <locale>

namespace std {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Makes a locale from the locale @p lhs and the pointer to a facet @p rhs.
 *
 * @tparam Facet the facet type
 * @param lhs the locale to copy
 * @param rhs a pointer to a facet. This may be the result of `new`.
 * @return a new locale that is a copy of @p lhs, with the additional facet @p rhs
 */
template<typename Facet>
inline locale
operator<<(const locale& lhs, Facet* rhs) {
  return locale(lhs, rhs);
}

} // namespace std

// EOF
