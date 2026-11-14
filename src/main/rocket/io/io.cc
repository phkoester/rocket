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
#include <concepts>
#include <iostream>
#include <limits>
#include <locale>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

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

// These follow the standard extractors and inserters for integers (`num_get` and `num_put`), as implemented
// by libstdc++, including the digit grouping of the stream's locale

namespace {

using traits = char_traits<char>;

/**
 * Returns the base selected by `basefield` in @p flags: 8 for `oct`, 16 for `hex`, 10 for `dec` or several
 * flags, and 0 for none, which means that input detects the base, like `%i`, and output is decimal.
 */
u32
baseFromFlags(ios::fmtflags flags) {
  const auto basefield = flags & ios::basefield;
  if (basefield == ios::oct) {
    return 8;
  }
  if (basefield == ios::hex) {
    return 16;
  }
  return basefield == ios::fmtflags() ? 0 : 10;
}

/// Returns the value of the digit @p c in @p base, or an empty optional if @p c is not such a digit.
optional<u32>
digitValue(traits::int_type c, u32 base) {
  u32 ret = 0;
  if (c >= '0' && c <= '9') {
    ret = static_cast<u32>(c - '0');
  } else if (c >= 'a' && c <= 'f') {
    ret = static_cast<u32>(c - 'a' + 10);
  } else if (c >= 'A' && c <= 'F') {
    ret = static_cast<u32>(c - 'A' + 10);
  } else {
    return {};
  }
  return ret < base ? optional(ret) : nullopt;
}

/**
 * Returns whether @p size, an element of `numpunct::grouping`, is the size of a digit group. A size that is
 * not positive, or `CHAR_MAX`, means that there are no more groups.
 */
bool
isGroupSize(char size) {
  return static_cast<signed char>(size) > 0 && size != numeric_limits<char>::max();
}

/// The numeric punctuation of a locale that applies to integers.
struct Punctuation {
  string grouping; ///< The sizes of the digit groups, from the right. The last one repeats.
  char thousandsSep; ///< The character between digit groups.
  char decimalPoint; ///< The decimal point, which ends an integer.
  bool useGrouping; ///< Whether digits are grouped at all.
};

/// Returns the numeric punctuation of @p loc.
Punctuation
punctuation(const locale& loc) {
  const auto& facet = use_facet<numpunct<char>>(loc);
  auto grouping = facet.grouping();
  // libstdc++ ends the grouping at a 0, as in C's `lconv::grouping`, where it means that the previous size
  // repeats. That is what the last size does anyway
  if (const auto pos = grouping.find('\0'); pos != string::npos) {
    grouping.resize(pos);
  }
  const bool useGrouping = not grouping.empty() && isGroupSize(grouping[0]);
  return { std::move(grouping), facet.thousands_sep(), facet.decimal_point(), useGrouping };
}

/**
 * Inserts @p sep between the digit groups of @p digits, as specified by @p grouping, which must not be empty.
 * This is libstdc++'s `__add_grouping`.
 */
string
addGrouping(string_view digits, const string& grouping, char sep) {
  vector<string_view> groups; // From the right
  u64 index = 0;
  while (isGroupSize(grouping[index]) && digits.size() > static_cast<u64>(grouping[index])) {
    const auto size = static_cast<u64>(grouping[index]);
    groups.push_back(digits.substr(digits.size() - size));
    digits.remove_suffix(size);
    if (index < grouping.size() - 1) {
      ++index; // Otherwise, the last size repeats
    }
  }

  string ret(digits);
  for (auto it = groups.rbegin(); it != groups.rend(); ++it) {
    ret.push_back(sep);
    ret.append(*it);
  }
  return ret;
}

/**
 * Checks if the digit groups found in the input match @p grouping. This is libstdc++'s `__verify_grouping`.
 *
 * All groups must match @p grouping exactly, starting at the rightmost group, except for the leftmost group,
 * which may be shorter.
 *
 * @param grouping the grouping of the locale, must not be empty
 * @param found the sizes of the groups found, from the left, at least two
 */
bool
verifyGrouping(const string& grouping, const string& found) {
  const u64 n = found.size() - 1;
  const u64 last = min<u64>(n, grouping.size() - 1);
  u64 i = n;
  bool ret = true;
  for (u64 j = 0; j < last && ret; --i, ++j) {
    ret = found[i] == grouping[j];
  }
  for (; i > 0 && ret; --i) {
    ret = found[i] == grouping[last];
  }
  if (isGroupSize(grouping[last])) {
    ret = ret && found[0] <= grouping[last];
  }
  return ret;
}

/// The result of #parseInteger.
struct ParsedInteger {
  u128 magnitude = 0; ///< The magnitude, valid if #number and not #overflow.
  bool negative = false; ///< Whether there was a minus sign.
  bool number = false; ///< Whether a number was found, i.e., digits, and no misplaced thousands separator.
  bool groupingMatches = true; ///< Whether the digit groups match the locale. If not, the number is still valid.
  bool overflow = false; ///< Whether the magnitude exceeds the limit.
  bool eof = false; ///< Whether the end of the input was reached.
};

/**
 * Parses an optionally signed integer from @p buf in the base selected by @p flags. This follows libstdc++'s
 * `num_get::_M_extract_int`.
 *
 * In base 16, an `0x` or `0X` prefix is skipped. If the base is detected, a leading `0x` or `0X` selects base
 * 16, a leading `0` base 8, anything else base 10. Digits are consumed even after an overflow, so that the
 * whole number is consumed.
 *
 * If the locale groups digits, thousands separators may separate the digits. A separator at the start or after
 * another one ends the input, and there is no number then. The decimal point always ends the input.
 *
 * The check for an overflow happens before each step, so no intermediate result can overflow. Checking
 * afterward whether the value got smaller does not work, because a value that wraps around can still end up
 * larger than before.
 *
 * @param buf the stream buffer to read from
 * @param flags the format flags of the stream
 * @param punct the numeric punctuation of the stream's locale
 * @param positiveLimit the greatest magnitude allowed without a minus sign
 * @param negativeLimit the greatest magnitude allowed with a minus sign
 */
ParsedInteger
parseInteger(
  streambuf& buf,
  ios::fmtflags flags,
  const Punctuation& punct,
  u128 positiveLimit,
  u128 negativeLimit) {
  ParsedInteger ret;
  auto c = buf.sgetc();
  const auto atEof = [&] { return traits::eq_int_type(c, traits::eof()); };
  const auto advance = [&] { c = buf.snextc(); };
  const auto isSep = [&] { return punct.useGrouping && c == traits::to_int_type(punct.thousandsSep); };
  const auto isDecimalPoint = [&] { return c == traits::to_int_type(punct.decimalPoint); };

  // Sign. A thousands separator or a decimal point is no sign, even if it looks like one

  if ((c == '+' || c == '-') && not isSep() && not isDecimalPoint()) {
    ret.negative = c == '-';
    advance();
  }

  // Leading zeros and base prefix

  const u32 basefield = baseFromFlags(flags);
  u32 base = basefield == 0 ? 10 : basefield;
  bool foundZero = false;
  i32 sepPos = 0; // The number of digits since the last thousands separator
  while (not atEof()) {
    if (isSep() || isDecimalPoint()) {
      break;
    }
    if (c == '0' && (not foundZero || base == 10)) {
      foundZero = true;
      ++sepPos;
      if (basefield == 0) {
        base = 8;
      }
      if (base == 8) {
        sepPos = 0;
      }
    } else if (foundZero && (c == 'x' || c == 'X')) {
      if (basefield == 0) {
        base = 16;
      }
      if (base != 16) {
        break;
      }
      foundZero = false; // Digits must follow
      sepPos = 0;
    } else {
      break;
    }
    advance();
    if (not atEof() && not foundZero) {
      break;
    }
  }

  // Digits and thousands separators

  const u128 base128 = base;
  const u128 limit = ret.negative ? negativeLimit : positiveLimit;
  string foundGrouping; // The sizes of the digit groups, from the left
  bool misplacedSep = false;
  while (not atEof()) {
    if (isSep()) {
      if (sepPos == 0) {
        misplacedSep = true; // At the start or after another separator. It is not consumed
        break;
      }
      foundGrouping.push_back(static_cast<char>(sepPos));
      sepPos = 0;
    } else if (isDecimalPoint()) {
      break;
    } else {
      const auto digit = digitValue(c, base);
      if (not digit) {
        break;
      }
      if (not ret.overflow) {
        const u128 digit128 = *digit;
        // This is `magnitude * base + digit > limit`, rearranged so that it cannot overflow itself
        if (ret.magnitude > (limit - digit128) / base128) {
          ret.overflow = true;
        } else {
          ret.magnitude = ret.magnitude * base128 + digit128;
        }
      }
      ++sepPos;
    }
    advance();
  }

  if (not foundGrouping.empty()) {
    foundGrouping.push_back(static_cast<char>(sepPos)); // The rightmost group
    ret.groupingMatches = verifyGrouping(punct.grouping, foundGrouping);
  }
  ret.number = not misplacedSep && (sepPos != 0 || foundZero || not foundGrouping.empty());
  ret.eof = atEof();
  return ret;
}

/**
 * Sets `badbit` on @p is after its stream buffer threw, and rethrows if `badbit` is in the exception mask, as
 * the standard extractors do. Must be called from a `catch` block.
 */
void
handleBufferException(ios& is) {
  try {
    is.setstate(ios::badbit);
  } catch (const ios::failure&) {
    // Rethrow the original exception below instead
  }
  if ((is.exceptions() & ios::badbit) != 0) {
    throw;
  }
}

/**
 * Extracts an integer of type @p T from @p is, like the standard extractors.
 *
 * If the sentry fails, e.g., at the end of the input, @p val is left unchanged. If there is no number, @p val
 * is set to 0. On an overflow, it is set to the greatest or the lowest value. Both set `failbit`. For an
 * unsigned type, a negative value wraps around, as with `strtoull`. If the digit groups do not match the
 * locale, @p val is set, but `failbit` is set as well.
 */
template<typename T>
istream&
extractInteger(istream& is, T& val) {
  constexpr bool SIGNED = same_as<T, i128>;

  const istream::sentry sentry(is); // Skips whitespace, unless `noskipws` is set
  if (not sentry) {
    return is;
  }

  ParsedInteger parsed;
  try {
    const auto max = static_cast<u128>(numeric_limits<T>::max());
    parsed = parseInteger(*is.rdbuf(), is.flags(), punctuation(is.getloc()), max, SIGNED ? max + 1 : max);
  } catch (...) {
    handleBufferException(is);
    return is;
  }

  ios::iostate state = parsed.groupingMatches ? ios::goodbit : ios::failbit;
  if (not parsed.number) {
    val = 0;
    state |= ios::failbit;
  } else if (parsed.overflow) {
    val = SIGNED && parsed.negative ? numeric_limits<T>::min() : numeric_limits<T>::max();
    state |= ios::failbit;
  } else {
    // Negate while still unsigned: unsigned arithmetic wraps by definition, so neither the lowest signed value
    // nor a negative unsigned value needs special handling
    val = static_cast<T>(parsed.negative ? static_cast<u128>(0) - parsed.magnitude : parsed.magnitude);
  }
  if (parsed.eof) {
    state |= ios::eofbit;
  }
  is.setstate(state); // May throw, depending on the exception mask
  return is;
}

/**
 * Inserts an integer of type @p T into @p os, like the standard inserters.
 *
 * This honors `basefield`, `showbase`, `showpos`, `uppercase`, `adjustfield`, the width, the fill character,
 * and the digit grouping of the stream's locale, which applies in any base. As with `printf`'s `%o` and `%x`,
 * a negative value is written as its two's complement in octal and hexadecimal. `showpos` only applies to a
 * signed value in decimal, `showbase` only to a nonzero value.
 */
template<typename T>
ostream&
insertInteger(ostream& os, T val) {
  constexpr bool SIGNED = same_as<T, i128>;

  const auto flags = os.flags();
  const u32 base = baseFromFlags(flags) == 0 ? 10 : baseFromFlags(flags);
  const bool upper = (flags & ios::uppercase) != 0;

  bool negative = false;
  if constexpr (SIGNED) {
    negative = base == 10 && val < 0;
  }
  // Negating while unsigned also works for the lowest value
  u128 magnitude = negative ? static_cast<u128>(0) - static_cast<u128>(val) : static_cast<u128>(val);

  // Sign or base prefix

  string_view prefix;
  if (base == 10) {
    if (negative) {
      prefix = "-";
    } else if (SIGNED && (flags & ios::showpos) != 0) {
      prefix = "+";
    }
  } else if ((flags & ios::showbase) != 0 && magnitude != 0) {
    prefix = base == 8 ? "0" : upper ? "0X" : "0x";
  }

  // Digits, from the end. The greatest `u128` has 43 octal digits. Then, digit grouping

  const string_view digitChars = upper ? "0123456789ABCDEF" : "0123456789abcdef";
  const u128 base128 = base;
  array<char, 43> buf; // NOLINT(*-member-init)
  auto* const end = buf.data() + buf.size();
  auto* begin = end;
  do {
    *--begin = digitChars[static_cast<u32>(magnitude % base128)];
    magnitude /= base128;
  } while (magnitude != 0);

  string digits(begin, end);
  if (const auto punct = punctuation(os.getloc()); punct.useGrouping) {
    digits = addGrouping(digits, punct.grouping, punct.thousandsSep);
  }

  // Padding. With `internal`, the fill characters go after a sign or after `0x`, but before the `0` of octal

  const u64 len = prefix.size() + digits.size();
  const u64 width = os.width() > 0 ? static_cast<u64>(os.width()) : 0;
  const string fill(width > len ? width - len : 0, os.fill());

  string out;
  out.reserve(len + fill.size());
  const auto adjust = flags & ios::adjustfield;
  if (adjust == ios::left) {
    out.append(prefix).append(digits).append(fill);
  } else if (adjust == ios::internal && prefix != "0") {
    out.append(prefix).append(fill).append(digits);
  } else {
    out.append(fill).append(prefix).append(digits);
  }

  // The width only applies to the next output, and is already part of `out`
  os.width(0);
  return os << out;
}

} // namespace

istream&
operator>>(istream& lhs, i128& rhs) {
  return extractInteger(lhs, rhs);
}

ostream&
operator<<(ostream& lhs, i128 rhs) {
  return insertInteger(lhs, rhs);
}

istream&
operator>>(istream& lhs, u128& rhs) {
  return extractInteger(lhs, rhs);
}

ostream&
operator<<(ostream& lhs, u128 rhs) {
  return insertInteger(lhs, rhs);
}

#endif

// EOF
