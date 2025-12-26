/**
 * @file escape.h
 *
 * Escaped strings, offering an interface similar to `std::quoted`.
 */

#pragma once

#include "Positions.h"
#include "S.h"
#include "assert.h"
#include "base.h"
#include "except.h"
#include "unicode-iterator.h"

#include <fmt/format.h>

#include <optional>
#include <string>

namespace rocket::escape {

// `CString` ------------------------------------------------------------------------------------------------

/**
 * C-string escaping.
 */
struct CString {
  /**
   * Parameters for C-string escaping.
   */
  struct Params {
    /**
     * Is `true` if the escaped string is to be enclosed in #quote characters.
     */
    bool enclosed = false;
    /**
     * The quote character to escape.
     *
     * This must be <code>'\0'</code>, <code>'"'</code>, or <code>'\''</code>, otherwise it is invalid.
     */
    char quote = '\0';
    /**
     * Configures the handling of tab characters.
     *
     * If this is null, then tab characters are escaped as `"\\t"`. Otherwise, a tab expands to at most
     * #tabSize spaces.
     */
    std::optional<size_t> tabSize;

    /**
     * Returns `true` if the escaped string is actually to be enclosed.
     *
     * @return `true` if the escaped string is actually to be enclosed
     */
    inline bool enclosing() const { return enclosed && quote != '\0'; }
  };
};

// `Regex` --------------------------------------------------------------------------------------------------

/**
 * Regular-expression escaping.
 */
struct Regex {
  /**
   * Parameters for regular-expression escaping.
   */
  struct Params {};
};

// `Result` -------------------------------------------------------------------------------------------------

/**
 * The result of an escape/unescape operation.
 *
 * @tparam C the character type
 */
template<typename C> requires Character<C>
struct Result {
  /**
   * The input of the escape/unescape operation.
   */
  std::basic_string<C> input;
  /**
   * Translated positions after escaping/unescaping.
   *
   * For each grapheme in the input string and for end-of-string, its character offset---i.e. either its
   * `char` or `char32_t` offset---, is mapped to a character offset in the output string.
   */
  Positions positions;
};

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

template<typename Schema, typename C, typename String>
struct EscapedString {
  static_assert(std::is_reference_v<String>);

  String s;
  Schema::Params params;
  Result<C>* result;

  EscapedString(
      String s,
      const Schema::Params& params,
      Result<C>* result) : s(s), params(params), result(result) {}
};

// `EscapedString<CString, ...>` ............................................................................

template<typename C> requires Character<C>
    std::basic_string<C> escapeCStringHex(unicode::CodePoint, size_t&);
template<typename C> requires Character<C>
    std::basic_string<C> escapeCStringTab(size_t&, const CString::Params&);

template<typename C> requires Character<C>
std::basic_string<C>
escapeCString(unicode::CodePoint cp, size_t& column, const CString::Params& params) {
  using String = std::basic_string<C>;

  // Escapable characters
  String ret;
  if (cp >= '\a' && cp <= '\\') {
    switch (cp) {
    case '\a': // Alert = 7
      ret = String { '\\', 'a' };
      break;
    case '\b':// Backspace = 8
      ret = String { '\\', 'b' };
      break;
    case '\t':// Horizontal tab = 9
      return escapeCStringTab<C>(column, params);
    case '\n': // Line feed = 10
      ret = String { '\\', 'n' };
      break;
    case '\v': // Vertical tab = 11
      ret = String { '\\', 'v' };
      break;
    case '\f': // Form feed = 12
      ret = String { '\\', 'f' };
      break;
    case '\r': // Carriage return = 13
      ret = String { '\\', 'r' };
      break;
    case '\e': // Escape = 27
      ret = String { '\\', 'e' };
      break;
    case '"': // Quotation mark = 34
      ret = params.quote == '"' ? String { '\\', '"' } : String { '"' };
      break;
    case '\'': // Apostrophe = 39
      ret = params.quote == '\'' ? String { '\\', '\'' } : String { '\'' };
      break;
    case '\\': // Backslash = 92
      ret = String { '\\', '\\' };
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
    return static_cast<std::basic_string<C>>(cp);
  }

  // Hex otherwise
  return escapeCStringHex<C>(cp, column);
}

template<typename C> requires Character<C>
std::basic_string<C>
escapeCStringHex(unicode::CodePoint cp, size_t& column) {
  // `std::format` doesn't support `char32_t`. Fortunately, we output ASCII only
  std::string s;
  if (cp > 0xffffU)
    s = fmt::format("\\U{:0>8x}", static_cast<uint32_t>(cp));
  else if (cp > 0x00ffU)
    s = fmt::format("\\u{:0>4x}", static_cast<uint32_t>(cp));
  else
    s = fmt::format("\\x{:0>2x}", static_cast<uint32_t>(cp));
  column += s.size();
  if constexpr (std::is_same_v<C, char>)
    return s;
  else
    return unicode::asciiTo32(s);
}

template<typename C> requires Character<C>
std::basic_string<C>
escapeCStringTab(size_t& column, const CString::Params& params) {
  using String = std::basic_string<C>;

  if (not params.tabSize) {
    String ret { '\\', 't' };
    column += ret.size();
    return ret;
  }
  else {
    size_t mod = column % *params.tabSize;
    String ret(*params.tabSize - mod, ' ');
    column += ret.size();
    return ret;
  }
}

template<typename C, typename String>
std::basic_istream<C>&
operator>>(std::basic_istream<C>& lhs, const EscapedString<CString, C, String>& rhs) {
  ROCKET_CHECK(rhs, rhs.params.quote == '\0' || rhs.params.quote == '"' || rhs.params.quote == '\'');
  std::basic_string<C>& s = rhs.s;
  const auto& params = rhs.params;
  Result<C>* result = rhs.result;

  s.clear();
  if (result) {
    result->input.clear();
    result->positions.clear();
  }
  size_t inputPos = io::tellg(lhs);

  // If needed, read quote
  if (params.enclosing()) {
    io::getChar(lhs, static_cast<C>(params.quote));
    if (result)
      result->input.push_back(params.quote);
  }

  while (true) {
    // Read grapheme
    size_t pos1 = io::tellg(lhs);
    if (result)
      result->positions.insert({ pos1 - inputPos, s.size() });
    unicode::Grapheme gr;
    lhs >> gr;
    if (lhs.eof()) {
      // EOF: end of input
      if (params.enclosing()) {
        throw except::ParseFailure<C>(
            lhs, pos1, { inputPos, pos1 }, S << "Missing terminating " << params.quote << " character");
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

        if (result)
          result->input.push_back(params.quote);
        return lhs;
      } else if (cp == '\\') {
        // Backslash: this may either be a C-string-escaped character or a hexadecimal sequence starting with
        // "\\x", "\\u", or "\\U"

        if (result)
          result->input.push_back('\\');

        // Read another grapheme following the backslash

        lhs >> gr;
        if (lhs.eof())
          throw except::ParseFailure<C>(lhs, pos2, { pos1, pos2 }, "Expected a Unicode grapheme, got EOF");
        io::check(lhs);

        if (not gr.codePoint())
          throw except::ParseFailure<C>(lhs, pos1, { pos1, io::tellg(lhs) }, "Invalid escape sequence");          ;
        cp = *gr.codePoint();
        switch (cp) {
        case 'a': // Alert = 7
          s.push_back('\a');
          if (result)
            result->input.push_back('a');
          break;
        case 'b': // Backspace = 8
          s.push_back('\b');
          if (result)
            result->input.push_back('b');
          break;
        case 't': // Horzontal tab = 9
          s.push_back('\t');
          if (result)
            result->input.push_back('t');
          break;
        case 'n': // Line feed = 10
          s.push_back('\n');
          if (result)
            result->input.push_back('n');
          break;
        case 'v': // Vertical tab = 11
          s.push_back('\v');
          if (result)
            result->input.push_back('v');
          break;
        case 'f': // Form feed = 12
          s.push_back('\f');
          if (result)
            result->input.push_back('f');
          break;
        case 'r': // Carriage return = 13
          s.push_back('\r');
          if (result)
            result->input.push_back('r');
          break;
        case 'e': // Escape = 27
          s.push_back('\e');
          if (result)
            result->input.push_back('e');
          break;
        case '"' : // Quotation mark = 34
        case '\'': // Apostrophe = 39
        case '\\': // Backslash = 92
          s.push_back(static_cast<C>(cp));
          if (result)
            result->input.push_back(static_cast<C>(cp));
          break;
        case 'x': {
          std::basic_string<C> input;
          uint32_t i = io::getHex<uint32_t>(lhs, 2, input);
          if (result) {
            result->input.push_back('x');
            result->input.append(input);
          }
          s.append(static_cast<std::basic_string<C>>(unicode::CodePoint(i)));
          break;
        }
        case 'u': {
          std::basic_string<C> input;
          uint32_t i = io::getHex<uint32_t>(lhs, 4, input);
          if (result) {
            result->input.push_back('u');
            result->input.append(input);
          }
          s.append(static_cast<std::basic_string<C>>(unicode::CodePoint(i)));
          break;
        }
        case 'U': {
          std::basic_string<C> input;
          uint32_t i = io::getHex<uint32_t>(lhs, 8, input);
          if (result) {
            result->input.push_back('U');
            result->input.append(input);
          }
          s.append(static_cast<std::basic_string<C>>(unicode::CodePoint(i)));
          break;
        }
        default: throw except::ParseFailure<C>(lhs, pos1, { pos1, io::tellg(lhs) }, "Invalid escape sequence");
        }
      } else {
        // No backslash: just add the code point

        auto add = static_cast<std::basic_string<C>>(cp);
        s.append(add);
        if (result)
          result->input.append(add);
      }
    } else {
      // Multi-code-point grapheme: just add it

      auto add = static_cast<std::basic_string<C>>(gr);
      s.append(add);
      if (result)
        result->input.append(add);
    }
  }
}

template<typename C, typename String>
std::basic_ostream<C>&
operator<<(std::basic_ostream<C>& lhs, const EscapedString<CString, C, String>& rhs) {
  ROCKET_CHECK(rhs, rhs.params.quote == '\0' || rhs.params.quote == '"' || rhs.params.quote == '\'');
  std::basic_string_view<C> s = rhs.s;
  const auto& params = rhs.params;
  Result<C>* result = rhs.result;

  if (result) {
    result->input = s;
    result->positions.clear();
  }
  size_t to = 0;

  // If needed, print quote
  if (params.enclosing()) {
    lhs << static_cast<C>(params.quote);
    ++to;
  }

  // Loop through graphemes
  auto it = unicode::GraphemeIterator<C>(s), end = unicode::GraphemeIterator<C>(s, s.size());
  size_t column = 0;
  for (; it != end; ++it) {
    unicode::Grapheme gr = *it;
    if (result)
      result->positions.insert({ it.position(), to });
    if (gr.codePoint()) {
      // Single-code-point grapheme
      unicode::CodePoint cp = *gr.codePoint();
      auto escaped = escapeCString<C>(cp, column, params);
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
      auto add = static_cast<std::basic_string<C>>(gr);
      lhs << add;
      to += add.size();
    }
  }
  if (result)
    result->positions.insert({ it.position(), to });

  // If needed, print quote
  if (params.enclosing())
    lhs << static_cast<C>(params.quote);
  return lhs;
}

// `EscapedString<Regex, ...>` ..............................................................................

template<typename C> requires Character<C>
std::basic_string<C>
escapeRegex(unicode::CodePoint cp, size_t& column) {
  using String = std::basic_string<C>;

  // Escapable characters
  String ret;
  if (cp >= '\t' && cp <= '}') {
    switch (cp) {
    case '\t': // Horizontal tab = 9
      ret = String { '\\', 't' };
      break;
    case '\n': // Line feed = 10
      ret = String { '\\', 'n' };
      break;
    case '\v': // Vertical tab = 11
      ret = String { '\\', 'v' };
      break;
    case '\f': // Form feed = 12
      ret = String { '\\', 'f' };
      break;
    case '\r': // Carriage return = 13
      ret = String { '\\', 'r' };
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
      ret = String { '\\', static_cast<C>(cp) };
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
    return static_cast<std::basic_string<C>>(cp);
  }

  // Hex otherwise (only up to U+FFFF)
  return escapeCStringHex<C>(cp, column);
}

template<typename C, typename String>
std::basic_istream<C>&
operator>>(std::basic_istream<C>& lhs, const EscapedString<Regex, C, String>& rhs) {
  std::basic_string<C>& s = rhs.s;
  Result<C>* result = rhs.result;

  s.clear();
  if (result) {
    result->input.clear();
    result->positions.clear();
  }
  size_t inputPos = io::tellg(lhs);

  while (true) {
    // Read grapheme
    size_t pos1 = io::tellg(lhs);
    if (result)
      result->positions.insert({ pos1 - inputPos, s.size() });
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

        if (result)
          result->input.push_back('\\');

        // Read another grapheme following the backslash

        lhs >> gr;
        if (lhs.eof())
          throw except::ParseFailure<C>(lhs, pos2, { pos1, pos2 }, "Expected a Unicode grapheme, got EOF");
        io::check(lhs);

        if (not gr.codePoint())
          throw except::ParseFailure<C>(lhs, pos1, { pos1, io::tellg(lhs) }, "Invalid escape sequence");          ;
        cp = *gr.codePoint();
        switch (cp) {
        case 't': // Horizontal tab = 9
          s.push_back('\t');
          if (result)
            result->input.push_back('t');
          break;
        case 'n': // Line feed = 10
          s.push_back('\n');
          if (result)
            result->input.push_back('n');
          break;
        case 'v': // Vertical tab = 11
          s.push_back('\v');
          if (result)
            result->input.push_back('v');
          break;
        case 'f': // Form feed = 12
          s.push_back('\f');
          if (result)
            result->input.push_back('f');
          break;
        case 'r': // Carriage return = 13
          s.push_back('\r');
          if (result)
            result->input.push_back('r');
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
          s.push_back(static_cast<C>(cp));
          if (result)
            result->input.push_back(static_cast<C>(cp));
          break;
        case 'x': {
          std::basic_string<C> input;
          uint32_t i = io::getHex<uint32_t>(lhs, 2, input);
          if (result) {
            result->input.push_back('x');
            result->input.append(input);
          }
          s.append(static_cast<std::basic_string<C>>(unicode::CodePoint(i)));
          break;
        }
        case 'u': {
          std::basic_string<C> input;
          uint32_t i = io::getHex<uint32_t>(lhs, 4, input);
          if (result) {
            result->input.push_back('u');
            result->input.append(input);
          }
          s.append(static_cast<std::basic_string<C>>(unicode::CodePoint(i)));
          break;
        }
        default: throw except::ParseFailure<C>(lhs, pos1, { pos1, io::tellg(lhs) }, "Invalid escape sequence");
        }
      } else {
        // No backslash: just add the code point

        auto add = static_cast<std::basic_string<C>>(cp);
        s.append(add);
        if (result)
          result->input.append(add);
      }
    } else {
      // Multi-code-point grapheme: just add it

      auto add = static_cast<std::basic_string<C>>(gr);
      s.append(add);
      if (result)
        result->input.append(add);
    }
  }
}

template<typename C, typename String>
std::basic_ostream<C>&
operator<<(std::basic_ostream<C>& lhs, const EscapedString<Regex, C, String>& rhs) {
  std::basic_string_view<C> s = rhs.s;
  Result<C>* result = rhs.result;

  if (result) {
    result->input = s;
    result->positions.clear();
  }
  size_t to = 0;

  // Loop through graphemes
  auto it = unicode::GraphemeIterator<C>(s), end = unicode::GraphemeIterator<C>(s, s.size());
  size_t column = 0;
  for (; it != end; ++it) {
    unicode::Grapheme gr = *it;
    if (result)
      result->positions.insert({ it.position(), to });
    if (gr.codePoint()) {
      // Single-code-point grapheme
      unicode::CodePoint cp = *gr.codePoint();
      auto escaped = escapeRegex<C>(cp, column);
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
      auto add = static_cast<std::basic_string<C>>(gr);
      lhs << add;
      to += add.size();
    }
  }
  if (result)
    result->positions.insert({ it.position(), to });

  return lhs;
}

} // namespace internal

// `Escaped` ------------------------------------------------------------------------------------------------

/**
 * A concept for valid escaping schemata.
 *
 * Currently, these are
 *
 * - #rocket::escape::CString
 * - #rocket::escape::Regex
 *
 * @tparam Schema the escaping schema
 * @tparam C the character type
 */
template<typename Schema, typename C>
concept Escaped =
    (std::is_same_v<Schema, CString> || std::is_same_v<Schema, Regex>) &&
    Character<C>;

// Functions ------------------------------------------------------------------------------------------------

/**
 * `escaped` overload with a nonconst string reference.
 *
 * Use this function to unescape a string with `operator>>`.
 *
 * @tparam Schema the escaping Schema
 * @tparam C the character type
 * @param s a nonconst string reference
 * @param params parameters for unescaping
 * @param result pointer to a Result instance. If nonnull, then the members of this Result are initialized
 * @return an internal representation of an escaped string
 *
 * ## Examples
 *
 * ```
 * using namespace rocket;
 * using namespace std;
 *
 * stringstream ss;
 * string in = "a\"b";
 * escaped::CString::Params params { .enclosed=true, .quote='"' };
 * ss << escaped::escaped<escaped::CString>(in, params);
 * cout << ss.str() << '\n'; // Quotation mark, 'a', backslash, quotation mark, 'b'
 * string out;
 * ss >> escaped::escaped<escaped::CString>(out, params);
 * assert(out == in); // After unescaping, `out` equals `in`
 * ```
 */
template<typename Schema, typename C> requires Escaped<Schema, C>
internal::EscapedString<Schema, C, std::basic_string<C>&>
escaped(
    std::basic_string<C>& s,
    const typename Schema::Params& params = {},
    Result<C>* result = nullptr) {
  return internal::EscapedString<Schema, C, std::basic_string<C>&>(s, params, result);
}

/**
 * `escaped` overload with a const string reference.
 *
 * Use this function to escape a string with `operator<<`.
 *
 * @tparam Schema the escaping Schema
 * @tparam C the character type
 * @param s a const string reference
 * @param params parameters for escaping
 * @param result pointer to a Result instance. If nonnull, then the members of this Result are initialized
 * @return an internal representation of an escaped string
 *
 * ## Examples
 *
 * ```
 * using namespace rocket;
 * using namespace std;
 *
 * stringstream ss;
 * string in = "a\"b";
 * escaped::CString::Params params { .enclosed=true, .quote='"' };
 * ss << escaped::escaped<escaped::CString>(in, params);
 * cout << ss.str() << '\n'; // ", a, \, ", b, "
 * string out;
 * ss >> escaped::escaped<escaped::CString>(out, params);
 * assert(out == in); // After unescaping, `out` equals `in`
 * ```
 */
template<typename Schema, typename C> requires Escaped<Schema, C>
internal::EscapedString<Schema, C, const std::basic_string<C>&>
escaped(
    const std::basic_string<C>& s,
    const typename Schema::Params& params = {},
    Result<C>* result = nullptr) {
  return internal::EscapedString<Schema, C, const std::basic_string<C>&>(s, params, result);
}

} // namespace rocket::escape

// EOF
