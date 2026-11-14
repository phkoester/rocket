/*
 * nio-utils.cc
 */

#include "rocket/InputFailure.h"
#include "rocket/assert.h"
#include "rocket/nio/nio-utils.h"
#include "rocket/unicode/Iterator.h"
#include "rocket/unicode/unicode.h"

#include <exception>

using namespace rocket;
using namespace std;

// Local functions ------------------------------------------------------------------------------------------

namespace {

/// The message of the #rocket::InputFailure thrown on an I/O error.
constexpr string_view IO_ERROR = "Cannot read from the source";
/// The message of the #rocket::InputFailure thrown on invalid UTF-8.
constexpr string_view INVALID_UTF8 = "Invalid UTF-8";

optional<unicode::CodePoint> readValidCodePoint(nio::Source& in);

void rewindTo(nio::Source& in, optional<u64> pos, u64 offset = 0);

bool isWordCodePoint(unicode::CodePoint cp);

/// Returns whether @p c is an ASCII character that can be part of a word.
bool
isAsciiWordChar(char c) {
  return static_cast<u8>(c) < 0x80 && isWordCodePoint(static_cast<char32>(static_cast<u8>(c)));
}

/// Returns whether @p next, a complete code point, continues the character @p character.
bool
continuesCharacter(string_view character, string_view next) {
  const string str = string(character) + string(next);
  unicode::Iterator<char> iter(unicode::IteratorType::Character, str);
  return iter.next() != character.size();
}

bool
isEndOfWord(string_view str, u64 pos) {
  if (pos >= str.size()) {
    // End of string: we have a word boundary
    return true;
  }

  try {
    const auto cp = unicode::nextCodePoint(str, pos);
    // Next code point is not alphanumeric, underscore, or dollar sign: we have a word boundary
    return not cp.isAlnum() && cp != '_' && cp != '$';
  } catch (const exception&) {
    // Could not read code point: we have a word boundary
    return true;
  }
}

/**
 * Returns whether the source is at a word boundary. The source is not advanced.
 *
 * An ASCII character decides this by itself, so peeking suffices. Before any other character, the whole code
 * point must be read and gone back over.
 *
 * @throw InvalidState if the next character is not ASCII and @p in cannot go back
 */
bool
isEndOfWord(nio::Source& in) {
  const auto c = in.peek();
  if (not c) {
    // End of input: we have a word boundary
    return true;
  }
  if (static_cast<u8>(*c) < 0x80) {
    return not isAsciiWordChar(*c);
  }

  const auto pos = in.tell();
  const auto result = readValidCodePoint(in);
  if (result) {
    rewindTo(in, pos); // Go back over the code point
  }

  // Invalid UTF-8: we have a word boundary
  return not result || not isWordCodePoint(*result);
}

/**
 * Checks if the character at position @p pos in @p str is escaped, that is, if it is preceded by an odd
 * number of backslashes.
 */
bool
isEscaped(string_view str, u64 pos) {
  u64 count = 0;
  while (count < pos && str[pos - count - 1] == '\\') {
    ++count;
  }
  return count % 2 != 0;
}

/// Returns whether @p cp can be part of a word: alphanumeric, underscore, or dollar sign.
bool
isWordCodePoint(unicode::CodePoint cp) {
  return cp.isAlnum() || cp == '_' || cp == '$';
}

/**
 * Reads a code point like #rocket::nio::readCodePoint, but returns an empty optional for invalid UTF-8,
 * which only an I/O error still throws for. The source is not advanced in that case.
 */
optional<unicode::CodePoint>
readValidCodePoint(nio::Source& in) {
  try {
    return nio::readCodePoint(in);
  } catch (const InputFailure&) {
    if (in.bad()) {
      throw; // I/O error
    }
    return {}; // Invalid UTF-8
  }
}

/**
 * Seeks @p in to the absolute position @p pos plus @p offset, where @p pos was obtained from `in.tell()`.
 *
 * @throw InvalidState if @p pos is empty, i.e., @p in cannot report its position and therefore cannot go back
 */
void
rewindTo(nio::Source& in, optional<u64> pos, u64 offset) {
  if (not pos) {
    throw InvalidState("Cannot go back in a source that cannot report its position");
  }
  in.seek(static_cast<i64>(*pos + offset), nio::SeekMode::beg);
}

/**
 * Returns whether @p s starts with @p prefix, optionally ignoring case.
 */
bool
startsWith(string_view s, string_view prefix, bool ignoreCase) {
  if (not ignoreCase) {
    return s.starts_with(prefix);
  }
  if (s.size() < prefix.size()) {
    return false;
  }
  return ranges::equal(s.substr(0, prefix.size()), prefix, [](char lhs, char rhs) {
    return tolower(static_cast<u8>(lhs)) == tolower(static_cast<u8>(rhs));
  });
}

} // namespace

namespace rocket::nio {

// Functions ------------------------------------------------------------------------------------------------

void
expectChar(nio::Source& in, char c) {
  if (not readChar(in, c)) {
    throw InputFailure(in.tell(), fmt::format("Expected {:?}", c));
  }
}

void
expectColon(nio::Source& in) {
  expectChar(in, ':');
}

void
expectComma(nio::Source& in) {
  expectChar(in, ',');
}

bool
readChar(nio::Source& in, char c) {
  if (in.peek() != c) {
    return false;
  }
  char current; // NOLINT
  return in.read(current) == 1;
}

optional<unicode::Character<char>>
readCharacter(nio::Source& in) {
  const auto start = in.tell();

  // The first code point decides about the end of the input, invalid UTF-8, and I/O errors
  const auto first = readCodePoint(in);
  if (not first) {
    return nullopt;
  }

  // Add code points while they continue the character
  string ret(static_cast<string>(*first));
  while (true) {
    const auto c = in.peek();
    if (not c) {
      if (in.bad()) {
        throw InputFailure(start, IO_ERROR);
      }
      break; // End of input
    }

    // An ASCII character is a complete code point, so peeking suffices
    if (static_cast<u8>(*c) < 0x80) {
      if (not continuesCharacter(ret, string_view(&*c, 1))) {
        break;
      }
      char consumed; // NOLINT
      in.read(consumed);
      ret.push_back(consumed);
      continue;
    }

    // Any other character must be read as a whole code point. If it does not continue the character, the
    // source must go back over it
    const auto pos = in.tell();
    optional<unicode::CodePoint> next;
    try {
      next = readCodePoint(in);
    } catch (const InputFailure&) {
      if (in.bad()) {
        throw InputFailure(start, IO_ERROR);
      }
      break; // Invalid UTF-8 ends the character. `readCodePoint` has not advanced the source
    }
    if (not next) {
      break;
    }
    const auto nextStr = static_cast<string>(*next);
    if (not continuesCharacter(ret, nextStr)) {
      rewindTo(in, pos);
      break;
    }
    ret.append(nextStr);
  }
  return unicode::Character<char>(std::move(ret));
}

optional<string_view>
readChoice( // NOLINT(*-complexity)
  nio::Source& in,
  const set<string_view>& values,
  bool ignoreCase,
  bool endOfWord) {
  // A byte is only consumed if it continues at least one candidate, so the source usually stops right after
  // the match, and a mismatch on the first byte consumes nothing. The source must go back on any other
  // failure, if a longer candidate failed after the match, or if checking the word boundary needs a
  // non-ASCII code point

  vector<string_view> candidates(values.begin(), values.end());
  for (const auto candidate : candidates) {
    ROCKET_CHECK(values, not candidate.empty(), "May not contain empty elements");
  }

  const auto matchesChar = [ignoreCase](char lhs, char rhs) {
    if (ignoreCase) {
      return tolower(static_cast<u8>(lhs)) == tolower(static_cast<u8>(rhs));
    }
    return lhs == rhs;
  };

  const auto start = in.tell();
  string seen; // The bytes consumed
  optional<string_view> ret; // The longest candidate matched so far

  while (true) {
    const auto c = in.peek();
    if (not c) {
      break;
    }

    // Keep the candidates that continue with `c`. If there are none, `c` is not consumed
    const u64 index = seen.size();
    vector<string_view> next;
    for (const auto candidate : candidates) {
      if (candidate.size() > index && matchesChar(candidate[index], *c)) {
        next.push_back(candidate);
      }
    }
    if (next.empty()) {
      break;
    }

    char consumed; // NOLINT
    in.read(consumed);
    seen.push_back(consumed);
    candidates = std::move(next);
    for (const auto candidate : candidates) {
      if (candidate.size() == seen.size() && (not ret || ret->size() < seen.size())) {
        ret = candidate; // The first of equally long candidates wins
      }
    }
  }

  // On failure, go back to the start, unless nothing has been consumed
  const auto fail = [&]() -> optional<string_view> {
    if (not seen.empty()) {
      rewindTo(in, start);
    }
    return {};
  };

  if (not ret) {
    return fail();
  }

  if (ret->size() < seen.size()) {
    // A longer candidate failed after the match. If an ASCII word character follows the match, there is no
    // word boundary
    if (endOfWord && isAsciiWordChar(seen[ret->size()])) {
      return fail();
    }
    rewindTo(in, start, ret->size());
  }

  if (endOfWord && not isEndOfWord(in)) {
    return fail();
  }
  return ret;
}

optional<unicode::CodePoint>
readCodePoint(nio::Source& in) {
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous

    const auto str = contiguous->str();
    if (str.empty()) {
      return {}; // `nextCodePoint` would throw for an empty string
    }

    try {
      u64 pos = 0;
      auto ret = unicode::nextCodePoint(str, pos);
      in.seek(static_cast<i64>(pos), SeekMode::cur);
      return ret;
    } catch (const exception&) {
      throw InputFailure(contiguous->position(), INVALID_UTF8);
    }
  } else {
    // Noncontiguous

    const auto start = in.tell();

    // Check the first byte by peeking: at the end of the input, or if the byte is neither a single byte nor
    // a lead byte, nothing is consumed
    const auto first = in.peek();
    if (not first) {
      if (in.bad()) {
        throw InputFailure(start, IO_ERROR);
      }
      return {};
    }
    const auto len = unicode::utf8::lengthFromByte(*first, unicode::InvalidUnicodePolicy::Continue);
    if (len == NPOS) {
      throw InputFailure(start, INVALID_UTF8);
    }

    string s(len, '\0');
    const u64 n = in.read(s);
    if (n != len && in.bad()) {
      throw InputFailure(start, IO_ERROR); // The position is unspecified
    }

    // Decode the code point. On failure, go back over the bytes read
    try {
      u64 pos = 0;
      if (n == len) {
        auto ret = unicode::nextCodePoint(s, pos);
        if (pos == s.size()) {
          return ret;
        }
      }
    } catch (const exception&) {
      // Fall through
    }
    rewindTo(in, start);
    throw InputFailure(start, INVALID_UTF8);
  }
}

optional<string>
readIdentifier(nio::Source& in) {
  const auto isStart = [](unicode::CodePoint cp) { return cp.isIdStart() || cp.eq('_') || cp.eq('$'); };
  const auto isContinue = [](unicode::CodePoint cp) { return cp.isIdContinue() || cp.eq('$'); };

  // Check the first code point. An ASCII character decides by itself, so peeking suffices. Any other
  // character must be read as a whole code point, and gone back over on failure
  const auto firstByte = in.peek();
  if (not firstByte || (static_cast<u8>(*firstByte) < 0x80 && not isStart(static_cast<char32>(*firstByte)))) {
    return {};
  }
  const auto start = in.tell();
  const auto first = readValidCodePoint(in);
  if (not first || not isStart(*first)) {
    rewindTo(in, start);
    return {};
  }

  string ret { static_cast<string>(*first) };
  while (true) {
    const auto c = in.peek();
    if (not c) {
      break;
    }

    // An ASCII character decides by itself, so peeking suffices
    if (static_cast<u8>(*c) < 0x80) {
      if (not isContinue(static_cast<char32>(*c))) {
        break;
      }
      char consumed; // NOLINT
      in.read(consumed);
      ret.push_back(consumed);
      continue;
    }

    // Any other character must be read as a whole code point. If it does not continue the identifier, the
    // source must go back over it, because it is not part of the identifier
    const auto pos = in.tell();
    const auto next = readValidCodePoint(in);
    if (not next || not isContinue(*next)) {
      rewindTo(in, pos);
      break;
    }
    ret.append(static_cast<string>(*next));
  }
  return ret;
}

bool
readString(nio::Source& in, string_view str, bool ignoreCase, bool endOfWord) {
  ROCKET_CHECK(str, not str.empty(), "May not be empty");

  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous

    const auto remaining = contiguous->str();
    if (not startsWith(remaining, str, ignoreCase)) {
      return false;
    }
    if (endOfWord && not isEndOfWord(remaining, str.size())) {
      return false;
    }
    in.seek(static_cast<i64>(str.size()), nio::SeekMode::cur);
    return true;
  } else {
    // Noncontiguous

    const auto pos = in.tell();

    for (const char expected : str) {
      char c; // NOLINT
      if (in.read(c) != 1 ||
        (ignoreCase ? tolower(static_cast<u8>(c)) != tolower(static_cast<u8>(expected)) : c != expected)) {
        // EOF or mismatch: rewind
        rewindTo(in, pos);
        return false;
      }
    }
    if (endOfWord && not isEndOfWord(in)) {
      // No word boundary: rewind
      rewindTo(in, pos);
      return false;
    }
    return true;
  }
}

std::chrono::nanoseconds
readSubseconds(nio::Source& in) {
  using namespace std::chrono;

  // Read subseconds as nanoseconds string

  string digits;
  if (readChar(in, '.')) {
    digits = readWhilePredicate(in, [](char c) { return isdigit(static_cast<u8>(c)); });
    if (digits.empty()) {
      throw InputFailure(in.tell(), "Expected subseconds");
    }
  }
  while (digits.size() < 9) {
    digits.push_back('0');
  }
  digits = digits.substr(0, 9);

  // Convert string to nanoseconds

  nanoseconds ret; // NOLINT
  {
    // When we used "{:i}" here, the scanning sometimes led to false values ...
    const auto result = scn::scan<nanoseconds::rep>(digits, "{}");
    ROCKET_ASSERT(result); // This *must* work
    ret = nanoseconds { result->value() };
  }
  return ret;
}

optional<string>
readUntilChar(nio::Source& in, char c) {
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous

    const auto remaining = contiguous->str();
    const u64 pos = remaining.find(c);
    if (pos == NPOS) {
      return {};
    }
    in.seek(static_cast<i64>(pos + 1), nio::SeekMode::cur);
    return string(remaining.substr(0, pos));
  } else {
    // Noncontiguous

    const auto pos = in.tell();

    string ret;

    while (true) {
      char current; // NOLINT
      if (in.read(current) != 1) {
        break;
      }
      if (current == c) {
        return ret;
      }
      ret.push_back(current);
    }

    rewindTo(in, pos);
    return {};
  }
}

optional<string>
readUntilUnescapedChar(nio::Source& in, char c) {
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous

    const auto remaining = contiguous->str();
    u64 pos = 0;

    while (true) {
      pos = remaining.find(c, pos);
      if (pos == NPOS) {
        return {};
      }
      if (not isEscaped(remaining, pos)) {
        in.seek(static_cast<i64>(pos + 1), nio::SeekMode::cur);
        return string(remaining.substr(0, pos));
      }
      ++pos;
    }
  } else {
    // Noncontiguous

    const auto pos = in.tell();

    string ret;

    while (true) {
      char current; // NOLINT
      if (in.read(current) != 1) {
        break;
      }
      if (current == c && not isEscaped(ret, ret.size())) {
        return ret;
      }
      ret.push_back(current);
    }

    rewindTo(in, pos);
    return {};
  }
}

string
readWhilePredicate(nio::Source& in, const function<bool(char)>& predicate) {
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous

    const auto remaining = contiguous->str();
    auto it = remaining.begin(), end = remaining.end();
    while (it != end && predicate(*it)) {
      ++it;
    }
    string ret(remaining.begin(), it);
    in.seek(static_cast<i64>(ret.size()), nio::SeekMode::cur);
    return ret;
  } else {
    // Noncontiguous

    string ret;
    while (true) {
      const auto c = in.peek();
      if (not c || not predicate(*c)) {
        break;
      }
      char consumed; // NOLINT
      in.read(consumed);
      ret.push_back(consumed);
    }
    return ret;
  }
}

void
skip(nio::Source& in, bool cComments, bool shellComments) { // NOLINT(*-complexity)
  constexpr string_view WS = "\t\n\v\f\r ";

  while (true) {
    const auto pos = in.tell();

    // Read first char
    char first; // NOLINT
    if (in.read(first) != 1) {
      break;
    }

    // If C comments and '/', read second char
    optional<char> second;
    if (cComments && first == '/') {
      char c; // NOLINT
      if (in.read(c) == 1) {
        second = c;
      }
    }

    // Skip whitespace characters
    if (WS.contains(first)) {
      continue;
    }

    // Skip shell-style `#` comments until EOL
    if (shellComments && first == '#') {
      if (not skipUntilString(in, "\n")) {
        break;
      }
      continue;
    }

    // Skip C-style `//` comments until EOL
    if (cComments && first == '/' && second == '/') {
      if (not skipUntilString(in, "\n")) {
        break;
      }
      continue;
    }

    // Skip C-style `/*` comments until `*/`
    if (cComments && first == '/' && second == '*') {
      if (not skipUntilString(in, "*/")) {
        rewindTo(in, pos);
        throw InputFailure(pos, "Unterminated C-style comment");
      }
      continue;
    }

    // Otherwise, stop skipping
    rewindTo(in, pos);
    break;
  }
}

bool
skipUntilString(nio::Source& in, string_view str) {
  ROCKET_CHECK(str, not str.empty(), "May not be empty");

  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous

    const auto pos = contiguous->str().find(str);
    if (pos == NPOS) {
      in.seek(0, nio::SeekMode::end);
      return false;
    }
    in.seek(static_cast<i64>(pos + str.size()), nio::SeekMode::cur);
    return true;
  } else {
    // Noncontiguous

    // Compare the last `str.size()` characters. A partial match must not simply be discarded on a mismatch,
    // because its tail may be the start of the actual match, as with "*/" in "**/" or "aab" in "aaab"

    string window;

    while (true) {
      char c; // NOLINT
      if (in.read(c) != 1) {
        break;
      }

      window.push_back(c);
      if (window.size() > str.size()) {
        window.erase(window.begin());
      }
      if (window == str) {
        return true;
      }
    }

    return false;
  }
}

} // namespace rocket::nio

// EOF
