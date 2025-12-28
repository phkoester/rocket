/*
 * base.cc
 */

#include "base.h"

#include "io.h"
#include "strings.h"

using namespace rocket;
using namespace std;

namespace {

// Local constants ------------------------------------------------------------------------------------------

constexpr string_view INT128_MIN = "-170141183460469231731687303715884105728";
constexpr string_view INT128_MAX = "170141183460469231731687303715884105727";

constexpr string_view UINT128_MAX = "340282366920938463463374607431768211455";

const set<char> DIGITS { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
const set<char> PLUS { '+' };
const set<char> PLUS_MINUS { '+', '-' };

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
  try {
    // Read optional `+` or `-`
    auto c = io::getOptionalChar(lhs, PLUS_MINUS);
    int sgn = c && *c == '-' ? -1 : 1;
    cout << "=== INT AFTER sgn, tell=" << io::tellg(lhs) << endl; // XXX

    // Read digits, remove leading zeroes
    string input = io::getWhile(lhs, DIGITS, 1);
    auto digits = strings::removeLeading<char>(input, "0");
    cout << "=== INT AFTER digits, tell=" << io::tellg(lhs) << endl; // XXX

    // Check limits
    if (digits.size() > INT128_MAX.size()) {
      lhs.setstate(ios::failbit);
      return lhs;
    }
    size_t count = INT128_MAX.size() - digits.size();
    string s = string(count, '0') + string(digits);
    if (sgn == -1) {
      // Check min limit
      if (s > INT128_MIN.substr(1)) { // We don't need the `-` here
        lhs.setstate(ios::failbit);
        return lhs;
      }
    } else {
      // Check max limit
      if (s > INT128_MAX) {
        lhs.setstate(ios::failbit);
        return lhs;
      }
    }

    // Apply digits
    int128_t value = 0;
    int128_t f = sgn;
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
      uint128_t v = *it - '0';
      value += f * v;
      f *= 10;
    }

    // Done
    rhs = value;
    return lhs;
  } catch (const exception& ex) {
    cout << "=== INT EX: " << ex.what() << endl; // XXX
    return lhs;
  }
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
  try {
    // Read optional `+`
    io::getOptionalChar(lhs, PLUS);

    // Read digits, remove leading zeroes
    string input = io::getWhile(lhs, DIGITS, 1);
    auto digits = strings::removeLeading<char>(input, "0");

    // Check max limit
    if (digits.size() > UINT128_MAX.size()) {
      lhs.setstate(ios::failbit);
      return lhs;
    }
    size_t count = UINT128_MAX.size() - digits.size();
    string s = string(count, '0') + string(digits);
    if (s > UINT128_MAX) {
      lhs.setstate(ios::failbit);
      return lhs;
    }

    // Apply digits
    uint128_t value = 0;
    uint128_t f = 1;
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
      uint128_t v = *it - '0';
      value += f * v;
      f *= 10;
    }

    // Done
    rhs = value;
    return lhs;
  } catch (const exception& ex) {
    cout << "=== UINT EX: " << ex.what() << endl; // XXX
    lhs.setstate(ios::failbit);
    return lhs;
  }
}

ostream&
operator<<(ostream& lhs, uint128_t rhs) {
  char buf[41];
  uint128ToString(buf, rhs);
  return lhs << buf;
}

// EOF
