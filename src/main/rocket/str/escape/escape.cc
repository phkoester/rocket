/*
 * escape.cc
 */

#include "escape.h"

#include "rocket/InputFailure.h"
#include "rocket/assert.h"
#include "rocket/scan.h"
#include "rocket/unicode/Character.h"
#include "rocket/unicode/Iterator.h"

using namespace rocket;
using namespace rocket::str;
using namespace rocket::str::escape;
using namespace std;

namespace {

// Local constants ------------------------------------------------------------------------------------------

/// The characters that have a special meaning in a regular expression and are escaped with a backslash.
constexpr string_view REGEX_SPECIAL_CHARS = "$()*+.?[\\]^{|}";

// Local functions ------------------------------------------------------------------------------------------

string escapeCStringCodePointHex(unicode::CodePoint cp, u64& column, const CStringConfig& config);
string escapeCStringTab(u64& column, const CStringConfig& config);

/**
 * Escapes a grapheme cluster that consists of several code points.
 *
 * A quote or a backslash may be part of such a cluster: it can be followed by combining marks, as in U+0022
 * U+0301, or preceded by a prepending character, as in U+0600 U+0022. These must be escaped, or the output
 * would be malformed. Everything else stays verbatim. In particular, ZERO WIDTH JOINER isn't printable, but
 * must not be turned into a hexadecimal sequence, as that would tear emoji sequences apart.
 *
 * Working on bytes is safe here, because in UTF-8, ASCII bytes never occur inside a multi-byte sequence.
 */
string
escapeCStringCluster(string_view cluster, const CStringConfig& config) {
  string ret;
  ret.reserve(cluster.size() + 1);
  for (const char c : cluster) {
    if (c == '\\' || (config.quoted() && c == config.quote)) {
      ret.push_back('\\');
    }
    ret.push_back(c);
  }
  return ret;
}

string
escapeCStringCodePoint(unicode::CodePoint cp, u64& column, const CStringConfig& config) {
  // Escapable characters
  string ret;
  if (cp >= '\a' && cp <= '\\') {
    // JSON has no short escape sequences for these characters
    if (config.json && (cp == '\a' || cp == '\v' || cp == '\x1b')) {
      return escapeCStringCodePointHex(cp, column, config);
    }

    switch (cp) {
      case '\a': // Alert = 7
        ret = string { '\\', 'a' };
        break;
      case '\b':// Backspace = 8
        ret = string { '\\', 'b' };
        break;
      case '\t':// Horizontal tab = 9
        return escapeCStringTab(column, config);
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
      case '\x1b': // Escape = 27
        ret = string { '\\', 'e' };
        break;
      case '"': // Quotation mark = 34
        ret = config.quote == '"' ? string { '\\', '"' } : string { '"' };
        break;
      case '\'': // Apostrophe = 39
        ret = config.quote == '\'' ? string { '\\', '\'' } : string { '\'' };
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
  return escapeCStringCodePointHex(cp, column, config);
}

string
escapeCStringCodePointHex(unicode::CodePoint cp, u64& column, const CStringConfig& config) {
  string ret;
  if (config.json) {
    // JSON
    if (cp > 0xFFFFU) {
      // We can’t escape code points > U+FFFF with `\u`, so just return the string representation
      ret = static_cast<string>(cp);
      column += cp.width();
      return ret;
    }
    ret = fmt::format("\\u{:0>4X}", static_cast<u32>(cp));
  } else {
    // C
    if (cp > 0xFFFFU) {
      ret = fmt::format("\\U{:0>8X}", static_cast<u32>(cp));
    } else if (cp > 0x00ffU) {
      ret = fmt::format("\\u{:0>4X}", static_cast<u32>(cp));
    } else {
      ret = fmt::format("\\x{:0>2X}", static_cast<u32>(cp));
    }
  }
  column += ret.size();
  return ret;
}

string
escapeCStringTab(u64& column, const CStringConfig& config) {
  if (not config.tabSize) {
    string ret { '\\', 't' };
    column += ret.size();
    return ret;
  }

  const u64 mod = column % *config.tabSize;
  string ret(*config.tabSize - mod, ' ');
  column += ret.size();
  return ret;
}

/**
 * Escapes a grapheme cluster that consists of several code points.
 *
 * Like a quote in a C string, a special character may be part of such a cluster: it can be followed by
 * combining marks or preceded by a prepending character. These must be escaped, or the character would keep
 * its special meaning. Everything else stays verbatim. See also #escapeCStringCluster.
 */
string
escapeRegexCluster(string_view cluster) {
  string ret;
  ret.reserve(cluster.size() + 1);
  for (const char c : cluster) {
    if (REGEX_SPECIAL_CHARS.contains(c)) {
      ret.push_back('\\');
    }
    ret.push_back(c);
  }
  return ret;
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
  return escapeCStringCodePointHex(cp, column, {});
}

unicode::CharacterView<char>
getChar(unicode::Iterator<char>& iter) {
  auto seg = iter.nextSegment();
  if (seg.empty()) {
    throw InputFailure(iter.current(), "Expected character, got EOI");
  }
  return unicode::CharacterView<char>(seg);
}

/**
 * Reads a grapheme cluster that must start with the ASCII character @p expected.
 *
 * @return the rest of the cluster. This is usually empty, but combining marks may follow @p expected
 */
string_view
getChar(unicode::Iterator<char>& iter, char expected) {
  const u64 pos = iter.current();
  auto seg = iter.nextSegment();
  if (seg.empty()) {
    throw InputFailure(pos, fmt::format("Expected character {:?}, got EOI", expected));
  }
  if (seg.front() != expected) {
    unicode::CharacterView<char> c(seg);
    throw InputFailure(pos, fmt::format("Expected character {:?}, got {:?}", expected, c));
  }
  return seg.substr(1);
}

u32
getHex(unicode::Iterator<char>& iter, u64 n) {
  const u64 pos0 = iter.current();

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
    input.append(static_cast<string_view>(c));
  }

  auto result = scn::scan<u32>(input, "{:x}");
  // We know the input string is valid
  return result->value();
}

optional<unicode::CharacterView<char>>
getOptionalChar(unicode::Iterator<char>& iter) {
  auto seg = iter.nextSegment();
  if (seg.empty()) {
    return nullopt;
  }
  return unicode::CharacterView<char>(seg);
}

/**
 * Reads the four hexadecimal digits following a JSON `\u`, combining a surrogate pair into one code point.
 *
 * @param iter the iterator, positioned after the `\u`
 * @param pos the position of the escape sequence
 * @return the code-point value
 * @throw #rocket::InputFailure if the digits are invalid or if a surrogate is unpaired
 */
u32
getJsonCodePoint(unicode::Iterator<char>& iter, u64 pos) {
  const auto unpaired = [&] {
    return InputFailure(pos, { pos, iter.current() }, "Unpaired surrogate in escape sequence");
  };
  const auto nextIs = [&](char expected) {
    const auto c = getOptionalChar(iter);
    return c && c->eq(expected);
  };

  const u32 first = getHex(iter, 4);
  if (first < 0xD800U || first > 0xDFFFU) {
    return first;
  }
  if (first > 0xDBFFU) {
    throw unpaired(); // Low surrogate without a preceding high surrogate
  }

  if (not nextIs('\\') || not nextIs('u')) {
    throw unpaired();
  }
  const u32 second = getHex(iter, 4);
  if (second < 0xDC00U || second > 0xDFFFU) {
    throw unpaired();
  }
  return 0x10000U + ((first - 0xD800U) << 10U) + (second - 0xDC00U);
}

/**
 * Converts the value of a hexadecimal escape sequence to a code point.
 *
 * @param iter the iterator, positioned after the escape sequence
 * @param pos the position of the escape sequence
 * @param value the value of the escape sequence
 * @return the code point
 * @throw #rocket::InputFailure if @p value is not a valid code-point value
 */
unicode::CodePoint
toCodePoint(const unicode::Iterator<char>& iter, u64 pos, u32 value) {
  if (not unicode::CodePoint::valid(static_cast<char32>(value))) {
    throw InputFailure(pos, { pos, iter.current() }, fmt::format("Invalid code-point value 0x{:X}", value));
  }
  return static_cast<char32>(value);
}

} // namespace

// Functions ------------------------------------------------------------------------------------------------

namespace rocket::str::escape {

string
escapeCString(string_view input, const CStringConfig& config, Positions* positions) {
  ROCKET_CHECK(config, config.quote == '\0' || config.quote == '"' || config.quote == '\'');

  string ret;
  if (positions != nullptr) {
    positions->clear();
  }
  u64 to = 0;

  // If needed, add quote

  if (config.quoted()) {
    ret.push_back(config.quote);
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

    if (positions != nullptr) {
      positions->insert({ current, to });
    }

    if (auto cp = c.toCodePoint(); cp) {
      // Single-code-point character

      auto escaped = escapeCStringCodePoint(*cp, column, config);
      ret.append(escaped);
      to += escaped.size();
    } else if (c.crLf()) {
      // CR/LF
      column += 4;
      ret.append("\\r\\n");
      to += 4;
    } else {
      // Multi-code-point character: verbatim, except for a quote or backslash inside it

      const auto add = escapeCStringCluster(seg, config);
      column += c.width() + (add.size() - seg.size()); // Each added backslash takes up one column
      ret.append(add);
      to += add.size();
    }
  }

  // Add EOI position

  if (positions != nullptr) {
    positions->insert({ iter.current(), to });
  }

  // If needed, add quote

  if (config.quoted()) {
    ret.push_back(config.quote);
  }

  return ret;
}

string
unescapeCString(string_view input, const CStringConfig& config, Positions* positions) { // NOLINT(*-complexity)
  ROCKET_CHECK(config, config.quote == '\0' || config.quote == '"' || config.quote == '\'');

  string ret;

  if (positions != nullptr) {
    positions->clear();
  }

  // If needed, read quote

  auto iter = unicode::Iterator(unicode::IteratorType::Character, input);
  if (config.quoted()) {
    // If the string starts with a combining mark, that mark is in the same cluster as the opening quote
    ret.append(getChar(iter, config.quote));
  }

  while (true) {
    // Read character

    u64 pos = iter.current();
    auto c1 = getOptionalChar(iter);
    if (positions != nullptr) {
      positions->insert({ pos, ret.size() });
    }
    if (not c1) {
      // EOI
      if (config.quoted()) {
        throw InputFailure(pos, { 0, pos }, fmt::format("Missing terminating {:?} character", config.quote));
      }
      return ret;
    }

    // A prepending character, such as U+0600, forms one cluster with a following backslash or quote. Add the
    // prepended part, then process the backslash or quote as usual. Comparing bytes is safe, see
    // `escapeCStringCluster`

    auto chars = static_cast<string_view>(*c1);
    if (chars.size() > 1 && (chars.back() == '\\' || (config.quoted() && chars.back() == config.quote))) {
      ret.append(chars.substr(0, chars.size() - 1));
      chars.remove_prefix(chars.size() - 1);
    }

    if (auto cp1 = unicode::CharacterView<char>(chars).toCodePoint(); cp1) {
      // Single-code-point character

      if (config.quoted() && cp1->eq(config.quote)) {
        // Terminating quote: EOI, nothing may follow it

        if (const u64 end = iter.current(); end != input.size()) {
          throw InputFailure(
            end,
            { end, input.size() },
            fmt::format("Unexpected input after terminating {:?} character", config.quote));
        }
        return ret;
      } else if (*cp1 == '\\') { // NOLINT
        // Backslash: this may either be a C-string-escaped character or a hexadecimal sequence starting with
        // "\\x", "\\u", or "\\U"

        // Read another character following the backslash

        auto c2 = getChar(iter);
        auto cp2 = c2.toCodePoint();
        if (not cp2) {
          // An escaped quote or backslash may carry combining marks, which form one cluster with it. These
          // marks are not part of the escape sequence, so add the cluster without the backslash

          const auto cluster = static_cast<string_view>(c2);
          if (cluster.front() != '"' && cluster.front() != '\'' && cluster.front() != '\\') {
            throw InputFailure(pos, { pos, iter.current() }, "Invalid escape sequence");
          }
          ret.append(cluster);
          continue;
        }

        switch (*cp2) {
          case 'a': // Alert = 7
            ret.push_back('\a');
            break;
          case 'b': // Backspace = 8
            ret.push_back('\b');
            break;
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
          case 'e': // Escape = 27
            ret.push_back('\x1b');
            break;
          case '"' : // Quotation mark = 34
          case '\'': // Apostrophe = 39
          case '\\': // Backslash = 92
            ret.push_back(static_cast<char>(*cp2));
            break;
          case '/': // Slash = 47
            if (not config.json) {
              throw InputFailure(pos, { pos, iter.current() }, "Invalid escape sequence");
            }
            ret.push_back('/');
            break;
          case 'x': {
            const char32 i = getHex(iter, 2);
            ret.append(static_cast<string>(unicode::CodePoint(i)));
            break;
          }
          case 'u': {
            const u32 i = config.json ? getJsonCodePoint(iter, pos) : getHex(iter, 4);
            ret.append(static_cast<string>(toCodePoint(iter, pos, i)));
            break;
          }
          case 'U': {
            const u32 i = getHex(iter, 8);
            ret.append(static_cast<string>(toCodePoint(iter, pos, i)));
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

      ret.append(chars);
    }
  }
}

string
escapeRegex(string_view input, Positions* positions) {
  string ret;
  if (positions != nullptr) {
    positions->clear();
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

    if (positions != nullptr) {
      positions->insert({ current, to });
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
      // Multi-code-point character: verbatim, except for a special character inside it

      const auto add = escapeRegexCluster(seg);
      column += c.width() + (add.size() - seg.size()); // Each added backslash takes up one column
      ret.append(add);
      to += add.size();
    }
  }

  // Add EOI position

  if (positions != nullptr) {
    positions->insert({ iter.current(), to });
  }

  return ret;
}

string
unescapeRegex(string_view input, Positions* positions) {
  string ret;

  if (positions != nullptr) {
    positions->clear();
  }

  auto iter = unicode::Iterator(unicode::IteratorType::Character, input);
  while (true) {
    // Read character

    const u64 pos = iter.current();
    auto c1 = getOptionalChar(iter);
    if (positions != nullptr) {
      positions->insert({ pos, ret.size() });
    }
    if (not c1) {
      // EOF
      return ret;
    }

    // A prepending character, such as U+0600, forms one cluster with a following backslash. Add the
    // prepended part, then process the backslash as usual. Comparing bytes is safe, see
    // `escapeCStringCluster`

    auto chars = static_cast<string_view>(*c1);
    if (chars.size() > 1 && chars.back() == '\\') {
      ret.append(chars.substr(0, chars.size() - 1));
      chars.remove_prefix(chars.size() - 1);
    }

    if (auto cp1 = unicode::CharacterView<char>(chars).toCodePoint(); cp1) {
      // Single-code-point character

      if (*cp1 == '\\') {
        // Backslash: this may either be a regular-expression-escaped character or a hexadecimal sequence
        // starting with "\\x" or "\\u"

        // Read another character following the backslash

        auto c2 = getChar(iter);
        auto cp2 = c2.toCodePoint();
        if (not cp2) {
          // An escaped special character may carry combining marks, which form one cluster with it. These
          // marks are not part of the escape sequence, so add the cluster without the backslash

          const auto cluster = static_cast<string_view>(c2);
          if (not REGEX_SPECIAL_CHARS.contains(cluster.front())) {
            throw InputFailure(pos, { pos, iter.current() }, "Invalid escape sequence");
          }
          ret.append(cluster);
          continue;
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
          case '}': // Right brace = 125
            ret.push_back(static_cast<char>(*cp2));
            break;
          case 'x': {
            const char32 i = getHex(iter, 2);
            ret.append(static_cast<string>(unicode::CodePoint(i)));
            break;
          }
          case 'u': {
            const u32 i = getHex(iter, 4);
            ret.append(static_cast<string>(toCodePoint(iter, pos, i)));
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

      ret.append(chars);
    }
  }
}

} // namespace rocket::str::escape

// EOF
