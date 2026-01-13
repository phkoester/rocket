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
Iterator<C>::Iterator(IteratorType type, basic_string_view<C> input, const locale& loc) :
    input_(input) {
  // 1. Make the `UnicodeString`

  if constexpr (is_same_v<C, char>) {
    us_ = icu::UnicodeString::fromUTF8(input);
  } else {
    us_ = icu::UnicodeString::fromUTF32(reinterpret_cast<const UChar32*>(input.data()), input.size());
  }
  ROCKET_CHECK(input, not us_.isBogus());

  // 2. Loop through the `UnicodeString` and populate `usToTxt_`

  int32_t u16length = us_.length();
  const UChar* u16Buf = us_.getBuffer();
  int32_t u16Index = 0;
  UChar32 u16cp;

  size_t inputLength = input.size();
  size_t inputIndex = 0;
  UChar32 inputCp;

  while (u16Index < u16length) {
    // Add a mapping for this input position
    usToInput_.insert({ static_cast<size_t>(u16Index), inputIndex });

    // Get next U16 code point
    U16_NEXT(u16Buf, u16Index, u16length, u16cp);

    // Get next input code point (UTF-8 or UTF-32)
    ROCKET_CHECK(input, inputIndex < inputLength);
    if constexpr (is_same_v<C, char>) {
      // UTF-8: Use `U8_NEXT` to loop through `input`
      U8_NEXT(input.data(), inputIndex, inputLength, inputCp);
    } else {
      // UTF-32: Easy
      inputCp = input[inputIndex++];
    }

    // Verify the code points match
    ROCKET_CHECK(input, inputCp == u16cp);
  }

  // Add a mapping for the end of the input
  usToInput_.insert({ static_cast<size_t>(u16Index), inputIndex });

  // 3. Create the `BreakIterator`

  icu::Locale icuLoc(loc.name().c_str());
  ROCKET_CHECK(loc, not icuLoc.isBogus());

  UErrorCode status = U_ZERO_ERROR;
  switch (type) {
  case IteratorType::Char:
    iter_.reset(icu::BreakIterator::createCharacterInstance(icuLoc, status));
    break;
  case IteratorType::Line:
    iter_.reset(icu::BreakIterator::createLineInstance(icuLoc, status));
    break;
  case IteratorType::Sentence:
    iter_.reset(icu::BreakIterator::createSentenceInstance(icuLoc, status));
    break;
  case IteratorType::Title:
    iter_.reset(icu::BreakIterator::createTitleInstance(icuLoc, status));
    break;
  case IteratorType::Word:
    iter_.reset(icu::BreakIterator::createWordInstance(icuLoc, status));
    break;
  default:
    ROCKET_FAIL("Invalid iterator type: {}", static_cast<int>(type));
  }
  ROCKET_EXPECT(U_SUCCESS(status));

  // Assign the `UnicodeString`
  iter_->setText(us_);
}

// Template instantiations ----------------------------------------------------------------------------------

template struct Iterator<char>;
template struct Iterator<char32_t>;

} // namespace rocket::unicode

// EOF
