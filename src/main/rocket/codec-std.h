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
  } catch (const rocket::io::InputFailure<char>&) {
    // Reset the stream, continue
    rocket::io::seekg(is, inputPos);
  }

  T value;
  parseRon(is, value);
  v = std::move(value);
  return is;
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

/// @fn_parseRon{`set`}
template<typename T>
inline istream&
parseRon(istream& is, set<T>& v) {
  return rocket::codec::ron::parsing::parseSet(is, v);
}

/// @fn_parseRon{`tuple`}
template<typename... T>
inline istream&
parseRon(istream& is, tuple<T...>& v) {
  return rocket::codec::ron::parsing::parseTuple(is, v, make_index_sequence<sizeof...(T)>());
}

/// @fn_parseRon{`unordered_map`}
template<typename K, typename V>
inline istream&
parseRon(istream& is, unordered_map<K, V>& v) {
  return rocket::codec::ron::parsing::parseMap(is, v);
}

/// @fn_parseRon{`unordered_set`}
template<typename T>
inline istream&
parseRon(istream& is, unordered_set<T>& v) {
  return rocket::codec::ron::parsing::parseSet(is, v);
}

/// @fn_parseRon{`variant`}
template<typename... T>
inline istream&
parseRon(istream& is, variant<T...>& v) {
  return rocket::codec::ron::parsing::parseVariant(is, v);
}

/// @fn_parseRon{`vector`}
template<typename T>
inline istream&
parseRon(istream& is, vector<T>& v) {
  return rocket::codec::ron::parsing::parseVector(is, v);
}

} // namespace std

// EOF
