/**
 * @file Bimap.h
 *
 * Bimaps, effectively `boost::bimaps::bimap`.
 */

#pragma once

#include "rocket/format/std.h"

#include <boost/bimap.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>

#include <map>

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
template<typename A, typename B>
bool
operator==(const bimap<A, B>& lhs, const bimap<A, B>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (const auto& [key, val] : lhs.left) {
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
template<typename A, typename B>
inline bool
operator!=(const bimap<A, B>& lhs, const bimap<A, B>& rhs) {
  return not operator==(lhs, rhs);
}

/// @op_output{`boost::bimaps::bimap`}
template<typename A, typename B>
inline std::ostream&
operator<<(std::ostream& lhs, const bimap<A, B>& rhs) {
  return lhs << fmt::format("{}", rhs);
}

/**
 * @fn_PrintTo{`boost::bimaps::bimap`}
 *
 * @todo Why do we need this? For some reason, GoogleTest doesn find `operator<<` ...
 */
template<typename A, typename B>
inline void
PrintTo(const bimap<A, B>& val, std::ostream* os) {
  *os << val;
}

} // namespace boost::bimaps

// #fmt::formatter<#boost::bimaps::bimap>--------------------------------------------------------------------

/**
 * @spec_fmt_formatter{`boost::bimaps::bimap`}
 *
 * This formatter formats the left map of a #rocket::UnorderedBimap. It uses the same format specifiers as
 * the underlying formatter for type #std::map.
 */
template<typename A, typename B, typename C>
struct fmt::formatter<boost::bimaps::bimap<A, B>, C> {
  /// @cond undocumented

  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const boost::bimaps::bimap<A, B>& val, FormatContext& ctx) const {
    // @todo Don't copy the whole map here
    std::map<K, V> map;
    for (const auto& [k, v] : val.left) { // cppcheck-suppress shadowArgument
      map.emplace(k, v);
    }
    return underlying_.format(map, ctx);
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return underlying_.parse(ctx);
  }

  constexpr void
  set_debug_format(bool val = true) {
    underlying_.set_debug_format(val);
  }

  /// @endcond

private:

  using K = boost::bimaps::bimap<A, B>::left_value_type::first_type;
  using V = boost::bimaps::bimap<A, B>::left_value_type::second_type;

  fmt::formatter<std::map<K, V>, C> underlying_;
};

// EOF
