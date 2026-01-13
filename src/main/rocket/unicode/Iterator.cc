/*
 * Iterator.cc
 */

#include "Iterator.h"

#include "rocket/assert.h"

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

namespace rocket::unicode {

// `Iterator` -----------------------------------------------------------------------------------------------

template<typename C> requires Character<C>
Iterator<C>::Iterator(IteratorType type, basic_string_view<C> text, const locale& loc) :
    text_(text) {
  // 1. Make the `UnicodeString`

  if constexpr (is_same_v<C, char>) {
    us_ = icu::UnicodeString::fromUTF8(text);
  } else {
    us_ = icu::UnicodeString::fromUTF32(reinterpret_cast<const UChar32*>(text.data()), text.size());
  }
  ROCKET_CHECK(text, not us_.isBogus());

  // 2. Loop through the `UnicodeString` and populate `usToTxt_`

  int32_t u16length = us_.length();
  const UChar* u16Buf = us_.getBuffer();
  int32_t u16Index = 0;
  UChar32 u16cp;

  size_t textLength = text.size();
  size_t textIndex = 0;
  UChar32 textCp;

  while (u16Index < u16length) {
    // Add a mapping for this text position
    usToText_.insert({ static_cast<size_t>(u16Index), textIndex });

    // Get next U16 code point
    U16_NEXT(u16Buf, u16Index, u16length, u16cp);

    // Get next text code point (UTF-8 or UTF-32)
    ROCKET_CHECK(text, textIndex < textLength);
    if constexpr (is_same_v<C, char>) {
      // UTF-8: Use `U8_NEXT` to loop through `text`
      U8_NEXT(text.data(), textIndex, textLength, textCp);
    } else {
      // UTF-32: Easy
      textCp = text[textIndex++];
    }

    // Verify the code points match
    ROCKET_CHECK(text, textCp == u16cp);
  }

  // Add a mapping for the end of the text
  usToText_.insert({ static_cast<size_t>(u16Index), textIndex });

  // 3. Create the `BreakIterator`

  icu::Locale locale(loc.name().c_str());
  ROCKET_CHECK(loc, not locale.isBogus());

  UErrorCode status = U_ZERO_ERROR;
  switch (type) {
  case IteratorType::Char:
    iter_.reset(icu::BreakIterator::createCharacterInstance(locale, status));
    break;
  case IteratorType::Line:
    iter_.reset(icu::BreakIterator::createLineInstance(locale, status));
    break;
  case IteratorType::Sentence:
    iter_.reset(icu::BreakIterator::createSentenceInstance(locale, status));
    break;
  case IteratorType::Title:
    iter_.reset(icu::BreakIterator::createTitleInstance(locale, status));
    break;
  case IteratorType::Word:
    iter_.reset(icu::BreakIterator::createWordInstance(locale, status));
    break;
  default:
    ROCKET_FAIL("Invalid iterator type: {}", static_cast<int>(type));
  }
  ROCKET_EXPECT(U_SUCCESS(status));

  // Assign the text to iterator
  iter_->setText(us_);
}

// Template instantiations ----------------------------------------------------------------------------------

template struct Iterator<char>;
template struct Iterator<char32_t>;

} // namespace rocket::unicode

// EOF
