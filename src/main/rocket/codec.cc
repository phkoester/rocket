/*
 * codec.cc
 */

#include "codec-std-decl.h"
#include "codec-std.h"

#include "codec.h"

#include "assert.h"

#include <limits>

using namespace rocket;
using namespace std;

namespace rocket::codec {

// `Symbols` ------------------------------------------------------------------------------------------------

const set<char> Symbols::Chars::Digits { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
const set<char> Symbols::Chars::DigitsApostrophe {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '\''
};
const set<char> Symbols::Chars::E { 'e', 'E' };
const set<char> Symbols::Chars::PlusMinus { '+', '-' };

const set<string_view> Symbols::Strings::Bool { String::Zero, String::One, String::False, String::True };
const set<string_view> Symbols::Strings::EmptySet { String::EmptySet, String::EmptySetSymbol };
const set<string_view> Symbols::Strings::FloatingPoint {
  String::Infinity, String::InfinitySymbol,
  String::Nan,
  String::NegativeInfinity, String::NegativeInfinitySymbol,
  String::PositiveInfinity, String::PositiveInfinitySymbol,
  String::Qnan, String::Snan
};
const set<string_view> Symbols::Strings::Infinity {
  String::Infinity, String::InfinitySymbol, String::PositiveInfinity, String::PositiveInfinitySymbol
};
const set<string_view> Symbols::Strings::NegativeInfinity {
  String::NegativeInfinity, String::NegativeInfinitySymbol
};

// `std::istream` utilities ---------------------------------------------------------------------------------

bool
getBool(istream& is) {
  auto value = io::getString(is, Symbols::Strings::Bool);
  return value == Symbols::String::One || value == Symbols::String::True;
}

namespace ron {

namespace parsing {

// RON parsing ----------------------------------------------------------------------------------------------

EnumResult
parseEnum(istream& is) {
  EnumResult ret;
  skip(is);
  ret.actualInputPos = io::tellg(is);
  io::getChar(is, '"');
  ret.inputPos = ret.actualInputPos + 1;
  ret.input = io::getUntil(is, '"', true, 1);
  ret.actualInput = '"' + ret.input + '"';
  return ret;
}

void
skip(istream& is, bool checkEof) {
  while (true) {
    // Skip whitespace, if any
    is >> ws;
    if (is.eof()) {
      if (checkEof) {
        throw io::ParseFailure(is, io::tellg(is), "EOF");
      }
      return;
    }
    io::check(is);

    // Read one char
    char c = io::getChar(is);
    if (is.eof()) {
      if (checkEof) {
        throw io::ParseFailure(is, io::tellg(is), "EOF");
      }
      return;
    }
    io::check(is);

    if (c == '#') {
      // Skip comment, continue
      is.ignore(numeric_limits<streamsize>::max(), '\n');
      if (is.eof()) {
        if (checkEof) {
          throw io::ParseFailure(is, io::tellg(is), "EOF");
        }
        return;
      }
      io::check(is);
    } else {
      // Not a comment: put back character, break
      is.putback(c);
      break;
    }
  }
}

} // namespace parsing

} // namespace ron

} // namespace rocket::codec

// EOF
