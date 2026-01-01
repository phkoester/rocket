/**
 * @file container.h
 *
 * Containers, built on top of STL and Boost.
 */

#pragma once

#include "rocket/format/std.h"

#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>

#include <map>
#include <set>
#include <span>

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
makeUnorderedBimap(std::initializer_list<std::pair<K, V>> list = {}) {
  UnorderedBimap<K, V> ret;
  for (const auto& elem : list) {
    ret.insert({ std::move(elem.first), std::move(elem.second) }); // `bimap` has no `emplace`
  }
  return ret;
}

/**
 * Extracts the values from an #UnorderedBimap, as seen from the map's left index.
 *
 * @tparam K the map's key type
 * @tparam V the map's value type
 * @param v an #UnorderedBimap
 * @return the values from @p v as a `std::set`, as seen from the map's left index
 */
template<typename K, typename V>
std::set<V>
values(const UnorderedBimap<K, V>& v) {
  std::set<V> ret;
  for (const auto& elem : v.left)
    ret.insert(elem.second);
  return ret;
}

// Functions ------------------------------------------------------------------------------------------------

/**
 * Makes a nonconst span for the value @p v of arbitrary type @p T.
 *
 * @tparam E the element type of the span
 * @tparam T the type of @p v
 * @param v an arbitrary value
 * @return a new span which serves as buffer on @p v
 */
template<typename E, typename T>
inline std::span<E>
asSpan(T& v) {
  static_assert(sizeof(T) % sizeof(E) == 0);
  return std::span<E>(reinterpret_cast<E*>(&v), sizeof(T) / sizeof(E));
}

/**
 * Makes a const span for the value @p v of arbitrary type @p T.
 *
 * @tparam E the element type of the span
 * @tparam T the type of @p v
 * @param v an arbitrary value
 * @return a new span which serves as buffer on @p v
 */
template<typename E, typename T>
inline std::span<const E>
asSpan(const T& v) {
  static_assert(sizeof(T) % sizeof(E) == 0);
  return std::span<const E>(reinterpret_cast<const E*>(&v), sizeof(T) / sizeof(E));
}

} // namespace rocket::container

// Namespace `boost::bimaps` --------------------------------------------------------------------------------

namespace boost::bimaps {

/// @op_eq{`boost::bimaps::bimap`}
template<typename A, typename B>
bool
operator==(const bimap<A, B>& lhs, const bimap<A, B>& rhs) {
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
template<typename A, typename B>
inline bool
operator!=(const bimap<A, B>& lhs, const bimap<A, B>& rhs) {
  return not operator==(lhs, rhs);
}

/// @op_output{`boost::bimaps::bimap`}
template<typename A, typename B>
std::ostream&
operator<<(std::ostream& lhs, const bimap<A, B>& rhs) {
  return lhs << fmt::format("{}", rhs);
}

/// @fn_PrintTo{`boost::bimaps::bimap`}
template<typename A, typename B>
void PrintTo(const bimap<A, B>& v, std::ostream* os) {
  *os << v;
}

} // namespace boost::bimaps

// Namespace `fmt` ------------------------------------------------------------------------------------------

template<typename A, typename B, typename Char>
struct fmt::formatter<boost::bimaps::bimap<A, B>, Char> {
  template<typename FormatContext>
  constexpr auto
  format(const boost::bimaps::bimap<A, B>& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    // @todo Understand how `underlying_` is implemented in `fmt/ranges.h`, and don't make a copy of the
    // whole map here
    std::map<K, V> map;
    for (const auto& [k, v] : v.left) {
      map.emplace(k, v);
    }
    return underlying_.format(map, ctx);
  }

  constexpr const Char*
  parse(parse_context<Char>& ctx) {
    return underlying_.parse(ctx);
  }

private:

  using K = boost::bimaps::bimap<A, B>::left_value_type::first_type;
  using V = boost::bimaps::bimap<A, B>::left_value_type::second_type;

  fmt::formatter<std::map<K, V>> underlying_;
};

// EOF

