/*
 * unicode.cc
 */

#include "unicode.h"

#include "rocket/assert.h"
#include "rocket/numeric.h"
#include "rocket/str/str.h"
#include "rocket/unicode/iterator.h" // XXX Ganz weg
#include "rocket/unicode/internal/block.h"

#include <unicodelib.h>
#include <unicodelib_encodings.h>

#include <unicode/uchar.h>
#include <unicode/unistr.h>

#include <numeric>

using namespace icu;
using namespace rocket;
using namespace rocket::unicode;
using namespace std;

namespace unicodelib = ::unicode;

namespace rocket::unicode {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

EastAsianWidth
eastAsianWidth(uint32_t cp) {
  const auto* block = biFind(eastAsianWidthBlocks, cp);
  return block ? block->eastAsianWidth : EastAsianWidth::neutral;
}

} // namespace internal

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
#if 0 // XXX
  // Block: Special
  if (v_ >= 0xfff0U && v_ <= 0xffffU)
    return false;
#endif
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
  case U_NON_SPACING_MARK:
  case U_ENCLOSING_MARK:
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

size_t
read(nio::Source& in, CodePoint& out) {
  auto pos = in.tell();

  string buf;

  char c;
  if (in.read(c) == 0) {
    return 0;
  }
  buf.push_back(c);

  auto cpSize = utf8::codePointSize(c);
  if (cpSize == 0) {
    // Not a UTF-8 code-point boundary
    in.seek(pos);
    return 0;
  }
  for (uint8_t i = 0; i < cpSize - 1; ++i) {
    if (in.read(c) == 0) {
      // Incomplete UTF-8 byte sequence
      in.seek(pos);
      return 0;
    }
    if (i > 0 && not utf8::continuationByte(c)) {
      // Invalid UTF-8 byte sequence
      in.seek(pos);
      return 0;
    }
    buf.push_back(c);
  }

  u32string buf32 = utf8To32(buf);
  if (buf32.size() != 1) {
    // Something went wrong
    in.seek(pos);
    return 0;
  }
  else {
    out = buf32[0];
  }

  return in.tell() - pos;
}

// `Grapheme` -----------------------------------------------------------------------------------------------

Grapheme::Grapheme(string_view s) : Grapheme(unicode::codePoints(s)) {}

Grapheme::Grapheme(u32string_view s) : Grapheme(unicode::codePoints(s)) {}

Grapheme::operator string() const {
  return utf32To8(operator u32string());
}

Grapheme::operator u32string() const {
  u32string ret;
  ret.reserve(codePoints_.size());
  copy(codePoints_.begin(), codePoints_.end(), back_inserter(ret));
  return ret;
}

bool
Grapheme::print() const { // XXX Weg
  switch (codePoints_.size()) {
  case 0: return false;
  case 1: return codePoints_[0].isPrint();
  default: return true;
  }
}

uint8_t
Grapheme::width() const {
  uint8_t ret = 0;
  for (auto cp : codePoints_) {
    ret = max(ret, cp.width());
    if (ret == 2) {
      return 2;
    }
#if 0 // XXX
    // From `unicode-display-width`
    if (cp == 0xfe0fU) {
      return 2;
    }
    int8_t cpw = cp.width();
    // Ignore nonpositive values
    if (cpw > 0) {
      ret = max(static_cast<uint8_t>(cpw), ret);
    }
    if (ret == 2) {
      return 2;
    }
#endif
  }
  return ret;
}

ostream&
operator<<(ostream& lhs, const Grapheme& rhs) {
  return lhs << fmt::format("{}", rhs);
}

size_t
read(nio::Source& in, Grapheme& out) {
  size_t pos1 = in.tell();

  // Read first code point

  CodePoint cp;
  if (read(in, cp) == 0) {
    return 0;
  }

  u32string input { cp };

  // Read more code points

  while (true) {
    // Read next code point

    size_t pos2 = in.tell();
    if (read(in, cp) == 0) {
      // EOF
      out = Grapheme(input);
      return in.tell() - pos1;
    }

    // If grapheme boundary, finish

    input.push_back(cp);
    if (CodePointIterator<char32_t>(input, input.size() - 1).graphemeBoundary()) {
      in.seek(pos2);
      out = Grapheme(input.substr(0, input.size() - 1));
      return in.tell() - pos1;
    }
  }
}

// Functions ------------------------------------------------------------------------------------------------

u32string
utf8To32(string_view s) {
  auto us = UnicodeString::fromUTF8(s);
  auto size = us.countChar32();
  u32string ret(size, 0);
  UErrorCode status = U_ZERO_ERROR;
  us.toUTF32(reinterpret_cast<UChar32*>(ret.data()), size, status);
  if (not U_SUCCESS(status)) {
    ROCKET_PROCESS_ERROR("status={}", u_errorName(status));
  }
  return ret;
}

string
utf32To8(u32string_view s) {
  auto us = UnicodeString::fromUTF32(reinterpret_cast<const UChar32*>(s.data()), s.size());
  string ret;
  us.toUTF8String(ret);
  return ret;
}

size_t
width(const Graphemes& grs, size_t index, size_t n) {
  auto begin = grs.begin() + index;
  auto end = n == NPOS ? grs.end() : begin + n;

  return accumulate(begin, end, 0UL, [](size_t n, const Grapheme& gr) {
    return add<size_t>(n, gr.width());
  });
}

// UTF-8 ....................................................................................................

namespace utf8 {

uint8_t
codePointSize(char c) {
  if ((c & 0x80) == 0) {
    return 1;
  }
  if ((c & 0xE0) == 0xC0) {
    return 2;
  }
  if ((c & 0xF0) == 0xE0) {
    return 3;
  }
  if ((c & 0xF8) == 0xF0) {
    return 4;
  }
  return 0;
}

CodePoints
codePoints(string_view s, UnorderedBimap<size_t, size_t>* positions) {
  if (positions) {
    positions->clear();
  }
  CodePoints ret;
  size_t i = 0;
  auto it = CodePointIterator<char>(s), end = CodePointIterator<char>(s, s.size());
  for (; it != end; ++it) {
    ret.push_back(*it);
    if (positions) {
      positions->insert({ i++, it.position() });
    }
  }
  if (positions) {
    positions->insert({ i++, it.position() });
  }
  return ret;
}

size_t
countCodePoints(string_view s) {
  auto us = UnicodeString::fromUTF8(s);
  return us.countChar32();
}

size_t
countGraphemes(string_view s) {
  // XXX Ohne GrahemeIterator
  return GraphemeIterator<char>(s, s.size()).graphemePosition();
}

Graphemes
graphemes(string_view s, UnorderedBimap<size_t, size_t>* positions) {
  if (positions) {
    positions->clear();
  }
  Graphemes ret;
  size_t i = 0;
  auto it = GraphemeIterator<char>(s), end = GraphemeIterator<char>(s, s.size());
  for (; it != end; ++it) {
    ret.push_back(*it);
    if (positions) {
      positions->insert({ i++, it.position() });
    }
  }
  if (positions) {
    positions->insert({ i++, it.position()});
  }
  return ret;
}

bool
valid(string_view s, string* out) {
  if (out) {
    out->clear();
  }

  bool ret = true;

  for (size_t i = 0, size = s.size(); i < size;) {
    char c = s[i];
    auto cpSize = codePointSize(c);
    if (cpSize == 0) {
      // Invalid UTF-8 byte
      if (out) {
        ret = false;
        out->append("�");
      } else {
        return false;
      }
      ++i;
    } else if (i + cpSize > size) {
      // Incomplete UTF-8 byte sequence
      if (not out) {
        return false;
      }
      ret = false;
      out->append(str::repeat<char>("�", size - i));
      break;
    } else if (cpSize == 1) {
      if (out) {
        out->push_back(c);
      }
      ++i;
    } else {
      // Multi-byte sequence: Check that all following bytes are continuation bytes
      bool valid = true;
      for (uint8_t j = 1; j < cpSize; ++j) {
        if (not utf8::continuationByte(s[i + j])) {
          valid = false;
          break;
        }
      }
      if (valid) {
        if (out) {
          out->append(s.substr(i, cpSize));
        }
      } else {
        // Invalid UTF-8 byte sequence
        if (not out) {
          return false;
        }
        ret = false;
        out->append(str::repeat<char>("�", cpSize));
      }
      i += cpSize;
    }
  }

  return ret;
}

} // namespace utf8

// UTF-32 ...................................................................................................

namespace utf32 {

CodePoints
codePoints(u32string_view s, UnorderedBimap<size_t, size_t>* positions) {
  if (positions) {
    positions->clear();
  }
  CodePoints ret;
  ret.reserve(s.size());
  copy(s.begin(), s.end(), back_inserter(ret));
  if (positions) {
    for (size_t i = 0, size = s.size(); i <= size; ++i) {
      positions->insert({ i, i });
    }
  }
  return ret;
}

size_t
countGraphemes(u32string_view s) {
  return GraphemeIterator<char32_t>(s, s.size()).graphemePosition();
}

Graphemes
graphemes(u32string_view s, UnorderedBimap<size_t, size_t>* positions) {
  if (positions) {
    positions->clear();
  }
  Graphemes ret;
  size_t i = 0;
  auto it = GraphemeIterator<char32_t>(s), end = GraphemeIterator<char32_t>(s, s.size());
  for (; it != end; ++it) {
    ret.push_back(*it);
    if (positions) {
      positions->insert({ i++, it.position() });
    }
  }
  if (positions) {
    positions->insert({ i++, it.position() });
  }
  return ret;
}

} // namespace utf32

} // namespace rocket::unicode

// EOF
