/**
 * @file codec-std.h
 *
 * Standard codec.
 */

#pragma once

#include "codec-std-decl.h"

#include "codec-global.h"

#include "codec.h"
#include "io.h"

#ifndef ROCKET_CODEC_H
#error `codec.h` must be included before this file
#endif

// Template definitions -------------------------------------------------------------------------------------

namespace std {

using ::parseRon;
using ::printRon;

/// @fn_printRon{`initializer_list`}
template<typename T>
inline ostream&
printRon(ostream& os, const initializer_list<T>& v) {
  constexpr bool indentChildren = rocket::IsContainer<T>::value;
  return rocket::codec::ron::printing::printRange(
      os, indentChildren, v.begin(), v.end(), '[', ", ", ",", ']');
}

/// @fn_parseRon{`optional`}
template<typename T>
istream&
parseRon(istream& is, optional<T>& v) {
  using namespace rocket::codec;

  ron::parsing::skip(is);
  size_t inputPos = rocket::io::tellg(is);

  // Try "null"
  try {
    rocket::io::getString<char>(is, "null");
    v.reset();
    return is;
  } catch (const rocket::except::InputFailure<char>&) {
    // Reset the stream, continue
    rocket::io::seekg(is, inputPos);
  }

  T value;
  parseRon(is, value);
  v = std::move(value);
  return is;
}

/// @fn_printRon{`optional`}
template<typename T>
inline ostream&
printRon(ostream& os, const optional<T>& v) {
  return v ? printRon(os, *v) : os << "null";
}

/// @fn_parseRon{`pair`}
template<typename A, typename B>
istream&
parseRon(istream& is, pair<A, B>& v) {
  using namespace rocket::codec;
  
  ron::parsing::skip(is);
  rocket::io::getChar(is, '(');

  parseRon(is, v.first);

  ron::parsing::skip(is);
  rocket::io::getChar(is, ',');

  parseRon(is, v.second);

  ron::parsing::skip(is);
  rocket::io::getChar(is, ')');
  return is;
}

/// @fn_printRon{`pair`}
template<typename A, typename B>
ostream&
printRon(ostream& os, const pair<A, B>& v) {
  using namespace rocket::codec::ron::printing;
  
  constexpr bool indentChildren = rocket::IsContainer<A>::value || rocket::IsContainer<B>::value;
  os << '('; {
    ROCKET_CODEC_RON_PRINT_CHILDREN();
    firstChild(os, indentChildren);
    printRon(os, v.first);
    nextChild(os, indentChildren, ", ", ",");
    printRon(os, v.second);
  }
  return endParent(os, indentChildren, ')');
}

/// @fn_parseRon{`set`}
template<typename T>
inline istream&
parseRon(istream& is, set<T>& v) {
  return rocket::codec::ron::parsing::parseSet(is, v);
}

/// @fn_printRon{`set`}
template<typename T>
inline ostream&
printRon(ostream& os, const set<T>& v) {
  constexpr bool indentChildren = rocket::IsContainer<T>::value;
  return rocket::codec::ron::printing::printRange(
      os, indentChildren, v.begin(), v.end(), '{', ", ", ",", '}');
}

/// @fn_printRon{`span`}
template<typename T>
inline ostream&
printRon(ostream& os, span<T> v) {
  constexpr bool indentChildren = rocket::IsContainer<T>::value;
  return rocket::codec::ron::printing::printRange(
      os, indentChildren, v.begin(), v.end(), '[', ", ", ",", ']');
}

/// @fn_parseRon{`tuple`}
template<typename... T>
inline istream&
parseRon(istream& is, tuple<T...>& v) {
  return rocket::codec::ron::parsing::parseTuple(is, v, make_index_sequence<sizeof...(T)>());
}

/// @fn_printRon{`tuple`}
template<typename... T>
inline ostream&
printRon(ostream& os, const tuple<T...>& v) {
  constexpr bool indentChildren = (... || rocket::IsContainer<T>::value);
  return rocket::codec::ron::printing::printTuple(
      os, indentChildren, v, make_index_sequence<sizeof...(T)>());
}

/// @fn_parseRon{`unordered_map`}
template<typename K, typename V>
inline istream&
parseRon(istream& is, unordered_map<K, V>& v) {
  return rocket::codec::ron::parsing::parseMap(is, v);
}

/// @fn_printRon{`unordered_map`}
template<typename K, typename V>
inline ostream&
printRon(ostream& os, const unordered_map<K, V>& v) {
  constexpr bool indentChildren = rocket::IsContainer<K>::value || rocket::IsContainer<V>::value;
  return rocket::codec::ron::printing::printMap(os, indentChildren, v);
}

/// @fn_parseRon{`unordered_set`}
template<typename T>
inline istream&
parseRon(istream& is, unordered_set<T>& v) {
  return rocket::codec::ron::parsing::parseSet(is, v);
}

/// @fn_printRon{`unordered_set`}
template<typename T>
inline ostream&
printRon(ostream& os, const unordered_set<T>& v) {
  constexpr bool indentChildren = rocket::IsContainer<T>::value;
  return rocket::codec::ron::printing::printRange(
      os, indentChildren, v.begin(), v.end(), '{', ", ", ",", '}');
}

/// @fn_parseRon{`variant`}
template<typename... T>
inline istream&
parseRon(istream& is, variant<T...>& v) {
  return rocket::codec::ron::parsing::parseVariant(is, v);
}

/// @fn_printRon{`variant`}
template<typename... T>
inline ostream&
printRon(ostream& os, const variant<T...>& v) {
  return visit([&](auto&& arg) -> auto& {
    os << v.index() << ':';
    return printRon(os, std::forward<decltype(arg)>(arg));
  }, v);
}

/// @fn_parseRon{`vector`}
template<typename T>
inline istream&
parseRon(istream& is, vector<T>& v) {
  return rocket::codec::ron::parsing::parseVector(is, v);
}

/// @fn_printRon{`vector`}
template<typename T>
inline ostream&
printRon(ostream& os, const vector<T>& v) {
  constexpr bool indentChildren = rocket::IsContainer<T>::value;
  return rocket::codec::ron::printing::printRange(
      os, indentChildren, v.begin(), v.end(), '[', ", ", ",", ']');
}

} // namespace std

// EOF
