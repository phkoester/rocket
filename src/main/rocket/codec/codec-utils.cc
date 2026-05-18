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
expectColon(nio::Source& in) {
  if (not read(in, ':')) {
    throw InputFailure(in.tell(), "Missing colon");
  }
}

void
expectComma(nio::Source& in) {
  if (not read(in, ',')) {
    throw InputFailure(in.tell(), "Missing comma");
  }
}

bool
read(nio::Source& in, char c) {
  char current;
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
read(nio::Source& in, const std::set<std::string_view>& values, bool ignoreCase) {
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
  auto candidates(values); // A local copy of the set

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

    char c;
    if (in.read(c) != 1) {
      break;
    }
    u64 index = seen.size();
    seen.push_back(c);

    for (auto it = candidates.begin(), end = candidates.end(); it != end; /* Empty */) {
      const auto candidate = *it;
      if (candidate.size() <= index || not matches(candidate[index], c)) {
        it = candidates.erase(it);
      } else {
        if (candidate.size() == seen.size()) {
          return candidate;
        }
        ++it;
      }
    }
  }

  in.seek(safe<i64>(pos), nio::SeekMode::beg);
  return {};
}

optional<string>
readUntil(nio::Source& in, char c) {
#ifndef ROCKET_NIO_NO_CONTIGUOUS_SOURCE
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous source

    const auto str = contiguous->str();
    u64 pos = str.find(c);
    if (pos == NPOS) {
      return {};
    }
    in.seek(safe<i64>(pos + 1), nio::SeekMode::cur);
    string ret(str.substr(0, pos));
    return ret;
  }
#endif

  // Noncontiguous source

  const auto pos = in.tell();

  string ret;

  while (true) {
    char current;
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

optional<string>
readUntilUnescaped(nio::Source& in, char c) {
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
  char current;
  string ret;

  while (true) {
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
      if (not skipUntil(in, "\n")) {
        break;
      }
      continue;
    }

    // Skip C-style "//" comments until EOL
    if (cComments && first == '/' && second == '/') {
      if (not skipUntil(in, "\n")) {
        break;
      }
      continue;
    }

    // Skip C-style "/*" comments until "*/"
    if (cComments && first == '/' && second == '*') {
      if (not skipUntil(in, "*/")) {
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
skipUntil(nio::Source& in, std::string_view s) {
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
    in.seek(pos + s.size(), nio::SeekMode::cur);
    return true;
  }
#endif

  string seen;

  while (true) {
    char c;
    if (in.read(c) != 1) {
      break;
    }

    u64 index = seen.size();
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
