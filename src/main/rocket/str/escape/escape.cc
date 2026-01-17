/*
 * escape.cc
 */

#include "escape.h"

#include "rocket/InputFailure.h"
#include "rocket/assert.h"
#include "rocket/unicode/Character.h"
#include "rocket/unicode/Iterator.h"

#include <inttypes.h> // `SCNx32`

using namespace rocket;
using namespace rocket::str;
using namespace rocket::str::escape;
using namespace std;

namespace {

// Local functions ------------------------------------------------------------------------------------------

string escapeCStringCodePointHex(unicode::CodePoint, u64&);
string escapeCStringTab(u64&, const CStringParams&);

string
escapeCStringCodePoint(unicode::CodePoint cp, u64& column, const CStringParams& params) {
  // Escapable characters
  string ret;
  if (cp >= '\a' && cp <= '\\') {
    switch (cp) {
    case '\a': // Alert = 7
      ret = string { '\\', 'a' };
      break;
    case '\b':// Backspace = 8
      ret = string { '\\', 'b' };
      break;
    case '\t':// Horizontal tab = 9
      return escapeCStringTab(column, params);
    case '\n': // Line feed = 10
      ret = string { '\\', 'n' };
      break;
    case '\v': // Vertical tab = 11
      ret = string { '\\', 'v' };
      break;
    case '\f': // Form feed = 12
      ret = string { '\\', 'f' };
      break;
    case '\r': // Carriage return = 13
      ret = string { '\\', 'r' };
      break;
    case '\e': // Escape = 27
      ret = string { '\\', 'e' };
      break;
    case '"': // Quotation mark = 34
      ret = params.quote == '"' ? string { '\\', '"' } : string { '"' };
      break;
    case '\'': // Apostrophe = 39
      ret = params.quote == '\'' ? string { '\\', '\'' } : string { '\'' };
      break;
    case '\\': // Backslash = 92
      ret = string { '\\', '\\' };
      break;
    }
  }
  if (not ret.empty()) {
    column += ret.size();
    return ret;
  }

  // Printable characters
  if (cp.isPrint()) {
    column += cp.width();
    return static_cast<string>(cp);
  }

  // Hex otherwise
  return escapeCStringCodePointHex(cp, column);
}

string
escapeCStringCodePointHex(unicode::CodePoint cp, u64& column) {
  string ret;
  if (cp > 0xffffU)
    ret = fmt::format("\\U{:0>8X}", static_cast<u32>(cp));
  else if (cp > 0x00ffU)
    ret = fmt::format("\\u{:0>4X}", static_cast<u32>(cp));
  else
    ret = fmt::format("\\x{:0>2X}", static_cast<u32>(cp));
  column += ret.size();
  return ret;
}

string
escapeCStringTab(u64& column, const CStringParams& params) {
  if (not params.tabSize) {
    string ret { '\\', 't' };
    column += ret.size();
    return ret;
  }
  else {
    u64 mod = column % *params.tabSize;
    string ret(*params.tabSize - mod, ' ');
    column += ret.size();
    return ret;
  }
}

string
escapeRegexCodePoint(unicode::CodePoint cp, u64& column) {
  // Escapable characters
  string ret;
  if (cp >= '\t' && cp <= '}') {
    switch (cp) {
    case '\t': // Horizontal tab = 9
      ret = string { '\\', 't' };
      break;
    case '\n': // Line feed = 10
      ret = string { '\\', 'n' };
      break;
    case '\v': // Vertical tab = 11
      ret = string { '\\', 'v' };
      break;
    case '\f': // Form feed = 12
      ret = string { '\\', 'f' };
      break;
    case '\r': // Carriage return = 13
      ret = string { '\\', 'r' };
      break;
    case '$' : // Dollar sign = 36
    case '(' : // Left parenthesis = 40
    case ')' : // Right parenthesis = 41
    case '*' : // Asterisk = 42
    case '+' : // Plus sign = 43
    case '.' : // Dot = 46
    case '?' : // Question mark = 63
    case '[' : // Left bracket = 91
    case '\\': // Backslash = 92
    case ']' : // Right bracket = 93
    case '^' : // Circumflex = 94
    case '{' : // Left Brace = 123
    case '|' : // Vertical bar = 124
    case '}' : // Right brace = 125
      ret = string { '\\', static_cast<char>(cp) };
      break;
    }
  }
  if (not ret.empty()) {
    column += ret.size();
    return ret;
  }

  // Printable characters, code points > U+FFFF
  if (cp.isPrint() || cp > 0xffffU) {
    column += cp.width();
    return static_cast<string>(cp);
  }

  // Hex otherwise (only up to U+FFFF)
  return escapeCStringCodePointHex(cp, column);
}

unicode::CharacterView<char>
getChar(unicode::Iterator<char>& iter) {
  auto seg = iter.nextSegment();
  if (seg.empty()) {
    throw InputFailure(iter.current(), "Expected character, got EOI");
  }
  return unicode::CharacterView<char>(seg);
}

void
getChar(unicode::Iterator<char>& iter, char expected) {
  u64 pos = iter.current();
  auto seg = iter.nextSegment();
  if (seg.empty()) {
    throw InputFailure(pos, fmt::format("Expected character {:?}, got EOI", expected));
  }
  unicode::CharacterView<char> c(seg);
  if (not c.eq(expected)) {
    throw InputFailure(pos, fmt::format("Expected character {:?}, got {:?}", expected, c));
  }
}

u32
getHex(unicode::Iterator<char>& iter, u64 n) {
  u64 pos0 = iter.current();

  string input;
  for (u64 i = 0; i < n; ++i) {
    auto pos = iter.current();
    auto seg = iter.nextSegment();
    if (seg.empty()) {
      throw InputFailure(pos, { pos0, iter.current() },
          fmt::format("Expected {} hexadecimal digits, got EOI", n, input));
    }
    auto c = unicode::CharacterView<char>(seg);
    if (not c.isXdigit()) {
      throw InputFailure(pos, { pos0, iter.current() },
          fmt::format("Expected a hexadecimal digit, got {:?}", c));
    }
    input.append(c);
  }

  u32 ret = 0;
  std::sscanf(input.c_str(), "%" SCNx32, &ret);
  return ret;
}

optional<unicode::CharacterView<char>>
getOptionalChar(unicode::Iterator<char>& iter) {
  auto seg = iter.nextSegment();
  if (seg.empty()) {
    return nullopt;
  }
  return unicode::CharacterView<char>(seg);
}

} // namespace

// Functions ------------------------------------------------------------------------------------------------

namespace rocket::str::escape {

string
escapeCString(string_view input, const CStringParams& params, Result* result) {
  ROCKET_CHECK(params, params.quote == '\0' || params.quote == '"' || params.quote == '\'');

  string ret;
  if (result) {
    result->positions.clear();
  }
  u64 to = 0;

  // If needed, add quote

  if (params.quoted()) {
    ret.push_back(params.quote);
    ++to;
  }

  // Loop through characters

  auto iter = unicode::Iterator(unicode::IteratorType::Character, input);
  u64 column = 0;
  while (true) {
    // Obtain character

    auto current = iter.current();
    auto seg = iter.nextSegment();
    if (seg.empty()) {
      // EOI
      break;
    }
    auto c = unicode::CharacterView<char>(seg);

    if (result) {
      result->positions.insert({ current, to });
    }

    if (auto cp = c.toCodePoint(); cp) {
      // Single-code-point character

      auto escaped = escapeCStringCodePoint(*cp, column, params);
      ret.append(escaped);
      to += escaped.size();
    } else if (c.crLf()) {
      // CR/LF
      column += 4;
      ret.append("\\r\\n");
      to += 4;
    } else {
      // Multi-code-point character

      column += c.width();
      auto add = static_cast<string_view>(c);
      ret.append(add);
      to += add.size();
    }
  }

  // Add EOI position

  if (result) {
    result->positions.insert({ iter.current(), to });
  }

  // If needed, add quote

  if (params.quoted()) {
    ret.push_back(params.quote);
  }

  return ret;
}

string
unescapeCString(string_view input, const CStringParams& params, Result* result) {
  ROCKET_CHECK(params, params.quote == '\0' || params.quote == '"' || params.quote == '\'');

  string ret;

  if (result) {
    result->positions.clear();
  }

  // If needed, read quote

  auto iter = unicode::Iterator(unicode::IteratorType::Character, input);
  if (params.quoted()) {
    getChar(iter, params.quote);
  }

  while (true) {
    // Read character

    u64 pos = iter.current();
    auto c1 = getOptionalChar(iter);
    if (result) {
      result->positions.insert({ pos, ret.size() });
    }
    if (not c1) {
      // EOI
      if (params.quoted()) {
        throw InputFailure(pos, { 0, pos }, fmt::format("Missing terminating {:?} character", params.quote));
      }
      return ret;
    }

    if (auto cp1 = c1->toCodePoint(); cp1) {
      // Single-code-point character

      if (params.quoted() && cp1->eq(params.quote)) {
        // Terminating quote: EOI

        return ret;
      } else if (*cp1 == '\\') {
        // Backslash: this may either be a C-string-escaped character or a hexadecimal sequence starting with
        // "\\x", "\\u", or "\\U"

        // Read another character following the backslash

        auto c2 = getChar(iter);
        auto cp2 = c2.toCodePoint();
        if (not cp2) {
          throw InputFailure(pos, { pos, iter.current() }, "Invalid escape sequence");
        }

        switch (*cp2) {
        case 'a': // Alert = 7
          ret.push_back('\a');
          break;
        case 'b': // Backspace = 8
          ret.push_back('\b');
          break;
        case 't': // Horzontal tab = 9
          ret.push_back('\t');
          break;
        case 'n': // Line feed = 10
          ret.push_back('\n');
          break;
        case 'v': // Vertical tab = 11
          ret.push_back('\v');
          break;
        case 'f': // Form feed = 12
          ret.push_back('\f');
          break;
        case 'r': // Carriage return = 13
          ret.push_back('\r');
          break;
        case 'e': // Escape = 27
          ret.push_back('\e');
          break;
        case '"' : // Quotation mark = 34
        case '\'': // Apostrophe = 39
        case '\\': // Backslash = 92
          ret.push_back(static_cast<char>(*cp2));
          break;
        case 'x': {
          char32 i = getHex(iter, 2);
          ret.append(static_cast<string>(unicode::CodePoint(i)));
          break;
        }
        case 'u': {
          char32 i = getHex(iter, 4);
          ret.append(static_cast<string>(unicode::CodePoint(i)));
          break;
        }
        case 'U': {
          char32 i = getHex(iter, 8);
          ret.append(static_cast<string>(unicode::CodePoint(i)));
          break;
        }
        default: {
          throw InputFailure(pos, { pos, iter.current() }, "Invalid escape sequence");
        }
        }
      } else {
        // No backslash: just add the code point

        ret.append(static_cast<string>(*cp1));
      }
    } else {
      // Multi-code-point character: just add it

      ret.append(*c1);
    }
  }
}

string
escapeRegex(string_view input, Result* result) {
  string ret;
  if (result) {
    result->positions.clear();
  }
  u64 to = 0;

  // Loop through characters

  auto iter = unicode::Iterator(unicode::IteratorType::Character, input);
  u64 column = 0;
  while (true) {
    // Obtain character

    auto current = iter.current();
    auto seg = iter.nextSegment();
    if (seg.empty()) {
      // EOI
      break;
    }
    auto c = unicode::CharacterView<char>(seg);

    if (result) {
      result->positions.insert({ current, to });
    }

    if (auto cp = c.toCodePoint(); cp) {
      // Single-code-point character

      auto escaped = escapeRegexCodePoint(*cp, column);
      ret.append(escaped);
      to += escaped.size();
    } else if (c.crLf()) {
      // CR/LF
      column += 4;
      ret.append("\\r\\n");
      to += 4;
    } else {
      // Multi-code-point character

      column += c.width();
      auto add = static_cast<string_view>(c);
      ret.append(add);
      to += add.size();
    }
  }

  // Add EOI position

  if (result) {
    result->positions.insert({ iter.current(), to });
  }

  return ret;
}

string
unescapeRegex(string_view input, Result* result) {
  string ret;

  if (result) {
    result->positions.clear();
  }

  auto iter = unicode::Iterator(unicode::IteratorType::Character, input);
  while (true) {
    // Read character

    u64 pos = iter.current();
    auto c1 = getOptionalChar(iter);
    if (result) {
      result->positions.insert({ pos, ret.size() });
    }
    if (not c1) {
      // EOF
      return ret;
    }

    if (auto cp1 = c1->toCodePoint(); cp1) {
      // Single-code-point character

      if (*cp1 == '\\') {
        // Backslash: this may either be a regular-expresssion-escaped character or a hexadecimal sequence
        // starting with "\\x" or "\\u"

        // Read another character following the backslash

        auto c2 = getChar(iter);
        auto cp2 = c2.toCodePoint();
        if (not cp2) {
          throw InputFailure(pos, { pos, iter.current() }, "Invalid escape sequence");
        }

        switch (*cp2) {
        case 't': // Horizontal tab = 9
          ret.push_back('\t');
          break;
        case 'n': // Line feed = 10
          ret.push_back('\n');
          break;
        case 'v': // Vertical tab = 11
          ret.push_back('\v');
          break;
        case 'f': // Form feed = 12
          ret.push_back('\f');
          break;
        case 'r': // Carriage return = 13
          ret.push_back('\r');
          break;
        case '$': // Dollar sign = 36
        case '(': // Left parenthesis = 40
        case ')': // Right parenthesis = 41
        case '*': // Asterisk = 42
        case '+': // Plus sign = 43
        case '.': // Dot = 46
        case '?': // Question mark = 63
        case '[': // Left bracket = 91
        case '\\': // Backslash = 92
        case ']': // Right bracket = 93
        case '^': // Circumflex = 94
        case '{': // Left brace = 123
        case '|': // Vertical bar = 124
        case '}': // Right brace = 123
          ret.push_back(static_cast<char>(*cp2));
          break;
        case 'x': {
          char32 i = getHex(iter, 2);
          ret.append(static_cast<string>(unicode::CodePoint(i)));
          break;
        }
        case 'u': {
          char32 i = getHex(iter, 4);
          ret.append(static_cast<string>(unicode::CodePoint(i)));
          break;
        }
        default: {
          throw InputFailure(pos, { pos, iter.current() }, "Invalid escape sequence");
        }
        }
      } else {
        // No backslash: just add the code point

        ret.append(static_cast<string>(*cp1));
      }
    } else {
      // Multi-code-point character: just add it

      ret.append(*c1);
    }
  }
}

} // namespace rocket::str::escape

// EOF
