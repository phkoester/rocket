/*
 * test-icu.cc
 *
 * Tests related to the ICU library.
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/UnorderedBimap.h"
#include "rocket/assert.h"

#include <unicode/brkiter.h>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/utf8.h>
#include <unicode/utf16.h>

#include <locale>
#include <memory>

using namespace icu;
using namespace rocket;
using namespace std;
using namespace testing;

// Iterator -------------------------------------------------------------------------------------------------

template<typename C> requires Character<C>
struct Iterator {
  enum Type { Character, Line, Sentence, Title, Word };

  Iterator(Type type, const std::locale& loc);

private:

  std::basic_string_view<C> text_;
  icu::UnicodeString us_;
  std::unique_ptr<icu::BreakIterator> iter_;
  /// Maps `C` indices in `text_` to `UChar` indices in `us_`.
  UnorderedBimap<int32_t, int32_t> textToUs_;
};

template struct Iterator<char>;
template struct Iterator<char32_t>;

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
  character,
  line,
  sentence,
  title,
  word
};

void loopWithMacros(const UnicodeString& s) {
  int32_t length = s.length();
  const UChar* buffer = s.getBuffer(); // Direct access to UTF-16 buffer
  int32_t i = 0;
  UChar32 cp;

  while (i < length) {
      // This macro extracts the code point and increments 'i' automatically
      U16_NEXT(buffer, i, length, cp);
      cout << "U16_NEXT code point: " << hex << cp << dec << ", i=" << i << endl;
  }
}

void
dumpSegments(SegmentType type, const std::locale& loc, string_view s) {
  Locale icuLoc(loc.name().c_str());
  ROCKET_EXPECT(not icuLoc.isBogus());

  UnicodeString text = UnicodeString::fromUTF8(s);
  loopWithMacros(text);

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
    case SegmentType::character:
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

  cout << "FORWARD\n";
  cout << "-------\n";

  int32_t end = iter->last();
  cout << "last.current=" << iter->current() << endl;

  // 3. Iterate through the grapheme clusters
  int32_t begin = iter->first();
  end = iter->next();

  while (end != BreakIterator::DONE) {
    UnicodeString seg;
    text.extractBetween(begin, end, seg);

    // Print the cluster (converting back to UTF-8 for console output)
    string out;
    seg.toUTF8String(out);
    cout << "Segment: [" << out << "], U16 length=" << (end - begin) << ", U32 length=" << seg.countChar32() << ", current=" << iter->current() << endl;

    begin = end;
    end = iter->next();
  }

  cout << "BACKWARD\n";
  cout << "--------\n";

  end = iter->last();
  cout << "current=" << iter->current() << endl;
  begin = iter->previous();

  while (begin != BreakIterator::DONE) {
    UnicodeString seg;
    text.extractBetween(begin, end, seg);
    string out;
    seg.toUTF8String(out);
    cout << "Segment: [" << out << "], U16 length=" << (end - begin) << ", U32 length=" << seg.countChar32() << ", current=" << iter->current() << endl;

    end = begin;
    begin = iter->previous();
  }
}

// TEST -----------------------------------------------------------------------------------------------------

TEST(icu, graphemeClusters) {
  dumpSegments(SegmentType::character, std::locale(), "a🧑‍🌾\r\nb");
}

// EOF
