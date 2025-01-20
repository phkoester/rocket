/**
 * @file codec-rocket.h
 *
 * Rocket codec.
 */

#pragma once

#include "codec-rocket-decl.h"

#include "codec-global.h"

#include "codec.h"

#ifndef ROCKET_CODEC_H
#error `codec.h` must be included before this file
#endif

namespace rocket {

using ::parseRon;
using ::printRon;

// Template definitions -------------------------------------------------------------------------------------

namespace math {

/// @fn_parseRon{#rocket::math::IntervalImpl}
template<typename T, typename Left, typename Right>
std::istream&
parseRon(std::istream& is, IntervalImpl<T, Left, Right>& v) {
  using namespace rocket::codec;

  // Try empty-set symbols
  ron::parsing::skip(is);
  size_t pos = io::tellg(is);
  try {
    io::getString<>(is, Symbols::Strings::EmptySet);
    v = {};
    return is;
  } catch (const except::InputFailure<char>& ex) {
    io::seekg(is, pos);
  }

  io::getChar(is, Left::Symbol);

  // Parse `lower`
  ron::parsing::skip(is);
  bool parsed = false;
  if constexpr (not Left::Closed) {
    pos = io::tellg(is);
    try {
      io::getString<>(is, Symbols::Strings::NegativeInfinity);
      v.lower = std::nullopt;
      parsed = true;
    } catch (const except::InputFailure<char>&) {
      io::seekg(is, pos);
    }
  }
  if (not parsed)
    parseRon(is, v.lower);

  ron::parsing::skip(is);
  io::getChar(is, ',');

  // Parse `upper`
  ron::parsing::skip(is);
  parsed = false;
  if constexpr (not Right::Closed) {
    pos = io::tellg(is);
    try {
      io::getString<>(is, Symbols::Strings::Infinity);
      v.upper = std::nullopt;
      parsed = true;
    } catch (const except::InputFailure<char>&) {
      io::seekg(is, pos);
    }
  }
  if (not parsed)
    parseRon(is, v.upper);

  ron::parsing::skip(is);
  io::getChar(is, Right::Symbol);
  return is;
}

/// @fn_printRon{#rocket::math::IntervalImpl}
template<typename T, typename Left, typename Right>
std::ostream&
printRon(std::ostream& os, const IntervalImpl<T, Left, Right>& v) {
  if (v.empty()) {
    // Use a neat mathematical symbol
    return os << "∅";
  } else {
    os << Left::Symbol;
    auto opt = option(v.lower);
    if (not opt)
      os << "-∞";
    else
      printRon(os, *opt);
    os << ',';
    opt = option(v.upper);
    if (not opt)
      os << "+∞"; // In interval notation, we prefer `+∞` over `∞`
    else
      printRon(os, *opt);
    return os << Right::Symbol;
  }
}

} // namespace math

} // namespace rocket

// EOF
