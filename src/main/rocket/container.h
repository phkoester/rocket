/**
 * @file container.h
 *
 * Containers, built on top of STL and Boost.
 */

#pragma once

#include <boost/bimap.hpp>
#include <boost/version.hpp>
#include <boost/bimap/unordered_set_of.hpp>

#include <set>

namespace rocket::container {

// `UnorderedBimap` -----------------------------------------------------------------------------------------

template<typename K, typename V>
using UnorderedBimap =
    boost::bimaps::bimap<boost::bimaps::unordered_set_of<K>, boost::bimaps::unordered_set_of<V>>;

/**
 * Convenience function to make an #UnorderedBimap of a `std::initializer_list`.
 *
 * @param list the map elements, as seen from the map's left index
 * @return a new #UnorderedBimap containing the elements of @p list in its left index
 */
template<typename K, typename V>
UnorderedBimap<K, V>
makeUnorderedBimap(const std::initializer_list<std::pair<K, V>>& list = {}) {
  UnorderedBimap<K, V> ret;
  for (const auto& elem : list) {
    ret.insert({ elem.first, elem.second }); // `bimap` has no `emplace`
  }
  return ret;
}

/**
 * Extracts the values from an #UnorderedBimap.
 *
 * @tparam K the map's key type
 * @tparam V the map's value type
 * @param v an #UnorderedBimap
 * @return the values from @p v as a `std::set`
 */
template<typename K, typename V>
std::set<V>
values(const UnorderedBimap<K, V>& v) {
  std::set<V> ret;
  for (const auto& elem : v.left)
    ret.insert(elem.second);
  return ret;
}

} // namespace rocket::container

// Namespace `boost::bimaps` --------------------------------------------------------------------------------

namespace boost::bimaps {

/// @op_eq{`boost::bimaps::bimap`}
template<typename K, typename V>
bool
operator==(const bimap<K, V>& lhs, const bimap<K, V>& rhs) {
  if (lhs.size() != rhs.size())
    return false;
  for (const auto& [k, v] : lhs.left) {
    auto it = rhs.left.find(k);
    if (it == rhs.left.end()) {
      return false;
    }
    if (it->second != v) {
      return false;
    }
  }
  return true;
}

/// @op_ne{`boost::bimaps::bimap`}
template<typename K, typename V>
inline bool
operator!=(const bimap<K, V>& lhs, const bimap<K, V>& rhs) {
  return not operator==(lhs, rhs);
}

} // namespace boost::bimaps

// EOF
