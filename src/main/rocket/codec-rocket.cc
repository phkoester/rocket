/*
 * codec-rocket.cc
 */

#include "codec-rocket-decl.h"
#include "codec-std-decl.h"
#include "codec-rocket.h"
#include "codec-std.h"

#include "enum.h"
#include "escape.h"

using namespace std;

namespace rocket {

// Functions ------------------------------------------------------------------------------------------------

std::ostream&
printRon(std::ostream& os, const Type& v) {
  return os << '`' << v.name() << '`';
}

namespace text {

ROCKET_ENUM_DEFINE(Position::Type, Position_Type, (note)(warning)(error));

} // namespace text

namespace unicode {

istream&
parseRon(istream& is, CodePoint& v) {
  // Skip
  codec::ron::parsing::skip(is);
  size_t inputPos = io::tellg(is);

  // "U+"
  io::getString<char>(is, "U+");
  string input = io::getWhile(is, io::Symbols<char>::Chars::HexDigits, 4);

  // Hex digits
  if (input.size() > 8) {
    throw except::ParseFailure<char>(
        is, inputPos + 2 + 8, { inputPos, io::tellg(is) },
        S << "Expected at most 8 hexadecimal characters, got " << input.size());
  }

  // Parse
  auto localIs = io::is(input);
  uint32_t i;
  localIs >> hex >> i;
  v = i;

  // Done
  return is;
}

ostream&
printRon(ostream& os, CodePoint v) {
  // Say goodbye to `ostringstream` ...
  return os << fmt::format("U+{:0>4X}", static_cast<uint32_t>(v));
}

istream&
parseRon(istream& is, Grapheme& v) {
  // Skip
  codec::ron::parsing::skip(is);
  size_t pos = io::tellg(is);

  // Unescape
  escape::CString::Params params { .enclosed=true, .quote='"' };
  escape::Result<char> escapedResult;
  string input;
  is >> escape::escaped<escape::CString>(input, params, &escapedResult);
  if (unicode::countGraphemes(input) != 1) {
    throw except::ParseFailure<char>(
        is, pos, { pos, io::tellg(is) },
        except::message::cannotParseAs(escapedResult.input, Type::of<Grapheme>()));
  }
  v = Grapheme(input);
  return is;
}

ostream&
printRon(ostream& os, const Grapheme& v) {
  return printRon(os, static_cast<string>(v));
}

} // namespace unicode

} // namespace rocket

// EOF
