/**
 * @file Bimap.h
 *
 * Bimaps, effectively `boost::bimaps::bimap`.
 */

#pragma once

#include "rocket/rocket.h"

#include <boost/bimap.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>

namespace rocket {

// `Bimap` --------------------------------------------------------------------------------------------------

/// The #rocket::Bimap type alias.
template<typename K, typename V>
using Bimap = boost::bimaps::bimap<
  boost::bimaps::set_of<K>,
  boost::bimaps::set_of<V>
>;

/**
 * Convenience function to make a #rocket::Bimap of a #std::initializer_list.
 *
 * @tparam K the map's left key type
 * @tparam V the map's left value type
 * @param list the map elements, as seen from the map's left index
 * @return a new #rocket::Bimap containing the elements of @p list in its left index
 */
template<typename K, typename V>
Bimap<K, V>
makeBimap(std::initializer_list<std::pair<K, V>> list = {}) {
  Bimap<K, V> ret;
  for (const auto& elem : list) {
    ret.insert({ std::move(elem.first), std::move(elem.second) }); // `bimap` has no `emplace`
  }
  return ret;
}

// `UnorderedBimap` -----------------------------------------------------------------------------------------

/// The #rocket::UnorderedBimap type alias.
template<typename K, typename V>
using UnorderedBimap = boost::bimaps::bimap<
  boost::bimaps::unordered_set_of<K>,
  boost::bimaps::unordered_set_of<V>
>;

/**
 * Convenience function to make a #rocket::UnorderedBimap of a #std::initializer_list.
 *
 * @tparam K the map's left key type
 * @tparam V the map's left value type
 * @param list the map elements, as seen from the map's left index
 * @return a new #rocket::UnorderedBimap containing the elements of @p list in its left index
 */
template<typename K, typename V>
UnorderedBimap<K, V>
makeUnorderedBimap(std::initializer_list<std::pair<K, V>> list = {}) {
  UnorderedBimap<K, V> ret;
  for (const auto& elem : list) {
    ret.insert({ std::move(elem.first), std::move(elem.second) }); // `bimap` has no `emplace`
  }
  return ret;
}

// `Positions` ----------------------------------------------------------------------------------------------

/**
 * A general-purpose bidirectional map that translates positions.
 */
using Positions = UnorderedBimap<u64, u64>;

// Functions ------------------------------------------------------------------------------------------------

/**
 * Inserts a key-value pair into a map, replacing the value if the key already exists.
 *
 * @tparam Map the map type
 * @tparam K the key type
 * @tparam V the value type
 * @param map the map to insert into
 * @param k the key to insert
 * @param v the value to insert
 * @return `true` if the key was inserted, `false` if the key was replaced
 */
template<typename Map, typename K, typename V>
bool
insertOrReplace(Map& map, const K& k, const V& v) {
  if (auto it = map.right.find(v); it != map.right.end()) {
    if (it->second == k) {
      return true;
    }
    map.right.erase(it);
  }
  if (auto it = map.left.find(k); it != map.left.end()) {
    return map.left.replace_data(it, v);
  }
  return map.insert({ k, v  }).second;
}

/**
 * Convenience function to make a #rocket::Positions from a #std::initializer_list.
 *
 * @param list the map elements, as seen from the map's left index
 * @return a new #rocket::Positions containing the elements of @p list
 */
inline Positions
makePositions(std::initializer_list<std::pair<u64, u64>> list) {
  return makeUnorderedBimap(list);
}

} // namespace rocket

// EOF
