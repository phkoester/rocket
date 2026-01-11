/*
 * test-icu.cc
 *
 * Tests related to the STL.
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/Process.h"
#include "rocket/assert.h"
#include "rocket/nio/nio.h"

#include <unicode/brkiter.h>
#include <unicode/uchar.h>
#include <unicode/unistr.h>

#include <locale>
#include <memory>

using namespace icu;
using namespace rocket;
using namespace std;
using namespace testing;

// Constants ------------------------------------------------------------------------------------------------

constexpr char TWO_BYTES    = 0b1101'1111;
constexpr char THREE_BYTES  = 0b1110'1111;
constexpr char FOUR_BYTES   = 0b1111'0111;
constexpr char CONT         = 0b1011'1111;

// Functions ------------------------------------------------------------------------------------------------

void
dumpIcuLocale(string_view what, const Locale& locale) {
  cout << what << ": name=" << locale.getName() << ", country=" << locale.getCountry() << ", lang=" << locale.getLanguage() << ", var=" << locale.getVariant() << ", script=" << locale.getScript() << endl;
  UErrorCode status = U_ZERO_ERROR;
  StringEnumeration *keywords = locale.createKeywords(status);
  if (keywords) {
    while (const char *keyword = keywords->next(nullptr, status)) {
      cout << "  Keyword: " << keyword << endl;
    }
    delete keywords;
  }
}

enum SegmentType {
  word,
  line,
  grapheme,
  sentence,
  title
};

int getCpWIdth(char32_t cp) {
  if (not u_isprint(cp))
    return 0;
  int32_t width_prop = u_getIntPropertyValue(cp, UCHAR_EAST_ASIAN_WIDTH);
  switch (width_prop) {
    case U_EA_WIDE:
    case U_EA_FULLWIDTH:
      return 2;
    case U_EA_NEUTRAL:
    case U_EA_HALFWIDTH:
    case U_EA_NARROW:
    case U_EA_AMBIGUOUS:
      return 1;
    default:
      ROCKET_TERMINATE("Invalid width property: {}", width_prop);
  }
}

int getGraphemeWidth(const icu::UnicodeString& segment) {
  int total_width = 0;
  int32_t i = 0;
  while (i < segment.length()) {
      UChar32 cp = segment.char32At(i);
      int width = getCpWIdth(cp);
      total_width += width;
      ROCKET_ASSERT(total_width <= 2);
      if (total_width == 2)
        return 2;

      i += U16_LENGTH(cp); // Move to next code point
  }
  return total_width;
}

void
dumpSegments(SegmentType type, const std::locale& loc, string_view s) {
  Locale icuLoc(loc.name().c_str());
  ROCKET_EXPECT(not icuLoc.isBogus());

  UnicodeString text = UnicodeString::fromUTF8(s);

  UErrorCode status = U_ZERO_ERROR;

  // 2. Create a Character BreakIterator
  unique_ptr<BreakIterator> iter;
  switch (type) {
    case SegmentType::word:
      iter.reset(BreakIterator::createWordInstance(icuLoc, status));
      break;
    case SegmentType::line:
      iter.reset(BreakIterator::createLineInstance(icuLoc, status));
      break;
    case SegmentType::grapheme:
      iter.reset(BreakIterator::createCharacterInstance(icuLoc, status));
      break;
    case SegmentType::sentence:
      iter.reset(BreakIterator::createSentenceInstance(icuLoc, status));
      break;
    case SegmentType::title:
      iter.reset(BreakIterator::createTitleInstance(icuLoc, status));
      break;
    default:
      ROCKET_FAIL("Invalid segment type: {}", static_cast<int>(type));
  }
  ROCKET_EXPECT(U_SUCCESS(status));

  iter->setText(text);

  // 3. Iterate through the grapheme clusters
  int32_t begin = iter->first();
  int32_t end = iter->next();

  while (end != BreakIterator::DONE) {
    UnicodeString segment;
    text.extractBetween(begin, end, segment);

    // Print the cluster (converting back to UTF-8 for console output)
    string out;
    segment.toUTF8String(out);
    cout << "Segment: [" << out << "], U16 length=" << (end - begin) <<
      ", U32 length=" << segment.countChar32();
    if (type == SegmentType::grapheme) {
      cout << ", width=" << getGraphemeWidth(segment);
    }
    cout << endl;

    begin = end;
    end = iter->next();
  }
}

bool
isLower(char32_t cp) {
  return u_islower(cp);
}

bool
isSpace(char32_t cp) {
  return u_isspace(cp);
}

bool
isUpper(char32_t cp) {
  return u_isupper(cp);
}

bool
isWhitespace(char32_t cp) {
  return u_isWhitespace(cp);
}

char32_t
toLower(char32_t cp) {
  return u_tolower(cp);
}

char32_t
toUpper(char32_t cp) {
  return u_toupper(cp);
}

u32string
utf8ToUtf32(string_view s) {
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
utf32ToUtf8(u32string_view s) {
  auto us = UnicodeString::fromUTF32(reinterpret_cast<const UChar32*>(s.data()), s.size());
  string ret;
  us.toUTF8String(ret);
  return ret;
}

// TEST -----------------------------------------------------------------------------------------------------

TEST(icu, graphemeClusters) {
  dumpSegments(SegmentType::grapheme, std::locale(), "a🧑‍🌾b");
}

TEST(icu, toLower) {
  EXPECT_EQ(toLower(U'A'), U'a');
  EXPECT_EQ(toLower(U'Ä'), U'ä');
}

TEST(icu, toUpper) {
  EXPECT_EQ(toUpper(U'a'), U'A');
  EXPECT_EQ(toUpper(U'ä'), U'Ä');
}

TEST(icu, utf8ToUtf32) {
  // Valid UTF-8

  EXPECT_EQ(utf8ToUtf32(""), U"");
  EXPECT_EQ(utf8ToUtf32("a"), U"a");
  EXPECT_EQ(utf8ToUtf32("abc äöü €"), U"abc äöü €");
  // U+1F9D1 (ADULT), U+200D (ZERO WIDTH JOINER), U+1F33E (EAR OF RICE)
  EXPECT_EQ(utf8ToUtf32("a🧑‍🌾b"), U"a🧑‍🌾b");

  auto s32 = utf8ToUtf32("🧑‍🌾");
  ASSERT_EQ(s32.size(), 3);
  EXPECT_EQ(s32[0], 0x1F9D1);
  EXPECT_EQ(s32[1], 0x200D);
  EXPECT_EQ(s32[2], 0x1F33E);

  // Invalid UTF-8

  EXPECT_EQ(utf8ToUtf32(string { CONT }), U"\uFFFD"); // U+FFFD (REPLACEMENT CHARACTER)
  EXPECT_EQ(utf8ToUtf32(string { CONT }), U"�");
  EXPECT_EQ(utf8ToUtf32(string { CONT, 'a', 'b' }), U"�ab");

  EXPECT_EQ(utf8ToUtf32(string { TWO_BYTES }), U"�");
  EXPECT_EQ(utf8ToUtf32(string { TWO_BYTES, 'a' }), U"�a");
  EXPECT_EQ(utf8ToUtf32(string { TWO_BYTES, 'a', 'b' }), U"�ab");

  EXPECT_EQ(utf8ToUtf32(string { THREE_BYTES }), U"�");
  EXPECT_EQ(utf8ToUtf32(string { THREE_BYTES, 'a', 'b' }), U"�ab");
  EXPECT_EQ(utf8ToUtf32(string { THREE_BYTES, CONT, 'a', 'b' }), U"�ab");

  EXPECT_EQ(utf8ToUtf32(string { FOUR_BYTES }), U"�");
  EXPECT_EQ(utf8ToUtf32(string { FOUR_BYTES, 'a', 'b' }), U"�ab");
  EXPECT_EQ(utf8ToUtf32(string { FOUR_BYTES, CONT, 'a', 'b' }), U"��ab");
  EXPECT_EQ(utf8ToUtf32(string { FOUR_BYTES, CONT, CONT, 'a', 'b' }), U"���ab");
}

TEST(icu, utf32ToUtf8) {
  EXPECT_EQ(utf32ToUtf8(U"abc äöü €"), "abc äöü €");
  // U+1F9D1 (ADULT), U+200D (ZERO WIDTH JOINER), U+1F33E (EAR OF RICE)
  EXPECT_EQ(utf32ToUtf8(U"a🧑‍🌾b"), "a🧑‍🌾b");
}

TEST(icu, getCpWIdth) {
  EXPECT_EQ(getCpWIdth(U'a'), 1);
  EXPECT_EQ(getCpWIdth(U'🧑'), 2);
  EXPECT_EQ(getCpWIdth(U'\u200D'), 0);
}
// EOF
