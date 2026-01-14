/*
 * unicode.cc
 */

#include "unicode.h"

#include "rocket/assert.h"
#include "rocket/numeric.h"

#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/utf8.h>

using namespace icu;
using namespace rocket;
using namespace rocket::unicode;
using namespace std;

namespace rocket::unicode {

// `CodePoint` ----------------------------------------------------------------------------------------------

CodePoint::CodePoint(char v) :
    v_(static_cast<unsigned char>(v)) {
  ROCKET_CHECK(v, isAscii());
}

CodePoint::operator string() const {
  return utf32To8(operator u32string());
}

bool
CodePoint::isPrint() const {
  return u_isprint(v_) != 0;
}

bool
CodePoint::isWhitespace() const {
  return u_isWhitespace(v_) != 0;
}

CodePoint
CodePoint::lower() const {
  return static_cast<char32_t>(u_tolower(v_));
}

CodePoint
CodePoint::upper() const {
  return static_cast<char32_t>(u_toupper(v_));
}

uint8_t
CodePoint::width() const {
  if (not isPrint()) {
    return 0;
  }

  auto generalCategory = u_getIntPropertyValue(v_, UCHAR_GENERAL_CATEGORY);
  switch (generalCategory) {
  case U_ENCLOSING_MARK:
  case U_NON_SPACING_MARK:
    return 0;
  }

  auto eastAsianWidth = u_getIntPropertyValue(v_, UCHAR_EAST_ASIAN_WIDTH);
  switch (eastAsianWidth) {
  case U_EA_FULLWIDTH:
  case U_EA_WIDE:
    return 2;
  }

  if (u_hasBinaryProperty(v_, UCHAR_EMOJI_PRESENTATION)) {
    return 2;
  }

  return 1;
}

ostream&
operator<<(ostream& lhs, CodePoint rhs) {
  return lhs << fmt::format("{}", rhs);
}

// Functions ------------------------------------------------------------------------------------------------

// XXX throw dok.
u32string
utf8To32(string_view s) {
  auto us = UnicodeString::fromUTF8(s);
  ROCKET_CHECK(s, not us.isBogus());
  auto size = us.countChar32();
  u32string ret(size, 0);
  UErrorCode status = U_ZERO_ERROR;
  us.toUTF32(reinterpret_cast<UChar32*>(ret.data()), size, status);
  ROCKET_EXPECT(U_SUCCESS(status));
  return ret;
}

// XXX throw dok.
string
utf32To8(u32string_view s) {
  auto us = UnicodeString::fromUTF32(reinterpret_cast<const UChar32*>(s.data()), s.size());
  ROCKET_CHECK(s, not us.isBogus());
  string ret;
  us.toUTF8String(ret);
  return ret;
}

// UTF-8 ....................................................................................................

namespace utf8 {

CodePoint
nextCodePoint(string_view s, size_t& pos) {
  const auto size = s.size();
  ROCKET_CHECK(pos, pos < size);
  UChar32 cp;
  int32_t i = to<int32_t>(pos);
  U8_NEXT(s.data(), i, size, cp);
  pos = to<size_t>(i);
  ROCKET_ASSERT(pos <= s.size());
  return static_cast<char32_t>(cp);
}

Cow<string_view, string>
validate(string_view s, UnorderedBimap<size_t, size_t>* positions) {
  Cow<string_view, string> ret(s);

  if (positions) {
    positions->clear();
  }

  auto addPosition = [&](size_t i) {
    if (positions) {
      if (not ret.modified()) {
        positions->insert({ i, i });
      } else {
        positions->insert({ i , ret.get().size() });
      }
    }
  };

  size_t i = 0, size  = s.size();
  while (i < size) {
    addPosition(i);

    UChar32 cp;
    auto oldI = i;
    U8_NEXT(s.data(), i, size, cp);
    if (cp >= 0) {
      // Valid code point
      if (ret.modified()) {
        ret.owned().append(&s[oldI], i - oldI);
      }
    } else {
      // Invalid code point
      if (not ret.modified()) {
        ret = string(s.data(), oldI);
      }
      ret.owned().append("�");
    }
  }

  addPosition(s.size());

  return ret;
}

} // namespace utf8

// UTF-32 ...................................................................................................

namespace utf32 {

CodePoint
nextCodePoint(u32string_view s, size_t& pos) {
  const auto size = s.size();
  ROCKET_CHECK(pos, pos < size);
  return s[pos++];
}

Cow<u32string_view, u32string>
validate(u32string_view s, UnorderedBimap<size_t, size_t>* positions) {
  Cow<u32string_view, u32string> ret(s);

  if (positions) {
    positions->clear();
  }

  auto addPosition = [&](size_t i) {
    if (positions) {
      positions->insert({ i, i });
    }
  };

  for (size_t i = 0, size = s.size(); i < size; ++i ) {
    addPosition(i);

    char32_t c = s[i];
    if (CodePoint(c).valid()) {
      // Valid code point
      if (ret.modified()) {
        ret.owned().push_back(c);
      }
    } else {
      // Invalid code point
      if (not ret.modified()) {
        ret = u32string(s.data(), i);
      }
      ret.owned().push_back(U'�');
    }
  }

  addPosition(s.size());

  return ret;
}

} // namespace utf32

} // namespace rocket::unicode

// EOF
