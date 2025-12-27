/*
 * io.cc
 */

#include "codec-std-decl.h"
#include "codec-std.h"

#include "io.h"

#include "assert.h"

#include <memory>

using namespace std;

namespace rocket::io {

// `Buffer` -------------------------------------------------------------------------------------------------

Buffer::Buffer(istream& is, size_t size) :
    is_(is),
    size_(size),
    pos_(tellg(is)) {
  ROCKET_CHECK(size, size >= MIN_BUFFER_SIZE);
  p_ = make_unique<byte[]>(size);
}

optional<byte>
Buffer::get() {
  // If available, obtain byte that was put back
  if (not intermediate_.empty()) {
    byte ret = intermediate_.back();
    intermediate_.pop_back();
    ++pos_;
    return ret;
  }

  // If needed, load the next chunk of data
  if (index_ == end_) {
    is_.read(reinterpret_cast<char*>(p_.get()), size_);
    index_ = 0;
    end_ = is_.gcount();
    if (end_ == 0)
      return nullopt;
  }
  // Return the next byte
  ++pos_;
  return p_.get()[index_++];
}

optional<vector<byte>>
Buffer::getCodePoint(unicode::CodePoint* cp) {
  size_t pos = pos_;
  auto got = get();
  if (not got)
    return nullopt;

  char c = static_cast<char>(*got);
  auto cpSize = unicode::utf8::codePointSize(c);
  if (cpSize == 0) {
    // XXX thowInputFailure?
    throw InputFailure(is_, pos, fmt::format("Invalid UTF-8 byte: {:#x}", *got)); // XXX x02?
  }

  vector<byte> ret;
  ret.reserve(cpSize);
  ret.push_back(*got);

  for (uint8_t i = 1; i < cpSize; ++i) {
    got = get();
    if (not got)
      throw InputFailure(is_, pos, "Incomplete UTF-8 byte sequence");
    if (not unicode::utf8::continuationByte(static_cast<char>(*got)))
      throw InputFailure(is_, pos, "Invalid UTF-8 byte sequence");
    ret.push_back(*got);
  }

  if (cp) {
    string_view s(reinterpret_cast<const char*>(ret.data()), ret.size());
    *cp = *unicode::CodePointIterator<char>(s);
  }

  return ret;
}

optional<vector<byte>>
Buffer::getGrapheme(unicode::Grapheme* gr) {
  // Read first code point
  unicode::CodePoint cp;
  auto bytes = getCodePoint(&cp);
  if (not bytes)
    return nullopt;

  // Update values
  u32string s { cp };
  vector<byte> ret;
  copy(bytes->begin(), bytes->end(), back_inserter(ret));

  // Read more code points

  while (true) {
    // Read next code point
    bytes = getCodePoint(&cp);

    // If EOF, finish
    if (not bytes) {
      if (gr)
        *gr = unicode::Grapheme(s);
      return ret;
    }

    // Update values I
    s.push_back(cp);

    // If grapheme boundary, put back and finish
    size_t pos = s.size() - 1;
    if (unicode::CodePointIterator<char32_t>(s, pos).graphemeBoundary()) {
      put(*bytes);
      if (gr)
        *gr = unicode::Grapheme(s.substr(0, pos));
      return ret;
    }

    // Update values II
    copy(bytes->begin(), bytes->end(), back_inserter(ret));
  }
}

void
Buffer::put(byte b) {
  intermediate_.push_back(b);
  ROCKET_EXPECT(pos_ > 0);
  --pos_;
}

void
Buffer::put(const vector<byte>& bytes) {
  for (auto it = bytes.rbegin(), end = bytes.rend(); it != end; ++it)
    put(*it);
}

// `Symbols` ------------------------------------------------------------------------------------------------

const set<char> Symbols::HexDigits {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  'a', 'b', 'c', 'd', 'e', 'f',
  'A', 'B', 'C', 'D', 'E', 'F'
};

// Functions ------------------------------------------------------------------------------------------------

void
check(std::istream& is) {
  if (is.eof()) {
    throw ParseFailure(is, tellg(is), "EOF");
  }
  if (is.fail()) {
    throw InputFailure(is);
  }
}

int
fd(const std::ios& ios)
{
  if (&ios == &std::cout) {
    return STDOUT_FILENO;
  } else if (&ios == &std::cerr) {
    return STDERR_FILENO;
  } else if (&ios == &std::cin) {
    return STDIN_FILENO;
  }
  return -1;
}

char
getChar(std::istream& is) {
  char c = '\0';
  is.read(&c, 1);
  return c;
}

char
getChar(std::istream& is, char v) {
  ROCKET_CHECK(v, isascii(v));

  size_t inputPos = tellg(is);

  char c = getChar(is);
  if (is.eof()) {
    throw ParseFailure(is, inputPos, fmt::format("Expected '{}' got EOF", v)); // XXX
  }
  check(is);
  if (c != v) {
    throw ParseFailure(is, inputPos, fmt::format("Expected '{}' got '{}'", v, c)); // XXX
  }
  return c;
}

char
getChar(std::istream& is, const std::set<char>& values) {
  size_t inputPos = tellg(is);

  char c = getChar(is);
  if (is.eof()) {
    throw ParseFailure(is, inputPos, fmt::format("Expected any of {} got EOF", values)); // XXX values
  }
  check(is);
  if (not values.contains(c)) {
    throw ParseFailure(is, inputPos, fmt::format("Expected any of {} got '{}'", values, c)); // XXX values
  }
  return c;
}

std::optional<char>
getOptionalChar(std::istream& is, char v) {
  ROCKET_CHECK(v, isascii(v));

  size_t inputPos = tellg(is);

  char c = getChar(is);
  if (is.eof()) {
    seekg(is, inputPos);
    return std::nullopt;
  }
  check(is);

  if (c != v) {
    seekg(is, inputPos);
    return std::nullopt;
  } else
    return c;
}

std::optional<char>
getOptionalChar(std::istream& is, const std::set<char>& values) {
  size_t inputPos = tellg(is);

  char c = getChar(is);
  if (is.eof()) {
    seekg(is, inputPos);
    return std::nullopt;
  }
  check(is);

  if (not values.contains(c)) {
    seekg(is, inputPos);
    return std::nullopt;
  } else
    return c;
}

std::string_view
getString(std::istream& is, std::string_view v) {
  ROCKET_CHECK(v, not v.empty());

  size_t inputPos = tellg(is);

  auto it = unicode::CodePointIterator<char>(v), end = unicode::CodePointIterator<char>(v, v.size());
  for (; it != end; ++it) {
    // Read one code point
    size_t pos = io::tellg(is);
    unicode::CodePoint cp;
    is >> cp;
    if (is.eof()) {
      throw ParseFailure(is, pos, { inputPos, pos },
          fmt::format("{} does not match {}, got EOF", v.substr(0, pos - inputPos), v)); // XXX
    }
    check(is);

    if (cp != *it) {
      throw ParseFailure(is, pos, { inputPos, tellg(is) },
          fmt::format("{} does not match {}", v.substr(0, pos - inputPos), v)); // XXX
    }
  }
  return v;
}

std::string
getString(std::istream& is, const std::set<std::string_view>& values) {
  std::string ret;

  auto localValues(values);

  size_t inputPos = tellg(is);
  std::string input; // Input so far

  while (true) {
    // Read one code point
    size_t pos = tellg(is);
    unicode::CodePoint cp;
    is >> cp;
    if (is.eof()) {
      if (not ret.empty()) {
        seekg(is, pos);
        return ret;
      } else {
        throw ParseFailure(is, pos, { inputPos, tellg(is) },
            fmt::format("{} does not match any of {}, got EOF", input, values)); // XXX
      }
    }
    check(is);

    // Advance by one code point
    input.append(static_cast<std::string>(cp));

    // Remove nonmatching values and fully matched value from set
    bool match = false;
    for (auto it = localValues.begin(), end = localValues.end(); it != end;) {
      auto value = *it;
      if (value.substr(0, input.size()) != input)
        it = localValues.erase(it);
      else if (value.size() == input.size()) {
        match = true;
        ret = input;
        it = localValues.erase(it);
      } else {
       match = true;
       ++it;
      }
    }

    // Finished?
    if (localValues.empty()) {
      if (not ret.empty()) {
        if (not match)
          seekg(is, pos);
        return ret;
      } else {
        throw ParseFailure(is, pos, { inputPos, tellg(is) },
            fmt::format("{} does not match any of {}, got EOF", input, values)); // XXX
      }
    }
  }
}

std::string
getUntil(std::istream& is, char delimiter, bool consumeDelimiter, size_t min) {
  ROCKET_CHECK(delimiter, isascii(delimiter));

  size_t inputPos = tellg(is);
  std::string input;

  while (true) {
    size_t pos = tellg(is);
    char c = getChar(is);
    if (is.eof()) {
      throw ParseFailure(is, pos, fmt::format("Seeking {}, got EOF", delimiter)); // XXX
    }
    check(is);

    if (c == delimiter) {
      if (not consumeDelimiter) {
        is.putback(c);
      }
      if (input.size() < min) {
        throw ParseFailure(is, pos, { inputPos, inputPos + min },
            fmt::format("Expected at least {} before {}, got {}", noun::character(min), delimiter, input.size())); // XXX
      }
      return input;
    }
    input.push_back(c);
  }
}

std::string
getUntil(
    std::istream& is,
    std::function<bool(char)> delimiter,
    std::string_view delimiterDescription,
    bool consumeDelimiter,
    size_t min) {
  size_t inputPos = tellg(is);
  std::string input;

  while (true) {
    size_t pos = tellg(is);
    char c = getChar(is);
    if (is.eof()) {
      throw ParseFailure(is, pos, fmt::format("Seeking {:?}, got EOF", delimiterDescription)); // XXX
    }
    check(is);

    if (delimiter(c)) {
      if (not consumeDelimiter)
        is.putback(c);
      if (input.size() < min) {
        throw ParseFailure(is, pos, { inputPos, inputPos + min },
            fmt::format("Expected at least {} before {}, got {}", noun::character(min), delimiterDescription, input.size())); // XXX
      }
      return input;
    }
    input.push_back(c);
  }
}

std::string
getWhile(std::istream& is, const std::set<char>& values, size_t min) {
  size_t inputPos = tellg(is);
  std::string input;

  while (true) {
    size_t pos = tellg(is);
    char c = getChar(is);
    if (is.eof()) {
      if (input.size() >= min) {
        seekg(is, pos);
        return input;
      } else {
        throw ParseFailure(is, pos, { inputPos, inputPos + min },
            fmt::format("Expected at least {} contained in {}, got {} and EOF", noun::character(min), values, input.size())); // XXX
      }
    }
    check(is);

    if (values.contains(c)) {
      input.push_back(c);
    } else {
      if (input.size() >= min) {
        seekg(is, pos);
        return input;
      } else {
        throw ParseFailure(is, pos, { inputPos, inputPos + min },
            fmt::format("Expected at least {} contained in {}, got {} and {}", noun::character(min), values, input.size(), c)); // XXX
      }
    }
  }
}

std::ispanstream
is(std::span<char> v, std::ios::openmode mode, std::ios::iostate exceptions) {
  auto ret = std::ispanstream(v, mode);
  ret.exceptions(exceptions);
  return ret;
}

std::istream&
seekg(std::istream& is, size_t position) {
  const auto state = is.rdstate();
  // Clear `eofbit` and `failbit`, leave `badbit` unchanged
  is.clear(state & ~(std::ios::eofbit | std::ios::failbit));
  typename std::istream::pos_type seekg = position;
  ROCKET_CHECK(position, seekg >= 0, "{}", message::overflow(Type::of(seekg)));
  // This might throw due to `badbit`, which is okay
  return is.seekg(seekg);
}

std::istream&
seekg(std::istream& is, size_t position, std::ios::seekdir dir) {
  const auto state = is.rdstate();
  // Clear `eofbit` and `failbit`, leave `badbit` unchanged
  is.clear(state & ~(std::ios::eofbit | std::ios::failbit));
  typename std::istream::pos_type seekg = position;
  ROCKET_CHECK(position, seekg >= 0, "{}", message::overflow(Type::of(seekg)));
  // This might throw due to `badbit`, which is okay
  return is.seekg(seekg, dir);
}

size_t
tellg(std::istream& is) noexcept {
  const auto state = is.rdstate();

  // Clear all bits
  is.clear();
  // This is expected to never throw, otherwise this implementation is flawed
  auto tellg = is.tellg();
  ROCKET_ASSERT(tellg >= 0);

  // Restore the state
  if ((is.exceptions() & state) == 0) {
    // Restore the state without exception
    is.clear(state);
  } else {
    // Restore the state with exception
    try {
      is.clear(state);
    } catch (const std::ios::failure&) {
      // Nothing to do, we want to catch this silently
    } catch (...) {
      ROCKET_PROCESS_ERROR("`is.clear()` failed");
    }
  }

  return tellg;
}

} // namespace rocket::io

// `std` ----------------------------------------------------------------------------------------------------

namespace std {

string&
operator<<(string& lhs, const istream& rhs) {
  ostringstream os;
  os << rhs.rdbuf();
  lhs.append(os.str());
  return lhs;
}

} // namespace std

// EOF
