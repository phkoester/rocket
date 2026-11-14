/*
 * nio-utils.cc
 */

#include "rocket/InputFailure.h"
#include "rocket/assert.h"
#include "rocket/nio/nio-utils.h"
#include "rocket/unicode/unicode.h"

#include <exception>

using namespace rocket;
using namespace std;

// Local functions ------------------------------------------------------------------------------------------

namespace {

/**
 * Seeks @p in to the absolute position @p pos plus @p offset, where @p pos was obtained from `in.tell()`.
 *
 * @throw InvalidState if @p pos is empty, i.e., @p in cannot report its position and therefore cannot go back
 */
void
rewindTo(nio::Source& in, optional<u64> pos, u64 offset = 0) {
  if (not pos) {
    throw InvalidState("Cannot go back in a source that cannot report its position");
  }
  in.seek(static_cast<i64>(*pos + offset), nio::SeekMode::beg);
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

bool
isEndOfWord(nio::Source& in) {
  const auto pos = in.tell();
  const auto result = nio::readCodePoint(in);
  rewindTo(in, pos); // Rewind immediately in any case

  if (not result) {
    // Could not read code point: we have a word boundary
    return true;
  }

  const auto cp = result.value();
  // Next code point is not alphanumeric, underscore, or dollar sign: we have a word boundary
  return not cp.isAlnum() && cp != '_' && cp != '$';
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

/**
 * Reads the longest of the given candidates from a noncontiguous source, advances the source only on
 * success.
 */
optional<string_view>
readLongestChoice( // NOLINT(*-complexity)
  nio::Source& in,
  vector<string_view> candidates,
  bool ignoreCase,
  bool endOfWord) {
  const auto pos = in.tell();

  string seen;
  optional<string_view> ret; // The best candidate so far

  const auto matchesChar = [ignoreCase](char lhs, char rhs) {
    if (ignoreCase) {
      return tolower(static_cast<u8>(lhs)) == tolower(static_cast<u8>(rhs));
    }
    return lhs == rhs;
  };

  const auto matchesString = [ignoreCase](string_view lhs, string_view rhs) {
    if (ignoreCase) {
      return ranges::equal(
        lhs, rhs,
        [](char lhs, char rhs) { return tolower(static_cast<u8>(lhs)) == tolower(static_cast<u8>(rhs)); });
    }
    return lhs == rhs;
  };

  while (true) {
    if (candidates.empty()) {
      break;
    }

    char c; // NOLINT
    if (in.read(c) != 1) {
      break;
    }
    const u64 index = seen.size();
    seen.push_back(c);

    for (auto it = candidates.begin(); it != candidates.end(); /* Empty */) {
      const auto candidate = *it;
      ROCKET_CHECK(candidates, not candidate.empty(), "May not contain empty elements");
      if (candidate.size() <= index || not matchesChar(candidate[index], c)) {
        it = candidates.erase(it);
      } else {
        if (matchesString(seen, candidate) && (not ret || ret->size() < candidate.size())) {
          ret = candidate;
        }
        ++it;
      }
    }
  }

  if (ret) {
    // Seek position after `ret`. The loop above has usually read past it, so this must happen before
    // checking for word boundary
    rewindTo(in, pos, ret->size());
    if (endOfWord && not isEndOfWord(in)) {
      ret = {};
    }
  }
  if (not ret) {
    // No match found: rewind
    rewindTo(in, pos);
  }
  return ret;
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

optional<string_view>
readChoice(nio::Source& in, const set<string_view>& values, bool ignoreCase, bool endOfWord) {
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous

    const auto remaining = contiguous->str();
    optional<string_view> ret; // The longest matching value so far
    for (const auto& val : values) {
      ROCKET_CHECK(values, not val.empty(), "May not contain empty elements");
      if (ret && ret->size() >= val.size()) {
        continue; // Cannot beat the current best
      }
      if (startsWith(remaining, val, ignoreCase)) {
        ret = val;
      }
    }
    if (ret && endOfWord && not isEndOfWord(remaining, ret->size())) {
      ret = {};
    }
    if (ret) {
      in.seek(static_cast<i64>(ret->size()), nio::SeekMode::cur);
    }
    return ret;
  } else {
    // Noncontiguous

    return readLongestChoice(in, vector<string_view>(values.begin(), values.end()), ignoreCase, endOfWord);
  }
}

ReadCodePointResult
readCodePoint(nio::Source& in) {
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous

    const auto str = contiguous->str();
    if (str.empty()) {
      // `nextCodePoint` would throw for an empty string, which must not be mistaken for invalid UTF-8
      return unexpected(Eof);
    }

    try {
      u64 pos = 0;
      auto ret = unicode::nextCodePoint(str, pos);
      in.seek(static_cast<i64>(pos), SeekMode::cur);
      return ret;
    } catch (const exception&) {
      return unexpected(InvalidUtf8);
    }
  } else {
    // Noncontiguous

    if (in.bad()) {
      return unexpected(Bad);
    }

    // A source that cannot report its position, such as a pipe or `NullSource`, cannot go back either. It is
    // not bad, though, so it is read all the same
    const auto initialPos = in.tell();

    // Read first byte
    char c; // NOLINT
    if (in.read(c) != 1) {
      return unexpected(in.bad() ? Bad : Eof);
    }
    string s;
    s.push_back(c);

    // The source may only advance on success, so rewind the position on failure, if possible. Seeking a
    // source that cannot seek would not only fail, but might also discard buffered input
    const auto failInvalidUtf8 = [&] {
      if (initialPos) {
        in.seek(static_cast<i64>(*initialPos), SeekMode::beg);
      }
      return unexpected(InvalidUtf8);
    };

    // Read subsequent bytes
    const auto len = unicode::utf8::lengthFromByte(c, unicode::InvalidUnicodePolicy::Continue);
    if (len == NPOS) {
      return failInvalidUtf8(); // Neither a single byte nor a lead byte
    }
    for (u64 i = 0; i < len - 1; ++i) {
      if (in.read(c) != 1) {
        if (in.bad()) {
          return unexpected(Bad);
        }
        return failInvalidUtf8(); // Truncated sequence: there was input, so this isn’t a plain EOF
      }
      s.push_back(c);
    }

    // Decode the code point
    try {
      u64 pos = 0;
      auto ret = unicode::nextCodePoint(s, pos);
      if (pos != s.size()) {
        return failInvalidUtf8();
      }
      return ret;
    } catch (const exception&) {
      return failInvalidUtf8();
    }
  }
}

optional<string>
readIdentifier(nio::Source& in) {
  const auto pos = in.tell();
  const auto first = readCodePoint(in);
  if (not first) {
    rewindTo(in, pos);
    return {};
  }

  const unicode::CodePoint cp = *first;
  if (not cp.isIdStart() && not cp.eq('_') && not cp.eq('$')) {
    rewindTo(in, pos);
    return {};
  }

  string ret { static_cast<string>(cp) };
  while (true) {
    const auto pos = in.tell();
    const auto next = readCodePoint(in);
    if (not next) {
      rewindTo(in, pos);
      break;
    }
    const unicode::CodePoint cont = *next;
    if (not cont.isIdContinue() && not cont.eq('$')) {
      rewindTo(in, pos);
      break;
    }
    ret.append(static_cast<string>(cont));
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
  // nio::out.println("DIGITS: {}", digits);

  // Convert string to nanoseconds

  nanoseconds ret; // NOLINT
  {
    // When we used "{:i}" here, the scanning sometimes led to false values ...
    const auto result = scn::scan<nanoseconds::rep>(digits, "{}");
    ROCKET_ASSERT(result); // This *must* work
    ret = nanoseconds { result->value() };
  }
  // nio::out.println("SUBSECONDS: {}", ret.count());
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
