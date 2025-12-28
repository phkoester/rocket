/*
 * base.cc
 */

#include "base.h"

#include <iostream>

using namespace rocket;
using namespace std;

namespace {

// Local functions ------------------------------------------------------------------------------------------

/**
 * @return pointer to the end
 */
char*
uint128ToStringImpl(char* dest, uint128_t v) {
  if (v >= 10)
    dest = uint128ToStringImpl(dest, v / 10);
  *dest = static_cast<char>(v % 10 + '0');
  return ++dest;
}

char*
int128ToString(char* dest, int128_t v) {
  if (v < 0) {
    *dest = '-';
    *uint128ToStringImpl(dest + 1, static_cast<uint128_t>(-1 - v) + 1) = '\0';
  } else
    *uint128ToStringImpl(dest, static_cast<uint128_t>(v)) = '\0';

  return dest;
}

char*
uint128ToString(char* dest, uint128_t v) {
  *uint128ToStringImpl(dest, v) = '\0';
  return dest;
}

} // namespace

// `int128_t` -----------------------------------------------------------------------------------------------

istream&
operator>>(istream& lhs, int128_t& rhs) {
  // Read optional sign ('+' or '-')

  char c;
  lhs >> c;
  if (lhs.fail() || lhs.eof()) {
    return lhs;
  }
  int128_t sgn = 1;
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

  int128_t val = 0;
  int128_t factor = 1;

  for (auto it = buf.rbegin(); it != buf.rend(); ++it) {
    int128_t v = *it - '0';
    auto old = val;
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
operator<<(ostream& lhs, int128_t rhs) {
  char buf[41];
  int128ToString(buf, rhs);
  return lhs << buf;
}

// `uint128_t` ----------------------------------------------------------------------------------------------

istream&
operator>>(istream& lhs, uint128_t& rhs) {
  // Read optional sign ('+' or '-')

  char c;
  lhs >> c;
  if (lhs.fail() || lhs.eof()) {
    return lhs;
  }
  if (c == '-') {
    // Negative number: use the `ìnt128_t``overload
    lhs.seekg(-1, ios::cur);
    return operator>>(lhs, reinterpret_cast<int128_t&>(rhs));
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

  uint128_t val = 0;
  uint128_t factor = 1;

  for (auto it = buf.rbegin(); it != buf.rend(); ++it) {
    uint128_t v = *it - '0';
    auto old = val;
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
operator<<(ostream& lhs, uint128_t rhs) {
  char buf[41];
  uint128ToString(buf, rhs);
  return lhs << buf;
}

// EOF
