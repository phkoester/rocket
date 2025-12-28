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

namespace unicode {

istream&
parseRon(istream& is, CodePoint& v) {
  // Skip
  codec::ron::parsing::skip(is);
  size_t inputPos = io::tellg(is);

  // "U+"
  io::getString(is, "U+");
  string input = io::getWhile(is, io::Symbols::HexDigits, 4);

  // Hex digits
  if (input.size() > 8) {
    throw io::ParseFailure(is, inputPos + 2 + 8, { inputPos, io::tellg(is) },
        fmt::format("Expected at most 8 hexadecimal characters, got {}", input.size()));
  }

  // Parse
  auto localIs = io::is(input);
  uint32_t i;
  localIs >> hex >> i;
  v = i;

  // Done
  return is;
}

istream&
parseRon(istream& is, Grapheme& v) {
  // Skip
  codec::ron::parsing::skip(is);
  size_t pos = io::tellg(is);

  // Unescape
  escape::CString::Params params { .enclosed=true, .quote='"' };
  escape::Result escapedResult;
  string input;
  auto escaped = escape::escaped<escape::CString>(input, params, &escapedResult);
  is >> escaped;
  if (unicode::countGraphemes(input) != 1) {
    throw io::ParseFailure(is, pos, { pos, io::tellg(is) },
        message::cannotParseAs(escapedResult.input, Type::of<Grapheme>()));
  }
  v = Grapheme(input);
  return is;
}

} // namespace unicode

} // namespace rocket

// EOF
