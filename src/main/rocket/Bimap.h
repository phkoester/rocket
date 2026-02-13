/**
 * @file Bimap.h
 *
 * Bimaps, effectively `boost::bimaps::bimap`.
 */

#pragma once

#include <boost/bimap.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>

namespace rocket {

// #Bimap ---------------------------------------------------------------------------------------------------

/// The #Bimap type alias.
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

// #UnorderedBimap ------------------------------------------------------------------------------------------

/// The #UnorderedBimap type alias.
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

} // namespace rocket

// #boost::bimaps -------------------------------------------------------------------------------------------

namespace boost::bimaps {

/// @op_eq{`boost::bimaps::bimap`}
/// XXX Mit codec, dann -> Bimap-codec.h
template<typename A, typename B>
bool
operator==(const bimap<A, B>& lhs, const bimap<A, B>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (const auto& [key, val] : lhs.left) { // NOLINT
    auto it = rhs.left.find(key);
    if (it == rhs.left.end()) {
      return false;
    }
    if (it->second != val) {
      return false;
    }
  }
  return true;
}

/// @op_ne{`boost::bimaps::bimap`}
/// XXX Mit codec, dann -> Bimap-codec.h
template<typename A, typename B>
inline bool
operator!=(const bimap<A, B>& lhs, const bimap<A, B>& rhs) {
  return not operator==(lhs, rhs);
}

} // namespace boost::bimaps

// EOF
