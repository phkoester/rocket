/*
 * escape.cc
 */

#include "escape.h"

#include "rocket/InputFailure.h"
#include "rocket/assert.h"
#include "rocket/nio/util.h"
#include "rocket/unicode/iterator.h"

using namespace rocket;
using namespace rocket::escape;
using namespace std;

namespace {

// Local functions ------------------------------------------------------------------------------------------

string escapeCStringCodePointHex(unicode::CodePoint, size_t&);
string escapeCStringTab(size_t&, const CStringParams&);

string
escapeCStringCodePoint(unicode::CodePoint cp, size_t& column, const CStringParams& params) {
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
  int8_t w;
  if (cp.print(&w)) {
    column += static_cast<size_t>(w); // We know `w` > 0
    return static_cast<string>(cp);
  }

  // Hex otherwise
  return escapeCStringCodePointHex(cp, column);
}

string
escapeCStringCodePointHex(unicode::CodePoint cp, size_t& column) {
  string ret;
  if (cp > 0xffffU)
    ret = fmt::format("\\U{:0>8x}", static_cast<uint32_t>(cp));
  else if (cp > 0x00ffU)
    ret = fmt::format("\\u{:0>4x}", static_cast<uint32_t>(cp));
  else
    ret = fmt::format("\\x{:0>2x}", static_cast<uint32_t>(cp));
  column += ret.size();
  return ret;
}

string
escapeCStringTab(size_t& column, const CStringParams& params) {
  if (not params.tabSize) {
    string ret { '\\', 't' };
    column += ret.size();
    return ret;
  }
  else {
    size_t mod = column % *params.tabSize;
    string ret(*params.tabSize - mod, ' ');
    column += ret.size();
    return ret;
  }
}

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

namespace rocket::escape {

string
escapeCString(string_view input, const CStringParams& params, Result* result) {
  ROCKET_CHECK(params, params.quote == '\0' || params.quote == '"' || params.quote == '\'');

  string ret;
  if (result) {
    result->positions.clear();
  }
  size_t to = 0;

  // If needed, print quote

  if (params.enclosing()) {
    ret.push_back(params.quote);
    ++to;
  }

  // Loop through graphemes

  auto it = unicode::GraphemeIterator(input), end = unicode::GraphemeIterator(input, input.size());
  size_t column = 0;
  for (; it != end; ++it) {
    // Obtain grapheme

    unicode::Grapheme gr = *it;
    if (result) {
      result->positions.insert({ it.position(), to });
    }

    if (gr.codePoint()) {
      // Single-code-point grapheme

      unicode::CodePoint cp = *gr.codePoint();
      auto escaped = escapeCStringCodePoint(cp, column, params);
      ret.append(escaped);
      to += escaped.size();
    } else if (gr.crlf()) {
      // CRLF
      column += 4;
      ret.append("\\r\\n");
      to += 4;
    } else {
      // Multi-code-point grapheme

      column += gr.width;
      auto add = static_cast<string>(gr);
      ret.append(add);
      to += add.size();
    }
  }

  // If needed, print quote

  if (params.enclosing()) {
    ret.push_back(params.quote);
  }

  return ret;
}

string
unescapeCString(const string& input, const CStringParams& params, Result* result) {
  ROCKET_CHECK(params, params.quote == '\0' || params.quote == '"' || params.quote == '\'');

  string ret;
  nio::StringSource in(input);
  if (result) {
    result->positions.clear();
  }

  // If needed, read quote

  if (params.enclosing()) {
    nio::getChar(in, params.quote);
  }

  while (true) {
    // Read grapheme

    size_t pos = in.tell();
    auto gr1 = nio::getOptionalGrapheme(in);

    if (not gr1) {
      // EOF
      if (params.enclosing()) {
        throw InputFailure(pos, { 0, pos }, fmt::format("Missing terminating {:?} character", params.quote));
      }
      return ret;
    }
    if (result) {
      result->positions.insert({ pos, ret.size() });
    }

    if (gr1->codePoint()) {
      // Single-code-point grapheme

      unicode::CodePoint cp1 = *gr1->codePoint();
      if (params.enclosing() && cp1 == params.quote) {
        // Terminating quote: end of input

        return ret;
      } else if (cp1 == '\\') {
        // Backslash: this may either be a C-string-escaped character or a hexadecimal sequence starting with
        // "\\x", "\\u", or "\\U"

        // Read another grapheme following the backslash

        unicode::Grapheme gr2 = nio::getGrapheme(in);

        if (not gr2.codePoint()) {
          throw InputFailure(pos, { pos, in.tell() }, "Invalid escape sequence");
        }
        unicode::CodePoint cp2 = *gr2.codePoint();
        switch (cp2) {
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
          ret.push_back(static_cast<char>(cp2));
          break;
        case 'x': {
          auto i = nio::getHex(in, 2);
          ret.append(static_cast<string>(unicode::CodePoint(i)));
          break;
        }
        case 'u': {
          auto i = nio::getHex(in, 4);
          ret.append(static_cast<string>(unicode::CodePoint(i)));
          break;
        }
        case 'U': {
          auto i = nio::getHex(in, 8);
          ret.append(static_cast<string>(unicode::CodePoint(i)));
          break;
        }
        default: {
          throw InputFailure(pos, { pos, in.tell() }, "Invalid escape sequence");
        }
        }
      } else {
        // No backslash: just add the code point

        ret.append(static_cast<string>(cp1));
      }
    } else {
      // Multi-code-point grapheme: just add it

      ret.append(static_cast<string>(*gr1));
    }
  }
}

// XXX
string
escapeRegex(string_view input, Result* result) {
  return "Hello from escapeRegex!";
}

// XXX
string
unescapeRegex(string_view input, Result* result) {
  return "Hello from unescapeRegex!";
}

} // namespace rocket::escape

// EOF
