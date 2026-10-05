/*
 * unicode.cc
 */

#include "unicode.h"

#include "rocket/assert.h"

#include <boost/safe_numerics/safe_integer.hpp>

#include <unicode/uchar.h>
#include <unicode/utf8.h>
#include <unicode/utypes.h>

#include <array>

using boost::safe_numerics::safe;

using namespace icu;
using namespace rocket;
using namespace rocket::unicode;
using namespace std;

namespace rocket::unicode {

// `CodePoint` ----------------------------------------------------------------------------------------------

CodePoint::operator string() const {
  array<char, 4> buf; // NOLINT
  i32 i = 0;
  UBool error = 0;
  U8_APPEND(buf.data(), i, 4, val_, error); // NOLINT
  ROCKET_DEBUG_ASSERT(error == 0, "`U8_APPEND` failed for code-point value 0x{:X}", static_cast<u32>(val_));
  return string(buf.data(), i); // NOLINT
}

bool
CodePoint::isPrint() const {
  return u_isprint(val_) != 0; // NOLINT
}

bool
CodePoint::isWhitespace() const {
  return u_isWhitespace(val_) != 0; // NOLINT
}

CodePoint
CodePoint::lower() const {
  return static_cast<char32>(u_tolower(val_)); // NOLINT
}

CodePoint
CodePoint::upper() const {
  return static_cast<char32>(u_toupper(val_)); // NOLINT
}

u8
CodePoint::width() const {
  if (not isPrint()) {
    return 0;
  }

  const auto generalCategory = u_getIntPropertyValue(val_, UCHAR_GENERAL_CATEGORY); // NOLINT
  switch (generalCategory) {
  case U_ENCLOSING_MARK:
  case U_NON_SPACING_MARK:
    return 0;
  }

  const auto eastAsianWidth = u_getIntPropertyValue(val_, UCHAR_EAST_ASIAN_WIDTH); // NOLINT
  switch (eastAsianWidth) {
  case U_EA_FULLWIDTH:
  case U_EA_WIDE:
    return 2;
  }

  if (u_hasBinaryProperty(val_, UCHAR_EMOJI_PRESENTATION)) { // NOLINT
    return 2;
  }

  return 1;
}

ostream&
operator<<(ostream& lhs, CodePoint rhs) {
  return lhs << fmt::format("{}", rhs);
}

// Functions ------------------------------------------------------------------------------------------------

u32string
convertUtf8To32(string_view str, InvalidUnicodePolicy policy) {
  u32string ret;
  ret.reserve(str.size());

  for (u64 pos = 0, size = str.size(); pos < size;) {
    UChar32 cp; // NOLINT
    i32 i = 0;
    U8_NEXT(&str[pos], i, size - pos, cp); // NOLINT
    ROCKET_DEBUG_ASSERT(i > 0, "`U8_NEXT` failed");

    if (cp < 0) {
      // Invalid or incomplete UTF-8 byte sequence
      if (policy == InvalidUnicodePolicy::Throw) {
        throw InputFailure(pos, "Invalid UTF-8 byte sequence");
      } else {
        pos += i;
        ret.push_back(U'�');
        continue;
      }
    }

    pos += i;
    ret.push_back(static_cast<char32>(cp));
  }

  return ret;
}

string
convertUtf32To8(u32string_view str, InvalidUnicodePolicy policy) {
  string ret;
  ret.reserve(str.size());

  array<char, 4> buf; // NOLINT

  for (u64 pos = 0, size = str.size(); pos < size; ++pos) {
    const auto c = str[pos];

    if (not CodePoint::valid(c)) {
      if (policy == InvalidUnicodePolicy::Throw) {
        throw InputFailure(pos, fmt::format("Invalid code-point value 0x{:X}", static_cast<u32>(c)));
      } else {
        ret.append("�");
        continue;
      }
    }

    i32 i = 0;
    UBool error = 0;
    U8_APPEND(buf.data(), i, 4, c, error); // NOLINT
    ROCKET_DEBUG_ASSERT(error == 0, "`U8_APPEND` failed for code-point value 0x{:X} at position {}", static_cast<u32>(c), pos);
    ret.append(buf.data(), i); // NOLINT
  }

  return ret;
}

// UTF-8 ....................................................................................................

namespace utf8 {

u64
lengthFromByte(char c, InvalidUnicodePolicy policy) {
  if (U8_IS_SINGLE(c)) {
    return 1;
  }
  if (U8_IS_LEAD(c)) {
    return U8_LENGTH_FROM_LEAD_BYTE(c);
  }
  if (policy == InvalidUnicodePolicy::Continue) {
    return NPOS;
  }
  throw InputFailure(0, fmt::format("{:0>#2x} is neither a single nor a UTF-8 lead byte", c));
}

CodePoint
nextCodePoint(string_view str, u64& pos, InvalidUnicodePolicy policy) {
  const auto size = str.size();
  ROCKET_CHECK(pos, pos < size);
  UChar32 cp; // NOLINT
  i32 i = safe<i32>(pos);
  i32 oldI = i;
  U8_NEXT(str.data(), i, safe<i32>(size), cp); // NOLINT
  ROCKET_DEBUG_ASSERT(i > oldI, "`U8_NEXT` failed");
  if (cp < 0) {
    if (policy == InvalidUnicodePolicy::Throw) {
      throw InputFailure(pos, "Invalid UTF-8 byte sequence");
    } else {
      pos = i;
      return U'�';
    }
  }
  pos = i;
  return static_cast<char32>(cp);
}

Cow<string_view, string>
validate(string_view str, InvalidUnicodePolicy policy, UnorderedBimap<u64, u64>* positions) { // NOLINT(*-complexity)
  Cow<string_view, string> ret(str);

  if (positions != nullptr) {
    positions->clear();
  }

  auto addPosition = [&](u64 i) {
    if (positions != nullptr) {
      if (not ret.modified()) {
        positions->insert({ i, i });
      } else {
        positions->insert({ i , ret.get().size() });
      }
    }
  };

  u64 i = 0, size = str.size();
  while (i < size) {
    addPosition(i);

    UChar32 cp; // NOLINT
    auto oldI = i;
    U8_NEXT(str.data(), i, size, cp); // NOLINT
    ROCKET_DEBUG_ASSERT(i > oldI, "`U8_NEXT` failed");
    if (cp >= 0) {
      // Valid code point
      if (ret.modified()) {
        ret.owned().append(&str[oldI], i - oldI);
      }
    } else {
      // Invalid code point
      if (policy == InvalidUnicodePolicy::Throw) {
        throw InputFailure(oldI, "Invalid UTF-8 byte sequence");
      }
      if (not ret.modified()) {
        ret = string(str.data(), oldI);
      }
      ret.owned().append("�");
    }
  }

  addPosition(str.size());

  return ret;
}

} // namespace utf8

// UTF-32 ...................................................................................................

namespace utf32 {

CodePoint
nextCodePoint(u32string_view str, u64& pos, InvalidUnicodePolicy policy) {
  const auto size = str.size();
  ROCKET_CHECK(pos, pos < size);
  char32 c = str[pos];
  if (not CodePoint::valid(c)) {
    if (policy == InvalidUnicodePolicy::Throw) {
      throw InputFailure(pos, fmt::format("Invalid code-point value 0x{:X}", static_cast<u32>(c)));
    } else {
      ++pos;
      return U'�';
    }
  }
  ++pos;
  return c;
}

Cow<u32string_view, u32string>
validate(u32string_view str, InvalidUnicodePolicy policy, UnorderedBimap<u64, u64>* positions) {
  Cow<u32string_view, u32string> ret(str);

  if (positions != nullptr) {
    positions->clear();
  }

  auto addPosition = [&](u64 i) {
    if (positions != nullptr) {
      positions->insert({ i, i });
    }
  };

  for (u64 i = 0, size = str.size(); i < size; ++i ) {
    addPosition(i);

    const char32 c = str[i];
    if (CodePoint::valid(c)) {
      // Valid code point
      if (ret.modified()) {
        ret.owned().push_back(c);
      }
    } else {
      // Invalid code point
      if (policy == InvalidUnicodePolicy::Throw) {
        throw InputFailure(i, fmt::format("Invalid code-point value 0x{:X}", static_cast<u32>(c)));
      }
      if (not ret.modified()) {
        ret = u32string(str.data(), i);
      }
      ret.owned().push_back(U'�');
    }
  }

  addPosition(str.size());

  return ret;
}

} // namespace utf32

} // namespace rocket::unicode

// EOF
