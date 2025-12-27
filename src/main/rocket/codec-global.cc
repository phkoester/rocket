/*
 * codec-global.cc
 */

#include "codec-std-decl.h"
#include "codec-std.h"

#include "assert.h"
#include "codec.h"
#include "escape.h"
#include "unicode.h"

using namespace rocket;
using namespace rocket::codec;
using namespace std;

namespace {

// Local functions ------------------------------------------------------------------------------------------

template<typename I> requires Integer<I>
istream&
parseInteger(istream& is, I& v) {
  ron::parsing::skip(is);
  v = getInteger<I>(is);
  return is;
}

template<typename F> requires FloatingPoint<F>
istream&
parseFloatingPoint(istream& is, F& v, int precision) {
  ron::parsing::skip(is);
  v = getFloatingPoint<F>(is, precision);
  return is;
}

} // namespace

// Functions ------------------------------------------------------------------------------------------------

istream&
parseRon(istream& is, bool& v) {
  ron::parsing::skip(is);
  v = getBool(is);
  return is;
}

istream&
parseRon(istream& is, char& v) {
  // Skip
  ron::parsing::skip(is);
  size_t pos = io::tellg(is);

  // Unescape
  escape::CString::Params params { .enclosed=true, .quote='\'' };
  escape::Result escapedResult;
  string input;
  auto escaped = escape::escaped<escape::CString>(input, params, &escapedResult);
  is >> escaped;
  if (input.size() != 1 || unicode::countCodePoints(input) != 1) {
    throw io::ParseFailure(is, pos, { pos, io::tellg(is) },
        fmt::format("{}", message::cannotParseAs(escapedResult.input, Type::of<char>())));
  }
  v = input[0];
  return is;
}

istream&
parseRon(istream& is, unsigned char& v) {
  ron::parsing::skip(is);
  string dummy;
  v = io::getHex<uint32_t>(is, 2, dummy);
  return is;
}

istream&
parseRon(istream& is, char32_t& v) {
  // Skip
  ron::parsing::skip(is);
  size_t pos = io::tellg(is);

  // Unescape
  escape::CString::Params params { .enclosed=true, .quote='\'' };
  escape::Result escapedResult;
  string input;
  auto escaped = escape::escaped<escape::CString>(input, params, &escapedResult);
  is >> escaped;
  u32string input32 = unicode::utf8To32(input);
  if (input32.size() != 1 || unicode::countCodePoints(input32) != 1) {
    throw io::ParseFailure(is, pos, { pos, io::tellg(is) },
        message::cannotParseAs(escapedResult.input, Type::of<char32_t>()));
  }
  v = input32[0];
  return is;
}

istream&
parseRon(istream& is, int16_t& v) {
  return parseInteger(is, v);
}

istream&
parseRon(istream& is, uint16_t& v) {
  return parseInteger(is, v);
}

istream&
parseRon(istream& is, int32_t& v) {
  return parseInteger(is, v);
}

istream&
parseRon(istream& is, uint32_t& v) {
  return parseInteger(is, v);
}

istream&
parseRon(istream& is, int64_t& v) {
  return parseInteger(is, v);
}

istream&
parseRon(istream& is, uint64_t& v) {
  return parseInteger(is, v);
}

istream&
parseRon(istream& is, int128_t& v) {
  return parseInteger(is, v);
}

istream&
parseRon(istream& is, uint128_t& v) {
  return parseInteger(is, v);
}

istream&
parseRon(istream& is, float& v, int precision) {
  return parseFloatingPoint(is, v, precision);
}

istream&
parseRon(istream& is, double& v, int precision) {
  return parseFloatingPoint(is, v, precision);
}

istream&
parseRon(istream& is, long double& v, int precision) {
  return parseFloatingPoint(is, v, precision);
}

// EOF
