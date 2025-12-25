/**
 * @file boost.h
 *
 * Boost utilities.
 */

#pragma once

#include <boost/bimap.hpp>
#include <boost/version.hpp>
#include <boost/bimap/unordered_set_of.hpp>

#include <set>

namespace rocket::boost {

namespace bimap {

// `UnorderedBimap` -----------------------------------------------------------------------------------------

/**
 * A class template that simplifies the usage of an unordered `bimap` with unordered indices for both keys
 * and values.
 *
 * @tparam K the map's key type
 * @tparam V the map's value type
 */
template<typename K, typename V>
struct UnorderedBimap {
  /**
   * Unordered keys.
   */
  using KeysType = ::boost::bimaps::unordered_set_of<K>;
  /**
   * Unordered values.
   */
  using ValuesType = ::boost::bimaps::unordered_set_of<V>;
  /**
   * The actual `bimap` type.
   */
  using Type = ::boost::bimaps::bimap<KeysType, ValuesType>;

  /**
  * Convenience function to make an unordered `bimap` from a `std::initializer_list`.
  *
  * @param list the map elements
  * @return an unordered `bimap`
  */
  static inline Type
  of(const std::initializer_list<std::pair<K, V>>& list = {}) {
    Type ret;
    for (const auto& elem : list)
      ret.insert({ elem.first, elem.second }); // `bimap` has no `emplace`
    return ret;
  }
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Extracts the values from an unordered `bimap`.
 *
 * @tparam K the map's key type
 * @tparam V the map's value type
 * @param map a `bimap`
 * @return the values from @p map as a `std::set`
 */
template<typename K, typename V>
std::set<V>
values(const typename UnorderedBimap<K, V>::Type& map) {
  std::set<V> ret;
  for (const auto& elem : map.left)
    ret.insert(elem.second);
  return ret;
}

} // namespace bimap

} // namespace rocket::boost

// `boost::bimaps` ------------------------------------------------------------------------------------------

namespace boost::bimaps {

/// @op_eq{`boost::bimaps::bimap`}
template<typename K, typename V>
bool
operator==(const bimap<K, V>& lhs, const bimap<K, V>& rhs) {
  if (lhs.size() != rhs.size())
    return false;
  for (const auto& [k, v] : lhs.left) {
    auto it = rhs.left.find(k);
    if (it == rhs.left.end())
      return false;
    if (it->second != v)
      return false;
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
