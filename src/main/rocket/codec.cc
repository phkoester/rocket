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
      if (checkEof)
        throw except::ParseFailure<char>(is, io::tellg(is), "EOF");
      return;
    }
    io::check(is);

    // Read one char
    char c = io::getChar(is);
    if (is.eof()) {
      if (checkEof)
        throw except::ParseFailure<char>(is, io::tellg(is), "EOF");
      return;
    }
    io::check(is);

    if (c == '#') {
      // Skip comment, continue
      is.ignore(numeric_limits<streamsize>::max(), '\n');
      if (is.eof()) {
        if (checkEof)
          throw except::ParseFailure<char>(is, io::tellg(is), "EOF");
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

// RON printing ---------------------------------------------------------------------------------------------

namespace printing {

// `Params` .................................................................................................

namespace internal {

// The stack always holds at least one default-constructed element
thread_local vector<Params> stack(1);

thread_local size_t level = 0;

void
incLevel() {
  const auto& params = printing::params();
  if (not params.indent)
    return;
  ++level;
}

void
decLevel() {
  const auto& params = printing::params();
  if (not params.indent)
    return;
  ROCKET_ASSERT(level > 0);
  --level;
}

void
indent(ostream& os, size_t level) {
  for (size_t i = 0; i < level; ++i)
    os << "  ";
}

void
push(const Params& params) {
  stack.push_back(params);
}

void
pop() {
  ROCKET_ASSERT(stack.size() > 1);
  stack.pop_back();
}

} // namespace internal

const Params& params() { return internal::stack.back(); }

// Functions ................................................................................................

ostream&
endParent(ostream& os, bool indentChildren, char right) {
  const auto& params = printing::params();
  bool indent = params.indent && indentChildren;
  if (indent) {
    os << '\n';
    internal::indent(os, internal::level);
  }
  return os << right;
}

void
firstChild(ostream& os, bool indentChildren) {
  const auto& params = printing::params();
  bool indent = params.indent && indentChildren;
  if (not indent)
    return;
  os << '\n';
  internal::indent(os, internal::level);
}

void
groupByThousands(string& s, size_t begin, size_t end) {
  size_t pos = end;
  while (true) {
    if (pos <= 3) // Avoid `size_t` overflow
      break;
    pos -= 3;
    if (pos <= begin)
      break;
    s.insert(pos, 1, '\'');
  }
}

void
nextChild(
    ostream& os,
    bool indentChildren,
    const char* delimiter,
    const char* delimiterIndent) {
  const auto& params = printing::params();
  bool indent = params.indent && indentChildren;
  if (not indent)
    os << delimiter;
  else {
    os << delimiterIndent << '\n';
    internal::indent(os, internal::level);
  }
}

} // namespace printing

} // namespace ron

} // namespace rocket::codec

// EOF
