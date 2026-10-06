/*
 * io.cc
 */

#include "rocket/io/io.h"

#include "rocket/assert.h"
#include "rocket/io/io-log.h"

#include <array>
#include <iostream>
#include <ranges>

using namespace rocket;
using namespace std;

namespace {

// Local functions ------------------------------------------------------------------------------------------

/**
 * @return pointer to the end
 */
char*
u128ToStringImpl(char* dest, u128 val) { // NOLINT(*-recursion)
  if (val >= 10) {
    dest = u128ToStringImpl(dest, val / 10); // Recursive call
  }
  *dest = static_cast<char>(val % 10 + '0');
  return ++dest;
}

char*
i128ToString(char* dest, i128 val) {
  if (val < 0) {
    *dest = '-';
    *u128ToStringImpl(dest + 1, static_cast<u128>(-1 - val) + 1) = '\0';
  } else {
    *u128ToStringImpl(dest, static_cast<u128>(val)) = '\0';
  }
  return dest;
}

char*
u128ToString(char* dest, u128 val) {
  *u128ToStringImpl(dest, val) = '\0';
  return dest;
}

} // namespace

namespace rocket::io {

// Functions ------------------------------------------------------------------------------------------------

FILE*
open(const std::filesystem::path& path, std::string_view modes) {
#ifdef ROCKET_OS_WINDOWS
  FILE* file = nullptr;
  fopen_s(&file, path.string().c_str(), string(modes).c_str());
  IO_LOG("path=" << path << ", fopen_s=" << result << ", file=" << file_ << ", ferror=" << (file_ ? ferror(file_) : -1));
#else
  FILE* file = fopen(path.string().c_str(), string(modes).c_str());
  IO_LOG("path=" << path << ", fopen=" << file_ << ", ferror=" << (file_ ? ferror(file_) : -1));
#endif
  return file;
}

std::ios::pos_type
tellg(std::istream& is) noexcept {
  const auto state = is.rdstate();

  // Clear all bits
  is.clear();
  // This is expected to never throw, otherwise this implementation is flawed
  auto ret = is.tellg();
  ROCKET_ASSERT(ret >= 0);

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
      ROCKET_TERMINATE("`is.clear()` failed");
    }
  }

  return ret;
}

} // namespace rocket::io

// Support for 128-bit data types ---------------------------------------------------------------------------

namespace {

/**
 * Skips leading whitespace and peeks at the next character.
 *
 * Only `peek()` and `get()` are used for reading, never `seekg()` or `unget()`, so that this also works
 * with non-seekable stream buffers (e.g. the one used by `scn::basic_istream_scanner`).
 *
 * @return the next character, or `char_traits<char>::eof()` if there is none
 */
int
peekFirst(istream& is) {
  is >> ws;
  return is.peek();
}

/**
 * Reads decimal digits into @p buf until a non-digit or EOF is encountered. The non-digit is not consumed.
 */
void
readDigits(istream& is, string& buf) {
  while (true) {
    const auto c = is.peek();
    if (c == char_traits<char>::eof() || c < '0' || c > '9') { // NOLINT
      break;
    }
    buf.push_back(static_cast<char>(is.get()));
  }
}

} // namespace

istream&
operator>>(istream& lhs, i128& rhs) {
  // Read optional sign ('+' or '-')

  const int c = peekFirst(lhs);
  if (c == char_traits<char>::eof()) {
    return lhs;
  }
  i128 sgn = 1;
  if (c == '-') {
    sgn = -1;
  }
  if (c == '+' || c == '-') {
    lhs.get();
  }

  // Read digits

  string buf;
  readDigits(lhs, buf);

  // Got no digits, or too many?

  if (buf.empty() || buf.size() > 39) {
    lhs.setstate(ios::failbit);
    return lhs;
  }

  // Convert string to value, check for overflow

  i128 val = 0;
  i128 factor = 1;

  for (const char c : ranges::reverse_view(buf)) {
    const i128 v = c - '0';
    const auto old = val;
    if (sgn == -1) {
      val -= v * factor;
      if (val > old) {
        // Negative overflow
        lhs.setstate(ios::failbit);
        return lhs;
      }
    } else {
      val += v * factor;
      if (val < old) {
        // Positive overflow
        lhs.setstate(ios::failbit);
        return lhs;
      }
    }
    factor *= 10;
  }

  // Done

  rhs = val;
  return lhs;
}

ostream&
operator<<(ostream& lhs, i128 rhs) {
  array<char, 41> buf; // NOLINT(*-member-init)
  i128ToString(buf.data(), rhs);
  return lhs << buf.data();
}

// `u128` ----------------------------------------------------------------------------------------------------

istream&
operator>>(istream& lhs, u128& rhs) {
  // Read optional sign ('+' or '-')

  const int c = peekFirst(lhs);
  if (c == char_traits<char>::eof()) {
    return lhs;
  }
  if (c == '-') {
    // Negative number: use the `i128` overload, which sees the still unconsumed '-'
    return operator>>(lhs, reinterpret_cast<i128&>(rhs));
  }
  if (c == '+') {
    lhs.get();
  }

  // Read digits

  string buf;
  readDigits(lhs, buf);

  // Got no digits, or too many?

  if (buf.empty() || buf.size() > 39) {
    lhs.setstate(ios::failbit);
    return lhs;
  }

  // Convert string to value, check for overflow

  u128 val = 0;
  u128 factor = 1;

  for (const char c : ranges::reverse_view(buf)) {
    const u128 v = c - '0';
    const auto old = val;
    val += v * factor;
    if (val < old) {
      // Overflow
      lhs.setstate(ios::failbit);
      return lhs;
    }
    factor *= 10;
  }

  // Done

  rhs = val;
  return lhs;
}

ostream&
operator<<(ostream& lhs, u128 rhs) {
  array<char, 41> buf; // NOLINT(*-member-init)
  u128ToString(buf.data(), rhs);
  return lhs << buf.data();
}

// EOF
