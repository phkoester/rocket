/*
 * io.cc
 */

#include "rocket/io/io.h"

#include "rocket/assert.h"
#include "rocket/io/io-log.h"

#include <array>
#include <iostream>
#include <optional>

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
open(const filesystem::path& path, string_view modes) {
#ifdef ROCKET_OS_WINDOWS
  FILE* file = nullptr;
  const auto result = fopen_s(&file, path.string().c_str(), string(modes).c_str());
  IO_LOG("path=" << path << ", fopen_s=" << result << ", file=" << file << ", ferror=" << (file ? ferror(file) : -1));
#else
  FILE* file = fopen(path.string().c_str(), string(modes).c_str()); // NOLINT
  IO_LOG("path=" << path << ", fopen=" << file << ", ferror=" << (file ? ferror(file) : -1));
#endif
  return file;
}

i32
seek(FILE* file, i64 offset, i32 origin) {
#ifdef ROCKET_OS_WINDOWS
  return _fseeki64(file, offset, origin);
#else
  return fseek(file, offset, origin);
#endif
}

u64
tell(FILE* file) {
#ifdef ROCKET_OS_WINDOWS
  auto result = _ftelli64(file); // `__int64`
#else
  auto result = ftell(file); // `long`
#endif
  IO_LOG("ftell=" << result << ", ferror=" << ferror(file));
  return static_cast<u64>(result);
}

u64
tellg(istream& is) noexcept {
  // Save the state
  const auto state = is.rdstate();

  // Clear all bits
  is.clear();

  // This is expected to never throw, otherwise this implementation is flawed
  decltype(is.tellg()) result;
  try {
    result = is.tellg();
  } catch (const exception& ex) {
    ROCKET_TERMINATE("`is.tellg()` failed: {}", ex.what());
  } catch (...) {
    ROCKET_TERMINATE("`is.tellg()` failed");
  }

  // Restore the state
  if ((is.exceptions() & state) == 0) {
    // Restore the state without exception
    is.clear(state);
  } else {
    // Restore the state with exception
    try {
      is.clear(state);
    } catch (const ios::failure&) {
      // Nothing to do, we want to catch this silently
    } catch (...) {
      ROCKET_TERMINATE("`is.clear()` failed");
    }
  }

  return static_cast<u64>(result);
}

} // namespace rocket::io

// Support for 128-bit data types ---------------------------------------------------------------------------

namespace {

/**
 * Converts decimal digits to an unsigned value.
 *
 * The check happens before each step, so no intermediate result can overflow. Checking afterward whether the
 * value got smaller does not work, because a value that wraps around can still end up larger than before.
 *
 * @param digits the decimal digits
 * @param limit the greatest value allowed
 * @return the value, or null if it exceeds @p limit
 */
optional<u128>
parseDigits(string_view digits, u128 limit) {
  u128 ret = 0;
  for (const char c : digits) {
    const u128 digit = c - '0';
    // This is `ret * 10 + digit > limit`, rearranged so that it cannot overflow itself
    if (ret > (limit - digit) / 10) {
      return nullopt;
    }
    ret = ret * 10 + digit;
  }
  return ret;
}

/**
 * Skips leading whitespace and peeks at the next character.
 *
 * Only `peek()` and `get()` are used for reading, never `seekg()` or `unget()`, so that this also works
 * with nonseekable stream buffers (e.g., the one used by `scn::basic_istream_scanner`).
 *
 * @return the next character, or `char_traits<char>::eof()` if there is none
 */
int
peekFirst(istream& is) {
  is >> ws;
  return is.peek();
}

/**
 * Reads decimal digits into @p buf until a nondigit or EOF is encountered. The nondigit is not consumed.
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
  const bool negative = c == '-';
  if (c == '+' || c == '-') {
    lhs.get();
  }

  // Read digits

  string buf;
  readDigits(lhs, buf);

  // Got no digits?

  if (buf.empty()) {
    lhs.setstate(ios::failbit);
    return lhs;
  }

  // Convert string to magnitude, check for overflow. The magnitude is unsigned, because the lowest value’s
  // magnitude is one greater than the highest value and doesn’t fit into an `i128`

  const auto max = static_cast<u128>(numeric_limits<i128>::max());
  const auto magnitude = parseDigits(buf, negative ? max + 1 : max);
  if (not magnitude) {
    lhs.setstate(ios::failbit);
    return lhs;
  }

  // Done. Negate while still unsigned: unsigned arithmetic wraps by definition, so the lowest value needs no
  // special handling

  rhs = static_cast<i128>(negative ? static_cast<u128>(0) - *magnitude : *magnitude);
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

  // Got no digits?

  if (buf.empty()) {
    lhs.setstate(ios::failbit);
    return lhs;
  }

  // Convert string to value, check for overflow

  const auto val = parseDigits(buf, numeric_limits<u128>::max());
  if (not val) {
    lhs.setstate(ios::failbit);
    return lhs;
  }

  // Done

  rhs = *val;
  return lhs;
}

ostream&
operator<<(ostream& lhs, u128 rhs) {
  array<char, 41> buf; // NOLINT(*-member-init)
  u128ToString(buf.data(), rhs);
  return lhs << buf.data();
}

// EOF
