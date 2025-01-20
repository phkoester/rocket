/**
 * @file codec-std-decl.h
 *
 * Standard codec: declarations.
 */

#pragma once

#include "concept.h"

#include <span>
#include <unordered_map>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#ifdef ROCKET_CODEC_H
#error `codec.h` must be included after this file
#endif

namespace std {

// Functions ------------------------------------------------------------------------------------------------

/// @fn_parseRon{`byte`}
std::istream& parseRon(istream& is, byte& v);

/// @fn_printRon{`byte`}
ostream& printRon(ostream& os, byte v);

/// @fn_parseRon{`string`}
std::istream& parseRon(istream& is, string& v);

/// @fn_printRon{`string`}
ostream& printRon(ostream& os, const string& v);

/// @fn_printRon{`string_view`}
ostream& printRon(ostream& os, string_view v);

/// @fn_parseRon{`u32string`}
std::istream& parseRon(istream& is, u32string& v);

/// @fn_printRon{`u32string`}
ostream& printRon(ostream& os, const u32string& v);

/// @fn_printRon{`u32string_view`}
ostream& printRon(ostream& os, u32string_view v);

// Template declarations ------------------------------------------------------------------------------------

template<typename T> ostream& printRon(ostream&, const initializer_list<T>&);

template<typename T> istream& parseRon(istream&, optional<T>&);

template<typename T> ostream& printRon(ostream&, const optional<T>&);

template<typename A, typename B> istream& parseRon(istream&, pair<A, B>&);

template<typename A, typename B> ostream& printRon(ostream&, const pair<A, B>&);

template<typename T> istream& parseRon(istream&, set<T>&);

template<typename T> ostream& printRon(ostream&s, const set<T>&);

template<typename T> ostream& printRon(ostream&, span<T>);

template<typename... T> istream& parseRon(istream&, tuple<T...>&);

template<typename... T> ostream& printRon(ostream&, const tuple<T...>&);

template<typename K, typename V> istream& parseRon(istream&, unordered_map<K, V>&);

template<typename K, typename V> ostream& printRon(ostream&, const unordered_map<K, V>&);

template<typename T> istream& parseRon(istream&s, unordered_set<T>&);

template<typename T> ostream& printRon(ostream&s, const unordered_set<T>&);

template<typename... T> istream& parseRon(istream&, variant<T...>&);

template<typename... T> ostream& printRon(ostream&, const variant<T...>&);

template<typename T> istream& parseRon(istream&, vector<T>&);

template<typename T> ostream& printRon(ostream&, const vector<T>&);

} // namespace std

// `IsContainerImpl` specializations ------------------------------------------------------------------------

namespace rocket {

/// @spec_rocket_IsContainerImpl{`std::initializer_list`}
template<typename T> struct IsContainerImpl<std::initializer_list<T>> : std::true_type {};

/// @spec_rocket_IsContainerImpl{`std::optional`}
template<typename T> struct IsContainerImpl<std::optional<T>> : IsContainerImpl<T>::type {};

/// @spec_rocket_IsContainerImpl{`std::pair`}
template<typename A, typename B> struct IsContainerImpl<std::pair<A, B>> : std::true_type {};

/// @spec_rocket_IsContainerImpl{`std::set`}
template<typename K, typename V> struct IsContainerImpl<std::set<K, V>> : std::true_type {};

/// @spec_rocket_IsContainerImpl{`std::span`}
template<typename T> struct IsContainerImpl<std::span<T>> : std::true_type {};

/// @spec_rocket_IsContainerImpl{`std::tuple`}
template<typename... T> struct IsContainerImpl<std::tuple<T...>> : std::true_type {};

/// @spec_rocket_IsContainerImpl{`std::unordered_map`}
template<typename K, typename V> struct IsContainerImpl<std::unordered_map<K, V>> : std::true_type {};

/// @spec_rocket_IsContainerImpl{`std::unordered_set`}
template<typename T> struct IsContainerImpl<std::unordered_set<T>> : std::true_type {};

/// @spec_rocket_IsContainerImpl{`std::vector`}
template<typename T> struct IsContainerImpl<std::vector<T>> : std::true_type {};

} // namespace rocket

// EOF
