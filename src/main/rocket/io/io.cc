/*
 * io.cc
 */

#include "rocket/io/io.h"

#include "rocket/assert.h"

#ifdef ROCKET_OS_WINDOWS
  #include "rocket/windows.h"
  #include <share.h> // `_SH_DENYNO`
#else
  #include <sys/stat.h>
  #include <sys/types.h> // `off_t`
#endif

#include <array>
#include <cerrno>
#include <iostream>
#include <optional>

using namespace rocket;
using namespace std;

namespace rocket::io {

// Functions working with `FILE` pointers -------------------------------------------------------------------

FILE*
open(const filesystem::path& path, string_view modes) {
#ifdef ROCKET_OS_WINDOWS
  // `_wfsopen` takes the path as a wide string, so paths with characters outside the ANSI code page work.
  // `_SH_DENYNO` allows other processes to read and write the file, as on POSIX systems. (`fopen_s` would
  // deny write access to others.) The modes are ASCII, so a plain widening suffices
  const wstring wmodes(modes.begin(), modes.end());
  return _wfsopen(path.c_str(), wmodes.c_str(), _SH_DENYNO);
#else
  return fopen(path.c_str(), string(modes).c_str()); // NOLINT
#endif
}

bool
seek(FILE* file, i64 offset, i32 origin) {
  ROCKET_ASSERT(file != nullptr);

  // On Windows and macOS, seeking a pipe or a terminal may report success
  if (not seekable(file)) {
    errno = ESPIPE;
    return false;
  }

#ifdef ROCKET_OS_WINDOWS
  const auto result = _fseeki64(file, offset, origin);
#else
  if constexpr (sizeof(off_t) < sizeof(i64)) {
    // 32-bit `off_t`: build with `_FILE_OFFSET_BITS=64` to support large files
    if (offset != static_cast<off_t>(offset)) {
      errno = EOVERFLOW;
      return false;
    }
  }
  const auto result = fseeko(file, static_cast<off_t>(offset), origin);
#endif
  return result == 0;
}

bool
seekable(FILE* file) {
  ROCKET_ASSERT(file != nullptr);

  const i32 fd = ROCKET_FILENO(file);
#ifdef ROCKET_OS_WINDOWS
  // `_fileno` returns -2 for a standard stream that isn’t attached to anything, e.g., in a GUI process
  if (fd < 0) {
    return false;
  }
  // `_get_osfhandle` returns -2 under the same conditions, which is also the pseudo-handle of the current
  // thread, so it must not be passed on
  const auto handle = reinterpret_cast<HANDLE>(_get_osfhandle(fd)); // NOLINT
  if (handle == INVALID_HANDLE_VALUE || handle == reinterpret_cast<HANDLE>(-2)) { // NOLINT
    return false;
  }
  const DWORD type = GetFileType(handle);
  return type == FILE_TYPE_DISK; // `FILE_TYPE_PIPE`: pipe or socket, `FILE_TYPE_CHAR`: console or `NUL`
#else
  if (fd < 0) {
    // No file descriptor, e.g., a stream made by `fmemopen` or `open_memstream`. Such streams are implemented
    // by the C library, which reports a position only if the stream is seekable
    return ftello(file) != -1;
  }
  struct stat st {};
  if (fstat(fd, &st) != 0) {
    return false;
  }
  return S_ISREG(st.st_mode) || S_ISBLK(st.st_mode);
#endif
}

u64
tell(FILE* file) {
  ROCKET_ASSERT(file != nullptr);

  // On Windows and macOS, a pipe or a terminal may report a meaningless position
  if (not seekable(file)) {
    return NPOS;
  }

#ifdef ROCKET_OS_WINDOWS
  const auto result = _ftelli64(file); // `__int64`
#else
  const auto result = ftello(file); // `off_t`
#endif
  return result < 0 ? NPOS : static_cast<u64>(result);
}

// Functions working with streams ---------------------------------------------------------------------------

bool
seekable(istream& is) noexcept {
  return tell(is) != NPOS;
}

bool
seekable(ostream& os) noexcept {
  // Ask the stream buffer directly, like `tell` for an input stream does
  auto* buf = os.rdbuf();
  if (buf == nullptr || buf->pubseekoff(0, ios::cur, ios::out) == ostream::pos_type(ostream::off_type(-1))) {
    return false;
  }

  // The stream buffers of the standard output streams are backed by `stdout` and `stderr`. On Windows and
  // macOS, they may report a meaningless position for a pipe or a terminal, so ask the `FILE` as well
  if (&os == &cout) {
    return seekable(stdout);
  }
  if (&os == &cerr || &os == &clog) {
    return seekable(stderr);
  }
  return true;
}

u64
tell(istream& is) noexcept {
  // Ask the stream buffer directly. This is what `tellg` does after constructing a sentry, but the sentry
  // fails if any state bit is set, and it flushes the tied stream
  auto* buf = is.rdbuf();
  if (buf == nullptr) {
    return NPOS;
  }
  const auto result = buf->pubseekoff(0, ios::cur, ios::in);
  if (result == istream::pos_type(istream::off_type(-1))) {
    return NPOS;
  }

  // The stream buffer of `cin` is backed by `stdin`. On Windows and macOS, it may report a meaningless
  // position for a pipe or a terminal, so ask `stdin` as well
  if (&is == &cin && not seekable(stdin)) {
    return NPOS;
  }

  return static_cast<u64>(streamoff(result));
}

} // namespace rocket::io

#ifndef ROCKET_HAS_INT128_OP_IO

// Support for 128-bit data types ---------------------------------------------------------------------------

namespace {

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
i32
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
    // No input: fail like the standard extractors (`ws` and `peek` only set `eofbit`)
    lhs.setstate(ios::failbit);
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

istream&
operator>>(istream& lhs, u128& rhs) {
  // Read optional sign ('+' or '-')

  const int c = peekFirst(lhs);
  if (c == char_traits<char>::eof()) {
    // No input: fail like the standard extractors (`ws` and `peek` only set `eofbit`)
    lhs.setstate(ios::failbit);
    return lhs;
  }
  if (c == '-') {
    // Negative number: use the `i128` overload, which sees the still unconsumed '-', and wrap the result
    // around. Reading into a temporary avoids aliasing a `u128` as an `i128`, which is undefined behavior
    // where the two are distinct class types
    i128 val {};
    operator>>(lhs, val);
    if (not lhs.fail()) {
      rhs = static_cast<u128>(val);
    }
    return lhs;
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

#endif

// EOF
