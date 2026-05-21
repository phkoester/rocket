/*
 * codec-utils.cc
 */

#include "rocket/assert.h"
#include "rocket/InputFailure.h"
#include "rocket/codec/codec-utils.h"
#include "rocket/unicode/unicode.h"

#include <boost/safe_numerics/safe_integer.hpp>

using namespace std;

using boost::safe_numerics::safe;

namespace rocket::codec {

// Utilities for encoding -----------------------------------------------------------------------------------

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

// Utilities for decoding -----------------------------------------------------------------------------------

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
readChoice(nio::Source& in, const vector<string_view>& values, bool ignoreCase) { // NOLINT(*-complexity)
#ifndef ROCKET_NIO_NO_CONTIGUOUS_SOURCE
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous source

    auto remaining = contiguous->str();
    for (const auto& value : values) {
      if (not ignoreCase) {
        // Case-sensitive

        if (remaining.starts_with(value)) {
          in.seek(safe<i64>(value.size()), nio::SeekMode::cur);
          return value;
        }
      } else {
        // Case-insensitive

        string lhs(remaining.substr(0, value.size()));
        ranges::transform(lhs, lhs.begin(), [](char c) { return tolower(c); });

        string rhs(value);
        ranges::transform(rhs, rhs.begin(), [](char c) { return tolower(c); });

        if (lhs == rhs) {
          in.seek(safe<i64>(value.size()), nio::SeekMode::cur);
          return value;
        }
      }
    }
    return {};
  }
#endif

  // Noncontiguous source

  const auto pos = in.tell();

  string seen;
  auto candidates(values); // A local copy of the vector
  optional<string_view> ret; // The best candidate so far

  const auto matches = [ignoreCase](char lhs, char rhs) {
    if (ignoreCase) {
      return tolower(lhs) == tolower(rhs);
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
      if (candidate.size() <= index || not matches(candidate[index], c)) {
        it = candidates.erase(it);
      } else {
        if (seen == candidate && (not ret || ret->size() < candidate.size())) {
          ret = candidate;
        }
        ++it;
      }
    }
  }

  if (ret) {
    // Seek position after #ret
    in.seek(safe<i64>(pos + ret->size()), nio::SeekMode::beg);
  } else {
    // No match found: rewind
    in.seek(safe<i64>(pos), nio::SeekMode::beg);
  }
  return ret;
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

    const auto str = contiguous->str();
    const u64 pos = str.find(c);
    if (pos == NPOS) {
      return {};
    }
    in.seek(safe<i64>(pos + 1), nio::SeekMode::cur);
    return string(str.substr(0, pos));
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
readWhilePredicate(nio::Source& in, std::function<bool(char)> predicate) {
#ifndef ROCKET_NIO_NO_CONTIGUOUS_SOURCE
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous source

    const auto str = contiguous->str();
    auto it = str.begin(), end = str.end();
    while (it != end && predicate(*it)) {
      ++it;
    }
    string ret(str.begin(), it);
    in.seek(ret.size(), nio::SeekMode::cur);
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

    const auto str = contiguous->str();
    u64 pos = 0;

    while (true) {
      pos = str.find(c, pos);
      if (pos == NPOS) {
        return {};
      }
      if (pos == 0 || str[pos - 1] != '\\') {
        in.seek(safe<i64>(pos + 1), nio::SeekMode::cur);
        string ret(str.substr(0, pos));
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
    optional<unicode::CodePoint> second;
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

    // Skip shell-style "#" comments until EOL
    if (shellComments && first == '#') {
      if (not skipUntilString(in, "\n")) {
        break;
      }
      continue;
    }

    // Skip C-style "//" comments until EOL
    if (cComments && first == '/' && second == '/') {
      if (not skipUntilString(in, "\n")) {
        break;
      }
      continue;
    }

    // Skip C-style "/*" comments until "*/"
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
skipUntilString(nio::Source& in, std::string_view s) {
  ROCKET_CHECK(s, not s.empty(), "May not be empty");

#ifndef ROCKET_NIO_NO_CONTIGUOUS_SOURCE
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous source

    const auto str = contiguous->str();
    const auto pos = str.find(s);
    if (pos == NPOS) {
      in.seek(0, nio::SeekMode::end);
      return false;
    }
    in.seek(safe<i64>(pos + s.size()), nio::SeekMode::cur);
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
    if (c == s[index]) {
      seen.push_back(c);
      if (seen == s) {
        return true;
      }
    } else {
      seen.clear();
    }
  }

  return false;
}

} // namespace rocket::codec

// EOF
