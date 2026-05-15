/*
 * rocket.cc
 */

#include "rocket.h"

#ifdef ROCKET_HAS_128

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

// #i128 ----------------------------------------------------------------------------------------------------

istream&
operator>>(istream& lhs, i128& rhs) {
  // Read optional sign ('+' or '-')

  char c = '\0';
  lhs >> c;
  if (lhs.fail() || lhs.eof()) {
    return lhs;
  }
  i128 sgn = 1;
  if (c == '-') {
    sgn = -1;
  }
  if (c != '+' && c != '-') {
    // Not a sign: go back, clear EOF
    lhs.seekg(-1, ios::cur);
  }

  // Read digits

  string buf;

  while (true) {
    // Read one digit

    lhs >> c;
    if (lhs.eof()) {
      // EOF: clear fail bit, exit loop
      lhs.clear(lhs.rdstate() & ~ios::failbit);
      break;
    }
    if (lhs.fail()) {
      return lhs;
    }
    if (c < '0' || c > '9') {
      // Not a digit: go back, clear EOF
      lhs.seekg(-1, ios::cur);
      break;
    }
    buf.push_back(c);
  }

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

// #u128 ----------------------------------------------------------------------------------------------------

istream&
operator>>(istream& lhs, u128& rhs) {
  // Read optional sign ('+' or '-')

  char c = '\0';
  lhs >> c;
  if (lhs.fail() || lhs.eof()) {
    return lhs;
  }
  if (c == '-') {
    // Negative number: use the `ì128` overload
    lhs.seekg(-1, ios::cur);
    return operator>>(lhs, reinterpret_cast<i128&>(rhs));
  }
  if (c != '+') {
    // Not a sign: go back, clear EOF
    lhs.seekg(-1, ios::cur);
  }

  // Read digits

  string buf;

  while (true) {
    // Read one digit

    lhs >> c;
    if (lhs.eof()) {
      // EOF: clear fail bit
      lhs.clear(lhs.rdstate() & ~ios::failbit);
      break;
    }
    if (lhs.fail()) {
      return lhs;
    }
    if (c < '0' || c > '9') {
      // Not a digit: go back, clear EOF
      lhs.seekg(-1, ios::cur);
      break;
    }
    buf.push_back(c);
  }

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

#endif // ROCKET_HAS_128

// EOF
