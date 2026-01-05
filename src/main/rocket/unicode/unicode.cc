/*
 * unicode.cc
 */

#include "unicode.h"

#include "rocket/assert.h"
#include "rocket/numeric.h"
#include "rocket/str/str.h"
#include "rocket/unicode/iterator.h"
#include "rocket/unicode/internal/block.h"

#include <unicodelib.h>
#include <unicodelib_encodings.h>

#include <numeric>

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
  ROCKET_CHECK(v, isascii(v));
}

CodePoint::operator string() const {
  return utf32To8(operator u32string());
}

CodePoint
CodePoint::lower() const {
  return unicodelib::simple_lowercase_mapping(v_);
}

bool
CodePoint::print(int8_t* width) const {
  // Block: Special
  if (v_ >= 0xfff0U && v_ <= 0xffffU)
    return false;
  int8_t w = this->width();
  if (width)
    *width = w;
  return w > 0;
}

CodePoint
CodePoint::upper() const {
  return unicodelib::simple_uppercase_mapping(v_);
}

bool
CodePoint::whitespace() const {
  return unicodelib::is_white_space(v_);
}

int8_t
CodePoint::width() const {
  // NUL
  if (v_ == 0) {
    return 0;
  }

  // C0 controls, DEL
  if (v_ <= 31 || v_ == 127) {
    return -1;
  }

  // C1 controls
  if (v_ >= 128 && v_ <= 159) {
    return -1;
  }

  // General category Mn or Me
  auto gc = unicodelib::_general_category_properties::get_value(v_);
  if (gc == unicodelib::GeneralCategory::Nonspacing_Mark ||
      gc == unicodelib::GeneralCategory::Enclosing_Mark) {
    return 0;
  }

  // Soft hyphen
  if (v_ == 0x00adU) {
    return 1;
  }

  // General category Cf, Zero Width Space
  if (gc == unicodelib::GeneralCategory::Cf || v_ == 0x200bU) {
    return 0;
  }

  // Hangul Jamo medial vowels and final consonants
  if (v_ >= 0x1160U && v_ <= 0x11ffU) {
    return 0;
  }

  // Spacing characters in the East Asian Wide (W) or East Asian Full-width (F) category
  auto eaw = internal::eastAsianWidth(v_);
  if (eaw == internal::EastAsianWidth::wide || eaw == internal::EastAsianWidth::fullWidth) {
    return 2;
  }

  // From `unicode-display-width`: Emoji characters in the Emoji_Presentation category
  if (internal::emojiEmoji_Presentation(v_)) {
    return 2;
  }

  return 1;
}

ostream&
operator<<(ostream& lhs, CodePoint rhs) {
  return lhs << fmt::format("{}", rhs);
}

size_t
read(nio::Source& in, CodePoint& v) {
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
    v = buf32[0];
  }

  return in.tell() - pos;
}

// `Grapheme` -----------------------------------------------------------------------------------------------

Grapheme::Grapheme(const CodePoints& cps) :
    codePoints(cps),
    width(unicode::width(codePoints)) {}

Grapheme::Grapheme(CodePoints&& cps) :
    codePoints(std::move(cps)),
    width(unicode::width(codePoints)) {}

Grapheme::Grapheme(string_view s) : Grapheme(unicode::codePoints(s)) {}

Grapheme::Grapheme(u32string_view s) : Grapheme(unicode::codePoints(s)) {}

Grapheme::operator string() const {
  return utf32To8(operator u32string());
}

Grapheme::operator u32string() const {
  u32string ret;
  ret.reserve(codePoints.size());
  copy(codePoints.begin(), codePoints.end(), back_inserter(ret));
  return ret;
}

bool
Grapheme::print() const {
  switch (codePoints.size()) {
  case 0: return false;
  case 1: return codePoints[0].print();
  default: return true;
  }
}

ostream&
operator<<(ostream& lhs, const Grapheme& rhs) {
  return lhs << fmt::format("{}", rhs);
}

size_t
read(nio::Source& in, Grapheme& v) {
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
      v = Grapheme(input);
      return in.tell() - pos1;
    }

    // If grapheme boundary, finish

    input.push_back(cp);
    if (CodePointIterator<char32_t>(input, input.size() - 1).graphemeBoundary()) {
      in.seek(pos2);
      v = Grapheme(input.substr(0, input.size() - 1));
      return in.tell() - pos1;
    }
  }
}

// Functions ------------------------------------------------------------------------------------------------

u32string
asciiTo32(string_view s) {
  u32string ret(s.size(), ' ');
  transform(s.begin(), s.end(), ret.begin(), [](char c) {
    ROCKET_CHECK(s, isascii(c));
    return static_cast<char32_t>(c);
  });
  return ret;
}

u32string
utf8To32(string_view s) {
  u32string ret;
  unicodelib::utf8::decode(s.data(), s.size(), ret);
  return ret;
}

string
utf32To8(u32string_view s) {
  string ret;
  unicodelib::utf8::encode(s.data(), s.size(), ret);
  return ret;
}

uint8_t
width(const CodePoints& cps) {
  uint8_t ret = 0;
  for (auto cp : cps) {
    // From `unicode-display-width`
    if (cp == 0xfe0fU)
      return 2;
    int8_t cw = cp.width();
    if (cw > 0) // Ignore nonpositive values
      ret = max(static_cast<uint8_t>(cw), ret);
    if (ret == 2)
      return 2;
  }
  return ret;
}

size_t
width(const Graphemes& grs, size_t index, size_t n) {
  auto begin = grs.begin() + index;
  auto end = n == NPOS ? grs.end() : begin + n;

  return accumulate(begin, end, 0UL, [](size_t n, const Grapheme& gr) {
    return add<size_t>(n, gr.width);
  });
}

// UTF-8 ....................................................................................................

namespace utf8 {

uint8_t
codePointSize(char c) {
  return unicodelib::utf8::codepoint_length(&c, 1);
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
  return CodePointIterator<char>(s, s.size()).codePointPosition();
}

size_t
countGraphemes(string_view s) {
  return GraphemeIterator<char>(s, s.size()).graphemePosition();
}

Graphemes
graphemes(string_view s, UnorderedBimap<size_t, size_t>* positions) {
  if (positions)
    positions->clear();
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
