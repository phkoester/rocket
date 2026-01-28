/*
 * Iterator.cc
 */

#include "Iterator.h"

#include "rocket/assert.h"

#include <unicode/brkiter.h>
#include <unicode/unistr.h>

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

namespace rocket::unicode {

// #IteratorImpl --------------------------------------------------------------------------------------------

/**
 * Nothing from the `icu` namespace may surface in the public API, so we wrap it here.
 */
struct IteratorImpl {
  icu::UnicodeString str;
  unique_ptr<icu::BreakIterator> iter;
};

struct IteratorImplDelete {
  void operator()(IteratorImpl* val) { delete val; }
};

// #Iterator ------------------------------------------------------------------------------------------------

template<typename C> requires IsChar<C>
Iterator<C>::Iterator(IteratorType type, basic_string_view<C> input, const locale& loc) :
    input_(input),
    impl_(new IteratorImpl(), [](IteratorImpl* val) { delete val; }) {
  // 1. Make the #UnicodeString

  auto& str = impl_->str;
  if constexpr (is_same_v<C, char>) {
    str = icu::UnicodeString::fromUTF8(input);
  } else {
    str = icu::UnicodeString::fromUTF32(reinterpret_cast<const UChar32*>(input.data()), input.size());
  }
  ROCKET_CHECK(input, not str.isBogus());

  // 2. Loop through the #UnicodeString and populate #usToInput_

  i32 u16length = str.length();
  const UChar* u16Buf = str.getBuffer();
  i32 u16Index = 0;
  UChar32 u16cp;

  u64 inputLength = input.size();
  u64 inputIndex = 0;
  UChar32 inputCp;

  while (u16Index < u16length) {
    // Add a mapping for this input position
    usToInput_.insert({ static_cast<u64>(u16Index), inputIndex });

    // Get next U16 code point
    U16_NEXT(u16Buf, u16Index, u16length, u16cp);

    // Get next input code point (UTF-8 or UTF-32)
    ROCKET_CHECK(input, inputIndex < inputLength);
    if constexpr (is_same_v<C, char>) {
      // UTF-8: Use #U8_NEXT to loop through #input
      U8_NEXT(input.data(), inputIndex, inputLength, inputCp);
    } else {
      // UTF-32: Easy
      inputCp = input[inputIndex++];
    }

    // Verify the code points match
    const char* msg;
    if constexpr (is_same_v<C, char>) {
      msg = "Invalid UTF-8 input";
    } else {
      msg = "Invalid UTF-32 input";
    }
    ROCKET_CHECK(input, inputCp == u16cp, "{}", msg);
  }

  // Add a mapping for EOI
  usToInput_.insert({ static_cast<u64>(u16Index), inputIndex });

  // 3. Create the #BreakIterator

  icu::Locale icuLoc(loc.name().c_str());
  ROCKET_CHECK(loc, not icuLoc.isBogus());

  auto& iter = impl_->iter;

  UErrorCode status = U_ZERO_ERROR;
  switch (type) {
  case IteratorType::Character:
    iter.reset(icu::BreakIterator::createCharacterInstance(icuLoc, status));
    break;
  case IteratorType::Line:
    iter.reset(icu::BreakIterator::createLineInstance(icuLoc, status));
    break;
  case IteratorType::Sentence:
    iter.reset(icu::BreakIterator::createSentenceInstance(icuLoc, status));
    break;
  case IteratorType::Title:
    iter.reset(icu::BreakIterator::createTitleInstance(icuLoc, status));
    break;
  case IteratorType::Word:
    iter.reset(icu::BreakIterator::createWordInstance(icuLoc, status));
    break;
  default:
    ROCKET_FLOP(type, "Invalid iterator type {}", static_cast<i32>(type));
  }
  ROCKET_EXPECT(U_SUCCESS(status));

  // Assign the #UnicodeString to the #BreakIterator
  iter->setText(str);
}

template<typename C> requires IsChar<C>
u64
Iterator<C>::current() const {
  return usToInput_.left.at(impl_->iter->current());
}

template<typename C> requires IsChar<C>
u64
Iterator<C>::first() {
  auto val = impl_->iter->first();
  return usToInput_.left.at(val);
}

template<typename C> requires IsChar<C>
u64
Iterator<C>::last() {
  auto val = impl_->iter->last();
  return usToInput_.left.at(val);
}

template<typename C> requires IsChar<C>
u64
Iterator<C>::next() {
  auto pos = impl_->iter->next();
  if (pos == icu::BreakIterator::DONE) {
    return NPOS;
  }
  return usToInput_.left.at(pos);
}

template<typename C> requires IsChar<C>
u64
Iterator<C>::previous() {
  auto pos = impl_->iter->previous();
  if (pos == icu::BreakIterator::DONE) {
    return NPOS;
  }
  return usToInput_.left.at(pos);
}

// Template instantiations ----------------------------------------------------------------------------------

template struct Iterator<char>;
template struct Iterator<char32>;

} // namespace rocket::unicode

// EOF
