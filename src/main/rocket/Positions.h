/**
 * @file Positions.h
 *
 * Bidirectional maps that translate positions.
 */

#pragma once

#include "rocket/Bimap.h"

namespace rocket {

// `Positions` ----------------------------------------------------------------------------------------------

/**
 * A general-purpose bidirectional map that translates positions.
 */
using Positions = UnorderedBimap<u64, u64>;

// Functions ------------------------------------------------------------------------------------------------

/**
 * Convenience function to make a #rocket::Positions from a #std::initializer_list.
 *
 * @param list the map elements, as seen from the map’s left index
 * @return a new #rocket::Positions containing the elements of @p list
 */
inline Positions
makePositions(std::initializer_list<std::pair<u64, u64>> list) {
  return makeUnorderedBimap(list);
}

} // namespace rocket

// EOF
