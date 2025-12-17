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

template<typename I> requires Integer<I>
ostream&
printInteger(ostream& os, I v) {
  ostringstream buf;
  buf << v;
  string s = buf.str();

  // Group by thousands
  size_t begin;
  if constexpr (is_unsigned_v<I>)
    begin = 0;
  else
    begin = s[0] == '-' ? 1 : 0;
  ron::printing::groupByThousands(s, begin, s.size());

  return os << s;
}

template<typename F> requires FloatingPoint<F>
istream&
parseFloatingPoint(istream& is, F& v, int precision) {
  ron::parsing::skip(is);
  v = getFloatingPoint<F>(is, precision);
  return is;
}

template<typename F> requires FloatingPoint<F>
ostream&
printFloatingPoint(ostream& os, F v, int precision) {
  if (isinf(v))
    return os << (v < 0 ? Symbols::String::NegativeInfinitySymbol : Symbols::String::InfinitySymbol);
  if (isnan(v))
    return os << (issignaling(v) ? Symbols::String::Snan : Symbols::String::Qnan);

  ostringstream buf;
  buf << setprecision(precision) << v;
  string s = buf.str();

  // Group by thousands
  size_t begin = s[0] == '-' ? 1 : 0;
  size_t end;
  size_t dot = s.find('.');
  size_t e = s.find_first_of("eE");
  if (dot != string::npos)
    end = dot;
  else if (e != string::npos)
    end = e;
  else
    end = s.size();
  ron::printing::groupByThousands(s, begin, end);

  return os << s;
}

} // namespace

// Functions ------------------------------------------------------------------------------------------------

istream&
parseRon(istream& is, bool& v) {
  ron::parsing::skip(is);
  v = getBool(is);
  return is;
}

ostream&
printRon(ostream& os, bool v) {
  return os << (v ? Symbols::String::True : Symbols::String::False);
}

istream&
parseRon(istream& is, char& v) {
  // Skip
  ron::parsing::skip(is);
  size_t pos = io::tellg(is);

  // Unescape
  escape::CString::Params params { .enclosed=true, .quote='\'' };
  escape::Result<char> escapedResult;
  string input;
  is >> escape::escaped<escape::CString>(input, params, &escapedResult);
  if (input.size() != 1 || unicode::countCodePoints(input) != 1) {
    throw except::ParseFailure<char>(
        is, pos, { pos, io::tellg(is) },
        except::message::cannotParseAs(escapedResult.input, Type::of<char>()));
  }
  v = input[0];
  return is;
}

ostream&
printRon(ostream& os, char v) {
  if (isascii(v)) {
    string s { v };
    ostringstream oss;
    oss << escape::escaped<escape::CString>(s, { .enclosed=true, .quote='\'' });
    return os << oss.str();
  } else {
    return os << format("'\\x{:0>2x}'", static_cast<int>(static_cast<unsigned char>(v)));
  }
}

istream&
parseRon(istream& is, unsigned char& v) {
  ron::parsing::skip(is);
  string dummy;
  v = io::getHex<uint32_t>(is, 2, dummy);
  return is;
}

ostream&
printRon(ostream& os, unsigned char v) {
  return os << format("{:0>2x}", static_cast<int>(v));
}

istream&
parseRon(istream& is, char32_t& v) {
  // Skip
  ron::parsing::skip(is);
  size_t pos = io::tellg(is);

  // Unescape
  escape::CString::Params params { .enclosed=true, .quote='\'' };
  escape::Result<char> escapedResult;
  string input;
  is >> escape::escaped<escape::CString>(input, params, &escapedResult);
  u32string input32 = unicode::utf8To32(input);
  if (input32.size() != 1 || unicode::countCodePoints(input32) != 1) {
    throw except::ParseFailure<char>(
        is, pos, { pos, io::tellg(is) },
        except::message::cannotParseAs(escapedResult.input, Type::of<char32_t>()));
  }
  v = input32[0];
  return is;
}

ostream&
printRon(ostream& os, char32_t v) {
  u32string s { v };
  u32ostringstream oss;
  oss << escape::escaped<escape::CString>(s, { .enclosed=true, .quote='\'' });
  return os << unicode::utf32To8(oss.str());
}

istream&
parseRon(istream& is, int16_t& v) {
  return parseInteger(is, v);
}

ostream&
printRon(ostream& os, int16_t v) {
  return printInteger(os, v);
}

istream&
parseRon(istream& is, uint16_t& v) {
  return parseInteger(is, v);
}

ostream&
printRon(ostream& os, uint16_t v) {
  return printInteger(os, v);
}

istream&
parseRon(istream& is, int32_t& v) {
  return parseInteger(is, v);
}

ostream&
printRon(ostream& os, int32_t v) {
  return printInteger(os, v);
}

istream&
parseRon(istream& is, uint32_t& v) {
  return parseInteger(is, v);
}

ostream&
printRon(ostream& os, uint32_t v) {
  return printInteger(os, v);
}

istream&
parseRon(istream& is, int64_t& v) {
  return parseInteger(is, v);
}

ostream&
printRon(ostream& os, int64_t v) {
  return printInteger(os, v);
}

istream&
parseRon(istream& is, uint64_t& v) {
  return parseInteger(is, v);
}

ostream&
printRon(ostream& os, uint64_t v) {
  return printInteger(os, v);
}

istream&
parseRon(istream& is, int128_t& v) {
  return parseInteger(is, v);
}

ostream&
printRon(ostream& os, int128_t v) {
  return printInteger(os, v);
}

istream&
parseRon(istream& is, uint128_t& v) {
  return parseInteger(is, v);
}

ostream&
printRon(ostream& os, uint128_t v) {
  return printInteger(os, v);
}

istream&
parseRon(istream& is, float& v, int precision) {
  return parseFloatingPoint(is, v, precision);
}

ostream&
printRon(ostream& os, float v, int precision) {
  return printFloatingPoint(os, v, precision);
}

istream&
parseRon(istream& is, double& v, int precision) {
  return parseFloatingPoint(is, v, precision);
}

ostream&
printRon(ostream& os, double v, int precision) {
  return printFloatingPoint(os, v, precision);
}

istream&
parseRon(istream& is, long double& v, int precision) {
  return parseFloatingPoint(is, v, precision);
}

ostream&
printRon(ostream& os, long double v, int precision) {
  return printFloatingPoint(os, v, precision);
}

// EOF
