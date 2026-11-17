/**
 * @file bimap.h
 *
 * Bimaps, effectively `boost::bimaps::bimap`.
 */

#pragma once

#include "rocket/rocket.h"

#include <boost/bimap.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>

#include <algorithm>

namespace rocket {

// `bimap` --------------------------------------------------------------------------------------------------

/// The #rocket::bimap type alias.
template<typename K, typename V>
using bimap = boost::bimaps::bimap<
  boost::bimaps::set_of<K>,
  boost::bimaps::set_of<V>
>;

/**
 * Convenience function to make a #rocket::bimap of a #std::initializer_list.
 *
 * @tparam K the map’s left key type
 * @tparam V the map’s left value type
 * @param list the map elements, as seen from the map’s left index
 * @return a new #rocket::bimap containing the elements of @p list in its left index
 */
template<typename K, typename V>
bimap<K, V>
make_bimap(std::initializer_list<std::pair<K, V>> list = {}) {
  bimap<K, V> ret;
  for (const auto& elem : list) {
    ret.insert({ std::move(elem.first), std::move(elem.second) }); // `bimap` has no `emplace`
  }
  return ret;
}

// `unordered_bimap` ----------------------------------------------------------------------------------------

/// The #rocket::unordered_bimap type alias.
template<typename K, typename V>
using unordered_bimap = boost::bimaps::bimap<
  boost::bimaps::unordered_set_of<K>,
  boost::bimaps::unordered_set_of<V>
>;

/**
 * Convenience function to make a #rocket::unordered_bimap of a #std::initializer_list.
 *
 * @tparam K the map’s left key type
 * @tparam V the map’s left value type
 * @param list the map elements, as seen from the map’s left index
 * @return a new #rocket::unordered_bimap containing the elements of @p list in its left index
 */
template<typename K, typename V>
unordered_bimap<K, V>
make_unordered_bimap(std::initializer_list<std::pair<K, V>> list = {}) {
  unordered_bimap<K, V> ret;
  for (const auto& elem : list) {
    ret.insert({ std::move(elem.first), std::move(elem.second) }); // `bimap` has no `emplace`
  }
  return ret;
}

} // namespace rocket

// `boost::bimaps` ------------------------------------------------------------------------------------------

namespace boost::bimaps {

/**
 * @op_eq{#rocket::unordered_bimap}
 *
 * Unlike ordered bimaps, unordered bimaps do not come with an `operator==` from Boost. Two unordered bimaps
 * are equal if they have the same size and each left key of @p lhs maps to an equal value in @p rhs. As
 * both indices of a bimap are unique, this is sufficient for equality.
 *
 * The operator is declared in this namespace so that it is found by argument-dependent lookup.
 */
template<typename K, typename V>
bool
operator==(const rocket::unordered_bimap<K, V>& lhs, const rocket::unordered_bimap<K, V>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  return std::ranges::all_of(lhs.left, [&rhs](const auto& elem) {
    auto it = rhs.left.find(elem.first);
    return it != rhs.left.end() && it->second == elem.second;
  });
}

} // namespace boost::bimaps

// EOF
