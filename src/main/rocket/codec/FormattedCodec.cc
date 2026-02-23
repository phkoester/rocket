/*
 * FormattedCodec.cc
 */

#include "rocket/codec/FormattedCodec.h"

#include "rocket/unicode/unicode.h"

#include <boost/safe_numerics/safe_integer.hpp>

using namespace std;

using boost::safe_numerics::safe;

namespace rocket::codec::internal {

// Utilities for encoding -----------------------------------------------------------------------------------

void
beginContainer(nio::Sink& out, FormattedConsumerConfig& config, char c) {
  if (config.indent) {
    ++config.level;
  }
  out.write(c);
}

void
endContainer(nio::Sink& out, FormattedConsumerConfig& config, u64 size, char c) {
  if (config.indent) {
    --config.level;
    if (size > 0) {
      out.print("\n{: <{}}", "", 2 * config.level);
    }
  }
  out.write(c);
}

void
nextElem(nio::Sink& out, const FormattedConsumerConfig& config, u64 index) {
  if (index > 0) {
    if (config.indent) {
      out.write(',');
    } else {
      out.write(", ");
    }
  }
  if (config.indent) {
    out.print("\n{: <{}}", "", 2 * config.level);
  }
}

// Utilities for encoding -----------------------------------------------------------------------------------

void
expectColon(nio::StringSource& in) {
  if (not read(in, ':')) {
    throw InputFailure(in.tell(), "Missing colon");
  }
}

void
expectComma(nio::StringSource& in) {
  if (not read(in, ',')) {
    throw InputFailure(in.tell(), "Missing comma");
  }
}

u64
findUnescaped(std::string_view str, char c) {
  u64 pos = 0;
  while (true) {
    pos = str.find(c, pos);
    if (pos == NPOS) {
      return NPOS;
    }
    if (pos == 0 || str[pos - 1] != '\\') {
      return pos;
    }
    ++pos;
  }
}

bool
read(nio::StringSource& in, char c) {
  auto available = in.available();
  if (available.starts_with(c)) {
    in.seek(1, nio::SeekMode::cur);
    return true;
  }
  return false;
}

optional<string_view>
read(nio::StringSource& in, const std::set<std::string_view>& values, bool ignoreCase) {
  auto available = in.available();
  for (const auto& value : values) {
    if (not ignoreCase) {
      // Case-sensitive

      if (available.starts_with(value)) {
        in.seek(safe<i64>(value.size()), nio::SeekMode::cur);
        return value;
      }
    } else {
      // Case-insensitive

      string lhs(available.substr(0, value.size()));
      transform(lhs.begin(), lhs.end(), lhs.begin(), [](char c) { return tolower(c); });

      string rhs(value);
      transform(rhs.begin(), rhs.end(), rhs.begin(), [](char c) { return tolower(c); });

      if (lhs == rhs) {
        in.seek(safe<i64>(value.size()), nio::SeekMode::cur);
        return value;
      }
    }
  }
  return {};
}

void
skip(nio::StringSource& in, const FormattedProducerConfig& config) { // NOLINT(*-complexity)
  const auto available = in.available();
  u64 pos = 0; // Position relative to current position of the source
  while (pos < available.size()) {
    const char c = available[pos];

    // Skip all ASCII characters 0--32
    if (static_cast<u8>(c) <= 32) {
      ++pos;
      continue;
    }

    // Skip shell-style "#" comments until EOL
    if (config.shellComments && c == '#') {
      const auto lf = available.find('\n', pos + 1);
      if (lf == NPOS) {
        pos = available.size();
        break;
      }
      pos = lf + 1;
      continue;
    }

    const string_view twoChars = available.substr(pos, 2);

    // Skip C-style "//" comments until EOL
    if (config.cComments && twoChars == "//") {
      const auto lf = available.find('\n', pos + 2);
      if (lf == NPOS) {
        pos = available.size();
        break;
      }
      pos = lf + 1;
      continue;
    }

    // Skip C-style "/*" comments until "*/"
    if (config.cComments && twoChars == "/*") {
      const auto eoc = available.find("*/", pos + 2);
      if (eoc == NPOS) {
        throw InputFailure(in.tell() + pos, "Unterminated C-style comment");
      }
      pos = eoc + 2;
      continue;
    }

    // Skip whitespace code points
    u64 newPos = 0;
    const auto cp = unicode::nextCodePoint(available.substr(pos), newPos);
    if (cp.isWhitespace()) {
      pos += newPos;
      continue;
    }

    // Otherwise, stop skipping
    break;
  }
  // Advance the source
  in.seek(safe<i64>(pos), nio::SeekMode::cur);
}

} // namespace rocket::codec::internal

// EOF
