/*
 * FormattedCodec.cc
 */

#include "rocket/codec/FormattedCodec.h"

#include "rocket/unicode/unicode.h"

using namespace std;

namespace rocket::codec::internal {

// Utilities for encoding -----------------------------------------------------------------------------------

thread_local u64 level = 0;

void
beginContainer(nio::Sink& out, const FormattedConsumerConfig& config, char c) {
  if (config.indent) {
    ++level;
  }
  out.write(c);
}

void
endContainer(nio::Sink& out, const FormattedConsumerConfig& config, u64 size, char c) {
  if (config.indent) {
    --level;
    if (size > 0) {
      out.print("\n{: <{}}", "", 2 * level);
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
    out.print("\n{: <{}}", "", 2 * level);
  }
}

// Utilities for encoding -----------------------------------------------------------------------------------

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
    } else {
      ++pos;
    }
  }
}

bool
read(
  nio::StringSource& in,
  char c) {
  auto available = in.available();
  if (available.starts_with(c)) {
    in.seek(1, nio::SeekMode::cur);
    return true;
  }
  return false;
}

optional<string_view>
read(
  nio::StringSource& in,
  const std::set<std::string_view>& values,
  bool ignoreCase) {
  auto available = in.available();
  for (const auto& value : values) {
    if (not ignoreCase) {
      // Case-sensitive

      if (available.starts_with(value)) {
        in.seek(value.size(), nio::SeekMode::cur);
        return value;
      }
    } else {
      // Case-insensitive

      string lhs(available.substr(0, value.size()));
      transform(lhs.begin(), lhs.end(), lhs.begin(), [](char c) { return tolower(c); });

      string rhs(value);
      transform(rhs.begin(), rhs.end(), rhs.begin(), [](char c) { return tolower(c); });

      if (lhs == rhs) {
        in.seek(value.size(), nio::SeekMode::cur);
        return value;
      }
    }
  }
  return {};
}

void
skip(nio::StringSource& in, const FormattedProducerConfig& config) {
  auto available = in.available();
  u64 pos = 0; // Position relative to current position of the source
  while (pos < available.size()) {
    char c = available[pos];

    // Skip all ASCII characters 0--32
    if (static_cast<u8>(c) <= 32) {
      ++pos;
      continue;
    }

    // Skip shell-style "#" comments until EOL
    if (config.shellComments && c == '#') {
      auto lf = available.find('\n', pos + 1);
      if (lf == NPOS) {
        pos = available.size();
        break;
      }
      else {
        pos = lf + 1;
        continue;
      }
    }

    string_view twoChars = available.substr(pos, 2);

    // Skip C-style "//" comments until EOL
    if (config.cComments && twoChars == "//") {
      auto lf = available.find('\n', pos + 2);
      if (lf == NPOS) {
        pos = available.size();
        break;
      }
      else {
        pos = lf + 1;
        continue;
      }
    }

    // Skip C-style "/*" comments until "*/"
    if (config.cComments && twoChars == "/*") {
      auto eoc = available.find("*/", pos + 2);
      if (eoc == NPOS) {
        throw InputFailure(in.tell() + pos, "Unterminated C-style comment");
      }
      else {
        pos = eoc + 2;
        continue;
      }
    }

    // Skip whitespace code points
    u64 newPos = 0;
    auto cp = unicode::nextCodePoint(available.substr(pos), newPos);
    if (cp.isWhitespace()) {
      pos += newPos;
      continue;
    }

    // Otherwise, stop skipping
    break;
  }
  in.seek(pos, nio::SeekMode::cur);
}

} // namespace rocket::codec::internal

// EOF
