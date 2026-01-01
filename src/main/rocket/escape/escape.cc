/*
 * escape.cc
 */

#include "escape.h"

#include "rocket/InputFailure.h"
#include "rocket/assert.h"
#include "rocket/nio/util.h"
#include "rocket/unicode/unicode.h"

using namespace rocket;
using namespace rocket::escape;
using namespace std;

namespace {

// Local functions ------------------------------------------------------------------------------------------

#if 0
std::string escapeCStringCodePointHex(unicode::CodePoint, size_t&);
std::string escapeCStringTab(size_t&, const CStringParams&);

std::string
escapeCStringCodePoint(unicode::CodePoint cp, size_t& column, const CStringParams& params) {
  // Escapable characters
  std::string ret;
  if (cp >= '\a' && cp <= '\\') {
    switch (cp) {
    case '\a': // Alert = 7
      ret = std::string { '\\', 'a' };
      break;
    case '\b':// Backspace = 8
      ret = std::string { '\\', 'b' };
      break;
    case '\t':// Horizontal tab = 9
      return escapeCStringTab(column, params);
    case '\n': // Line feed = 10
      ret = std::string { '\\', 'n' };
      break;
    case '\v': // Vertical tab = 11
      ret = std::string { '\\', 'v' };
      break;
    case '\f': // Form feed = 12
      ret = std::string { '\\', 'f' };
      break;
    case '\r': // Carriage return = 13
      ret = std::string { '\\', 'r' };
      break;
    case '\e': // Escape = 27
      ret = std::string { '\\', 'e' };
      break;
    case '"': // Quotation mark = 34
      ret = params.quote == '"' ? std::string { '\\', '"' } :std::string { '"' };
      break;
    case '\'': // Apostrophe = 39
      ret = params.quote == '\'' ? std::string { '\\', '\'' } : std::string { '\'' };
      break;
    case '\\': // Backslash = 92
      ret = std::string { '\\', '\\' };
      break;
    }
  }
  if (not ret.empty()) {
    column += ret.size();
    return ret;
  }

  // Printable characters
  int8_t w;
  if (cp.print(&w)) {
    column += static_cast<size_t>(w); // We know `w` > 0
    return static_cast<std::string>(cp);
  }

  // Hex otherwise
  return escapeCStringCodePointHex(cp, column);
}

std::string
escapeCStringCodePointHex(unicode::CodePoint cp, size_t& column) {
  std::string ret;
  if (cp > 0xffffU)
    ret = fmt::format("\\U{:0>8x}", static_cast<uint32_t>(cp));
  else if (cp > 0x00ffU)
    ret = fmt::format("\\u{:0>4x}", static_cast<uint32_t>(cp));
  else
    ret = fmt::format("\\x{:0>2x}", static_cast<uint32_t>(cp));
  column += ret.size();
  return ret;
}

std::string
escapeCStringTab(size_t& column, const CStringParams& params) {
  if (not params.tabSize) {
    std::string ret { '\\', 't' };
    column += ret.size();
    return ret;
  }
  else {
    size_t mod = column % *params.tabSize;
    std::string ret(*params.tabSize - mod, ' ');
    column += ret.size();
    return ret;
  }
}
#endif












#if 0

std::ostream&
operator<<(std::ostream& lhs, const EscapedString<CString>& rhs) {
  ROCKET_CHECK(rhs, rhs.params.quote == '\0' || rhs.params.quote == '"' || rhs.params.quote == '\'');
  std::string_view s = rhs.s;
  const auto& params = rhs.params;
  Result* result = rhs.result;

  if (result) {
    result->input = s;
    result->positions.clear();
  }
  size_t to = 0;

  // If needed, print quote
  if (params.enclosing()) {
    lhs << params.quote;
    ++to;
  }

  // Loop through graphemes
  auto it = unicode::GraphemeIterator<char>(s), end = unicode::GraphemeIterator<char>(s, s.size());
  size_t column = 0;
  for (; it != end; ++it) {
    unicode::Grapheme gr = *it;
    if (result) {
      result->positions.insert({ it.position(), to });
    }
    if (gr.codePoint()) {
      // Single-code-point grapheme
      unicode::CodePoint cp = *gr.codePoint();
      auto escaped = escapeCString(cp, column, params);
      lhs << escaped;
      to += escaped.size();
    } else if (gr.crlf()) {
      // CRLF
      column += 4;
      lhs << "\\r\\n";
      to += 4;
    } else {
      // Multi-code-point grapheme
      column += gr.width;
      auto add = static_cast<std::string>(gr);
      lhs << add;
      to += add.size();
    }
  }
  if (result) {
    result->positions.insert({ it.position(), to });
  }

  // If needed, print quote
  if (params.enclosing())
    lhs << params.quote;
  return lhs;
}

// `EscapedString<Regex>` ...................................................................................

std::string
escapeRegex(unicode::CodePoint cp, size_t& column) {
  // Escapable characters
  std::string ret;
  if (cp >= '\t' && cp <= '}') {
    switch (cp) {
    case '\t': // Horizontal tab = 9
      ret = std::string { '\\', 't' };
      break;
    case '\n': // Line feed = 10
      ret = std::string { '\\', 'n' };
      break;
    case '\v': // Vertical tab = 11
      ret = std::string { '\\', 'v' };
      break;
    case '\f': // Form feed = 12
      ret = std::string { '\\', 'f' };
      break;
    case '\r': // Carriage return = 13
      ret = std::string { '\\', 'r' };
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
      ret = std::string { '\\', static_cast<char>(cp) };
      break;
    }
  }
  if (not ret.empty()) {
    column += ret.size();
    return ret;
  }

  // Printable characters, code points > U+FFFF
  int8_t w;
  if (cp.print(&w) || cp > 0xffffU) {
    if (w < 0)
      w = 0;
    column += static_cast<size_t>(w); // We know `w` >= 0
    return static_cast<std::basic_string<char>>(cp);
  }

  // Hex otherwise (only up to U+FFFF)
  return escapeCStringHex(cp, column);
}

std::istream&
operator>>(std::istream& lhs, const EscapedString<Regex>& rhs) {
  std::string& s = const_cast<std::string&>(rhs.s);
  Result* result = rhs.result;

  s.clear();
  if (result) {
    result->input.clear();
    result->positions.clear();
  }
  size_t inputPos = io::tellg(lhs);

  while (true) {
    // Read grapheme
    size_t pos1 = io::tellg(lhs);
    if (result) {
      result->positions.insert({ pos1 - inputPos, s.size() });
    }
    unicode::Grapheme gr;
    lhs >> gr;
    if (lhs.eof()) {
      // EOF: end of input
      return lhs;
    }
    io::check(lhs);
    size_t pos2 = io::tellg(lhs);

    if (gr.codePoint()) {
      // Single-code-point grapheme

      unicode::CodePoint cp = *gr.codePoint();
      if (cp == '\\') {
        // Backslash: this may either be a regular-expresssion-escaped character or a hexadecimal sequence
        // starting with "\\x" or "\\u"

        if (result) {
          result->input.push_back('\\');
        }

        // Read another grapheme following the backslash

        lhs >> gr;
        if (lhs.eof()) {
          throw io::ParseFailure(lhs, pos2, { pos1, pos2 }, "Expected a Unicode grapheme, got EOF");
        }
        io::check(lhs);

        if (not gr.codePoint()) {
          throw io::ParseFailure(lhs, pos1, { pos1, io::tellg(lhs) }, "Invalid escape sequence");
        }
        cp = *gr.codePoint();
        switch (cp) {
        case 't': // Horizontal tab = 9
          s.push_back('\t');
          if (result) {
            result->input.push_back('t');
          }
          break;
        case 'n': // Line feed = 10
          s.push_back('\n');
          if (result) {
            result->input.push_back('n');
          }
          break;
        case 'v': // Vertical tab = 11
          s.push_back('\v');
          if (result) {
            result->input.push_back('v');
          }
          break;
        case 'f': // Form feed = 12
          s.push_back('\f');
          if (result) {
            result->input.push_back('f');
          }
          break;
        case 'r': // Carriage return = 13
          s.push_back('\r');
          if (result) {
            result->input.push_back('r');
          }
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
          s.push_back(static_cast<char>(cp));
          if (result) {
            result->input.push_back(static_cast<char>(cp));
          }
          break;
        case 'x': {
          std::string input;
          uint32_t i = io::getHex<uint32_t>(lhs, 2, input);
          if (result) {
            result->input.push_back('x');
            result->input.append(input);
          }
          s.append(static_cast<std::string>(unicode::CodePoint(i)));
          break;
        }
        case 'u': {
          std::string input;
          uint32_t i = io::getHex<uint32_t>(lhs, 4, input);
          if (result) {
            result->input.push_back('u');
            result->input.append(input);
          }
          s.append(static_cast<std::string>(unicode::CodePoint(i)));
          break;
        }
        default: {
          throw io::ParseFailure(lhs, pos1, { pos1, io::tellg(lhs) }, "Invalid escape sequence");
        }
        }
      } else {
        // No backslash: just add the code point

        auto add = static_cast<std::string>(cp);
        s.append(add);
        if (result) {
          result->input.append(add);
        }
      }
    } else {
      // Multi-code-point grapheme: just add it

      auto add = static_cast<std::string>(gr);
      s.append(add);
      if (result) {
        result->input.append(add);
      }
    }
  }
}

std::ostream&
operator<<(std::ostream& lhs, const EscapedString<Regex>& rhs) {
  std::string_view s = rhs.s;
  Result* result = rhs.result;

  if (result) {
    result->input = s;
    result->positions.clear();
  }
  size_t to = 0;

  // Loop through graphemes
  auto it = unicode::GraphemeIterator<char>(s), end = unicode::GraphemeIterator<char>(s, s.size());
  size_t column = 0;
  for (; it != end; ++it) {
    unicode::Grapheme gr = *it;
    if (result) {
      result->positions.insert({ it.position(), to });
    }
    if (gr.codePoint()) {
      // Single-code-point grapheme
      unicode::CodePoint cp = *gr.codePoint();
      auto escaped = escapeRegex(cp, column);
      lhs << escaped;
      to += escaped.size();
    } else if (gr.crlf()) {
      // CRLF
      column += 4;
      lhs << "\\r\\n";
      to += 4;
    } else {
      // Multi-code-point grapheme
      column += gr.width;
      auto add = static_cast<std::string>(gr);
      lhs << add;
      to += add.size();
    }
  }
  if (result) {
    result->positions.insert({ it.position(), to });
  }

  return lhs;
}
#endif

} // namespace

#if 0
namespace rocket::escape {

string
unescapeCString(const string& input, const CStringParams& params, Result* result) {
  ROCKET_CHECK(params, params.quote == '\0' || params.quote == '"' || params.quote == '\'');

  string ret;

  if (result) {
    result->positions.clear();
  }
  nio::StringSource in(input);
  char c;

  // If needed, read quote
  if (params.enclosing()) {
    nio::getChar(in, c, params.quote);
  }

  while (true) {
    // Test char
    size_t pos1 = in.tell();
    if (nio::getChar(in, c) == 0) {
      // EOF: end of input
      if (params.enclosing()) {
        throw InputFailure(pos1, { 0, pos1 },
            fmt::format("Missing terminating {:?} character", params.quote));
      }
      return ret;
    }

    // Terminating quote?
    if (params.enclosing() && c == params.quote) {
    }

    // Read grapheme
    auto pos1 = in.tell();
    if (result) {
      result->positions.insert({ pos1, ret.size() });
    }
    unicode::Grapheme gr = nio::getGrapheme(in);
    in >> gr;
    if (in.eof()) {
      // EOF: end of input
      if (params.enclosing()) {
        throw io::ParseFailure(lhs, pos1, { inputPos, pos1 },
            fmt::format("Missing terminating {:?} character", params.quote)); // XXX ''?
      }
      return lhs;
    }
    io::check(lhs);
    size_t pos2 = io::tellg(lhs);

    if (gr.codePoint()) {
      // Single-code-point grapheme

      unicode::CodePoint cp = *gr.codePoint();
      if (params.enclosing() && cp == static_cast<uint32_t>(params.quote)) {
        // Terminating quote: end of input

        if (result) {
          result->input.push_back(params.quote);
        }
        return lhs;
      } else if (cp == '\\') {
        // Backslash: this may either be a C-string-escaped character or a hexadecimal sequence starting with
        // "\\x", "\\u", or "\\U"

        if (result) {
          result->input.push_back('\\');
        }

        // Read another grapheme following the backslash

        lhs >> gr;
        if (lhs.eof()) {
          throw io::ParseFailure(lhs, pos2, { pos1, pos2 }, "Expected a Unicode grapheme, got EOF");
        }
        io::check(lhs);

        if (not gr.codePoint()) {
          throw io::ParseFailure(lhs, pos1, { pos1, io::tellg(lhs) }, "Invalid escape sequence");
        }
        cp = *gr.codePoint();
        switch (cp) {
        case 'a': // Alert = 7
          ret.push_back('\a');
          if (result) {
            result->input.push_back('a');
          }
          break;
        case 'b': // Backspace = 8
          ret.push_back('\b');
          if (result) {
            result->input.push_back('b');
          }
          break;
        case 't': // Horzontal tab = 9
          ret.push_back('\t');
          if (result) {
            result->input.push_back('t');
          }
          break;
        case 'n': // Line feed = 10
          ret.push_back('\n');
          if (result) {
            result->input.push_back('n');
          }
          break;
        case 'v': // Vertical tab = 11
          ret.push_back('\v');
          if (result) {
            result->input.push_back('v');
          }
          break;
        case 'f': // Form feed = 12
          ret.push_back('\f');
          if (result) {
            result->input.push_back('f');
          }
          break;
        case 'r': // Carriage return = 13
          ret.push_back('\r');
          if (result) {
            result->input.push_back('r');
          }
          break;
        case 'e': // Escape = 27
          ret.push_back('\e');
          if (result) {
            result->input.push_back('e');
          }
          break;
        case '"' : // Quotation mark = 34
        case '\'': // Apostrophe = 39
        case '\\': // Backslash = 92
          ret.push_back(static_cast<char>(cp));
          if (result) {
            result->input.push_back(static_cast<char>(cp));
          }
          break;
        case 'x': {
          std::string input;
          uint32_t i = io::getHex<uint32_t>(lhs, 2, input);
          if (result) {
            result->input.push_back('x');
            result->input.append(input);
          }
          ret.append(static_cast<std::string>(unicode::CodePoint(i)));
          break;
        }
        case 'u': {
          std::string input;
          uint32_t i = io::getHex<uint32_t>(lhs, 4, input);
          if (result) {
            result->input.push_back('u');
            result->input.append(input);
          }
          ret.append(static_cast<std::string>(unicode::CodePoint(i)));
          break;
        }
        case 'U': {
          std::string input;
          uint32_t i = io::getHex<uint32_t>(lhs, 8, input);
          if (result) {
            result->input.push_back('U');
            result->input.append(input);
          }
          ret.append(static_cast<std::string>(unicode::CodePoint(i)));
          break;
        }
        default: {
          throw io::ParseFailure(lhs, pos1, { pos1, io::tellg(lhs) }, "Invalid escape sequence");
        }
        }
      } else {
        // No backslash: just add the code point

        auto add = static_cast<std::string>(cp);
        ret.append(add);
        if (result) {
          result->input.append(add);
        }
      }
    } else {
      // Multi-code-point grapheme: just add it

      auto add = static_cast<std::string>(gr);
      ret.append(add);
      if (result) {
        result->input.append(add);
      }
    }
  }
}

} // namespace rocket::escape
#endif

// XXX
std::string rocket::escape::escapeCString(std::string_view input, const CStringParams& params, Result* result) {
  return "Hello from escapeCString!";
}

// EOF
