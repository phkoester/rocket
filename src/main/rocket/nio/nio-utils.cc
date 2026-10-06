/*
 * nio-utils.cc
 */

#include "rocket/assert.h"
#include "rocket/InputFailure.h"
#include "rocket/nio/nio-utils.h"
#include "rocket/unicode/unicode.h"

#include <boost/safe_numerics/safe_integer.hpp>

using namespace rocket;
using namespace std;

using boost::safe_numerics::safe;

// Local functions ------------------------------------------------------------------------------------------

namespace {

/**
 * Reads the longest of the given candidates from a noncontiguous source, advances the source only on
 * success.
 */
optional<string_view>
readLongestChoice(nio::Source& in, vector<string_view> candidates, bool ignoreCase) { // NOLINT(*-complexity)
  const auto pos = in.tell();

  string seen;
  optional<string_view> ret; // The best candidate so far

  const auto matchesChar = [ignoreCase](char lhs, char rhs) {
    if (ignoreCase) {
      return tolower(lhs) == tolower(rhs);
    }
    return lhs == rhs;
  };

  const auto matchesString = [ignoreCase](string_view lhs, string_view rhs) {
    if (ignoreCase) {
      return ranges::equal(lhs, rhs, [](char lhs, char rhs) { return tolower(lhs) == tolower(rhs); });
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
    // Seek position after `ret`
    in.seek(safe<i64>(pos + ret->size()), nio::SeekMode::beg);
  } else {
    // No match found: rewind
    in.seek(safe<i64>(pos), nio::SeekMode::beg);
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
    return tolower(lhs) == tolower(rhs);
  });
}

} // namespace

namespace rocket::nio {

// Utilities for writing to a sink --------------------------------------------------------------------------

void
beginContainer(nio::Sink& out, bool indent, u64& level, char c) {
  if (indent) {
    ++level;
  }
  out.write(c);
}

void
endContainer(nio::Sink& out, bool indent, u64& level, u64 size, char c) {
  if (indent) {
    --level;
    if (size > 0) {
      out.print("\n{: <{}}", "", 2 * level);
    }
  }
  out.write(c);
}

void
nextElem(nio::Sink& out, bool indent, u64 level, u64 index) {
  if (index > 0) {
    if (indent) {
      out.write(',');
    } else {
      out.write(", ");
    }
  }
  if (indent) {
    out.print("\n{: <{}}", "", 2 * level);
  }
}

// Utilities for reading from a source ----------------------------------------------------------------------

void
expectChar(nio::Source& in, char c) {
  if (not readChar(in, c)) {
    throw InputFailure(in.tell(), fmt::format("Expected {}", c));
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
  char current; // NOLINT
  if (in.read(current) != 1) {
    return false;
  }
  if (current != c) {
    in.seek(-1, nio::SeekMode::cur);
    return false;
  }
  return true;
}

optional<string_view>
readChoice(nio::Source& in, const set<string_view>& values, bool ignoreCase) {
#ifndef ROCKET_NIO_NO_CONTIGUOUS_SOURCE
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous source

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
    if (ret) {
      in.seek(safe<i64>(ret->size()), nio::SeekMode::cur);
    }
    return ret;
  }
#endif

  // Noncontiguous source

  return readLongestChoice(in, vector<string_view>(values.begin(), values.end()), ignoreCase);
}

bool
readString(nio::Source& in, std::string_view str, bool ignoreCase) {
  ROCKET_CHECK(str, not str.empty(), "May not be empty");

#ifndef ROCKET_NIO_NO_CONTIGUOUS_SOURCE
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous source

    if (not startsWith(contiguous->str(), str, ignoreCase)) {
      return false;
    }
    in.seek(safe<i64>(str.size()), nio::SeekMode::cur);
    return true;
  }
#endif

  // Noncontiguous source

  const auto pos = in.tell();

  for (const char expected : str) {
    char c; // NOLINT
    if (in.read(c) != 1 || (ignoreCase ? tolower(c) != tolower(expected) : c != expected)) {
      // EOF or mismatch: rewind
      in.seek(safe<i64>(pos), nio::SeekMode::beg);
      return false;
    }
  }
  return true;
}

std::chrono::nanoseconds
readSubseconds(nio::Source& in) {
  using namespace std::chrono;

  // Read subseconds as nanoseconds string

  std::string digits;
  if (readChar(in, '.')) {
    digits = readWhilePredicate(in, [](char c) { return isdigit(c); });
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
    ROCKET_ASSERT(result);
    ret = nanoseconds { result->value() };
  }
  // nio::out.println("SUBSECONDS: {}", ret.count());
  return ret;
}

optional<string>
readUntilChar(nio::Source& in, char c) {
#ifndef ROCKET_NIO_NO_CONTIGUOUS_SOURCE
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous source

    const auto remaining = contiguous->str();
    const u64 pos = remaining.find(c);
    if (pos == NPOS) {
      return {};
    }
    in.seek(safe<i64>(pos + 1), nio::SeekMode::cur);
    return string(remaining.substr(0, pos));
  }
#endif

  // Noncontiguous source

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

  in.seek(safe<i64>(pos), nio::SeekMode::beg);
  return {};
}

string
readWhilePredicate(nio::Source& in, const std::function<bool(char)>& predicate) {
#ifndef ROCKET_NIO_NO_CONTIGUOUS_SOURCE
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous source

    const auto remaining = contiguous->str();
    auto it = remaining.begin(), end = remaining.end();
    while (it != end && predicate(*it)) {
      ++it;
    }
    string ret(remaining.begin(), it);
    in.seek(safe<i64>(ret.size()), nio::SeekMode::cur);
    return ret;
  }
#endif

  // Noncontiguous source
  string ret;
  while (true) {
    char c; // NOLINT
    if (in.read(c) != 1) {
      break;
    }
    if (not predicate(c)) {
      in.seek(-1, nio::SeekMode::cur);
      break;
    }
    ret.push_back(c);
  }
  return ret;
}

optional<string>
readUntilUnescapedChar(nio::Source& in, char c) {
#ifndef ROCKET_NIO_NO_CONTIGUOUS_SOURCE
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous source

    const auto remaining = contiguous->str();
    u64 pos = 0;

    while (true) {
      pos = remaining.find(c, pos);
      if (pos == NPOS) {
        return {};
      }
      if (pos == 0 || remaining[pos - 1] != '\\') {
        in.seek(safe<i64>(pos + 1), nio::SeekMode::cur);
        string ret(remaining.substr(0, pos));
        return ret;
      }
      ++pos;
    }

    ROCKET_TERMINATE_UNREACHABLE_CODE();
  }
#endif

  // Noncontiguous source

  const auto pos = in.tell();

  optional<char> previous;
  string ret;

  while (true) {
    char current; // NOLINT;
    if (in.read(current) != 1) {
      break;
    }
    if (current == c && (not previous || *previous != '\\')) {
      return ret;
    }
    ret.push_back(current);
    previous = current;
  }

  in.seek(safe<i64>(pos), nio::SeekMode::beg);
  return {};
}

void
skip(nio::Source& in, bool cComments, bool shellComments) { // NOLINT(*-complexity)
  while (true) {
    const auto pos = in.tell();

    // Read first code point
    auto first = in.readCodePoint();
    if (not first) {
      break;
    }

    // Read second code point
    ReadCodePointResult second;
    if (cComments && first == '/') {
      second = in.readCodePoint();
      if (not second) {
        break;
      }
      if (*second != '/' && *second != '*') {
        break;
      }
    }

    // Skip all ASCII characters 0--32
    if (*first <= 32) {
      continue;
    }

    // Skip whitespace code points
    if (first->isWhitespace()) {
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
        in.seek(safe<i64>(pos), nio::SeekMode::beg);
        throw InputFailure(pos, "Unterminated C-style comment");
      }
      continue;
    }

    // Otherwise, stop skipping
    in.seek(safe<i64>(pos), nio::SeekMode::beg);
    break;
  }
}

bool
skipUntilString(nio::Source& in, std::string_view str) {
  ROCKET_CHECK(str, not str.empty(), "May not be empty");

#ifndef ROCKET_NIO_NO_CONTIGUOUS_SOURCE
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous source

    const auto remaining = contiguous->str();
    const auto pos = remaining.find(str);
    if (pos == NPOS) {
      in.seek(0, nio::SeekMode::end);
      return false;
    }
    in.seek(safe<i64>(pos + str.size()), nio::SeekMode::cur);
    return true;
  }
#endif

  string seen;

  while (true) {
    char c; // NOLINT
    if (in.read(c) != 1) {
      break;
    }

    const u64 index = seen.size();
    if (c == str[index]) {
      seen.push_back(c);
      if (seen == str) {
        return true;
      }
    } else {
      seen.clear();
    }
  }

  return false;
}

} // namespace rocket::nio

// EOF
