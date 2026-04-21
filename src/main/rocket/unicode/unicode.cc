/*
 * unicode.cc
 */

#include "unicode.h"

#include "rocket/assert.h"

#include <boost/safe_numerics/safe_integer.hpp>

#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/utf8.h>
#include <unicode/utypes.h>

#include <array>

using boost::safe_numerics::safe;

using namespace icu;
using namespace rocket;
using namespace rocket::unicode;
using namespace std;

namespace rocket::unicode {

// #CodePoint -----------------------------------------------------------------------------------------------

CodePoint::operator string() const {
  array<char, 4> buf; // NOLINT
  i32 i = 0;
  UBool error = 0;
  U8_APPEND(buf.data(), i, 4, val_, error); // NOLINT
  ROCKET_EXPECT(error == 0, "Invalid code point {:0>4X}", static_cast<u32>(val_));
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
utf8To32(string_view str) {
  auto us = UnicodeString::fromUTF8(str);
  ROCKET_CHECK(str, not us.isBogus());
  auto size = us.countChar32();
  u32string ret(size, 0);
  UErrorCode status = U_ZERO_ERROR;
  us.toUTF32(reinterpret_cast<UChar32*>(ret.data()), size, status);
  ROCKET_EXPECT(U_SUCCESS(status));
  return ret;
}

string
utf32To8(u32string_view str) {
  auto us = UnicodeString::fromUTF32(reinterpret_cast<const UChar32*>(str.data()), str.size()); // NOLINT
  ROCKET_CHECK(str, not us.isBogus());
  string ret;
  us.toUTF8String(ret);
  return ret;
}

// UTF-8 ....................................................................................................

namespace utf8 {

CodePoint
nextCodePoint(string_view str, u64& pos) {
  const auto size = str.size();
  ROCKET_CHECK(pos, pos < size);
  UChar32 cp; // NOLINT
  i32 i = safe<i32>(pos);
  U8_NEXT(str.data(), i, safe<i32>(size), cp); // NOLINT
  pos = safe<u64>(i);
  return static_cast<char32>(cp);
}

Cow<string_view, string>
validate(string_view str, UnorderedBimap<u64, u64>* positions) { // NOLINT(*-complexity)
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

  u64 i = 0, size  = str.size();
  while (i < size) {
    addPosition(i);

    UChar32 cp; // NOLINT
    auto oldI = i;
    U8_NEXT(str.data(), i, size, cp); // NOLINT
    if (cp >= 0) {
      // Valid code point
      if (ret.modified()) {
        ret.owned().append(&str[oldI], i - oldI);
      }
    } else {
      // Invalid code point
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
nextCodePoint(u32string_view str, u64& pos) {
  const auto size = str.size();
  ROCKET_CHECK(pos, pos < size);
  return str[pos++];
}

Cow<u32string_view, u32string>
validate(u32string_view str, UnorderedBimap<u64, u64>* positions) {
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
    if (CodePoint(c).valid()) {
      // Valid code point
      if (ret.modified()) {
        ret.owned().push_back(c);
      }
    } else {
      // Invalid code point
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
