/**
 * @file Positions.h
 *
 * A general-purpose bidirectional map that translates `size_t` values.
 */

#pragma once

#include "boost.h"

namespace rocket {

// `Positions` ----------------------------------------------------------------------------------------------

/// A general-purpose bidirectional map that translates `size_t` values.
using Positions = boost::bimap::UnorderedBimap<size_t, size_t>::Type;

// Functions ------------------------------------------------------------------------------------------------

/**
 * Convenience function to make a #rocket::Positions map of a `std::initializer_list`.
 *
 * @param list the map elements, as seen from the map's left index
 * @return a new #rocket::Positions map containing the elements of @p list in its left index
 */
inline Positions
positions(const std::initializer_list<std::pair<size_t, size_t>>& list = {}) {
  return rocket::boost::bimap::UnorderedBimap<size_t, size_t>::of(list);
}

} // namespace rocket

// EOF
