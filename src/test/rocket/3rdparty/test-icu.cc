/*
 * test-icu.cc
 *
 * Tests related to the ICU library.
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/assert.h"
#include "rocket/unicode/unicode.h"

#include <unicode/brkiter.h>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/utf8.h>

#include <locale>
#include <memory>

using namespace icu;
using namespace rocket;
using namespace std;
using namespace testing;

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

void
dumpSegments(SegmentType type, const std::locale& loc, string_view s) {
  Locale icuLoc(loc.name().c_str());
  ROCKET_EXPECT(not icuLoc.isBogus());

  UnicodeString text = UnicodeString::fromUTF8(s);

  UErrorCode status = U_ZERO_ERROR;

  // 2. Create a BreakIterator
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
    UnicodeString seg;
    text.extractBetween(begin, end, seg);

    // Print the cluster (converting back to UTF-8 for console output)
    string out;
    seg.toUTF8String(out);
    cout << "Segment: [" << out << "], U16 length=" << (end - begin) << ", U32 length=" << seg.countChar32() << endl;

    begin = end;
    end = iter->next();
  }
}

// TEST -----------------------------------------------------------------------------------------------------

TEST(icu, graphemeClusters) {
  dumpSegments(SegmentType::grapheme, std::locale(), "a🧑‍🌾b");
}

// EOF
