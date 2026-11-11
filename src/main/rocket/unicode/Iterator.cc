/*
 * Iterator.cc
 */

#include "Iterator.h"

#include "rocket/assert.h"
#include "rocket/icu.h"

#include <algorithm>
#include <array>
#include <limits>
#include <string>
#include <string_view>

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

namespace {

// Local functions ------------------------------------------------------------------------------------------

// Returns the name of `loc` in a form suitable for ICU. A locale combined from several named locales has a
// composite name such as `LC_CTYPE=en_US.UTF-8;LC_NUMERIC=C;...` (libstdc++ and MSVC both use this format),
// which ICU rejects. In that case, return the `LC_CTYPE` component, as character classification is closest
// to what text segmentation depends on. Otherwise, return the name unchanged
string
localeName(const locale& loc) {
  string ret = loc.name();
  if (ret.find('=') == NPOS) {
    return ret;
  }

  constexpr string_view KEY = "LC_CTYPE=";

  for (string_view rest = ret; not rest.empty();) {
    const auto semicolon = rest.find(';');
    const auto entry = rest.substr(0, semicolon);
    if (entry.starts_with(KEY)) {
      return string(entry.substr(KEY.size()));
    }
    if (semicolon == NPOS) {
      break;
    }
    rest.remove_prefix(semicolon + 1);
  }

  return ret;
}

} // namespace

namespace rocket::unicode {

// `Iterator` -----------------------------------------------------------------------------------------------

template<typename C> requires IsChar<C>
Iterator<C>::Iterator(IteratorType type, basic_string_view<C> input, const locale& loc) : // NOLINT(*-complexity)
  input_(input) {
  // ICU break positions are `int32_t`
  ROCKET_CHECK(input, input.size() <= static_cast<u64>(numeric_limits<i32>::max()), "Input too long");

  // 1. Validate the input and, for UTF-32, convert it to UTF-16

  const auto inputLength = static_cast<i32>(input.size());
  if constexpr (same_as<C, char>) {
    for (i32 i = 0; i < inputLength;) {
      UChar32 cp; // NOLINT
      U8_NEXT(input.data(), i, inputLength, cp);
      ROCKET_CHECK(input, cp >= 0, "Invalid UTF-8 input");
    }
  } else {
    u16_.reserve(input.size());
    for (auto c : input) {
      const auto cp = static_cast<u32>(c);
      ROCKET_CHECK(input, cp <= 0x10FFFF and not U_IS_SURROGATE(cp), "Invalid UTF-32 input");
      if (U_IS_BMP(cp)) {
        u16_.push_back(static_cast<char16_t>(cp));
      } else {
        supplementaries_.push_back(static_cast<i32>(u16_.size()));
        u16_.push_back(static_cast<char16_t>(U16_LEAD(cp)));
        u16_.push_back(static_cast<char16_t>(U16_TRAIL(cp)));
      }
    }
    ROCKET_CHECK(input, u16_.size() <= static_cast<u64>(numeric_limits<i32>::max()), "Input too long");
  }

  // 2. Resolve the locale

  UErrorCode status = U_ZERO_ERROR;
  array<char, ULOC_FULLNAME_CAPACITY> locName {};
  uloc_getName(localeName(loc).c_str(), locName.data(), static_cast<i32>(locName.size()), &status);
  ROCKET_CHECK(loc, U_SUCCESS(status) and status != U_STRING_NOT_TERMINATED_WARNING, "Invalid locale");

  // 3. Create the break iterator

  UBreakIteratorType ubrkType; // NOLINT
  switch (type) {
    case IteratorType::Character:
      ubrkType = UBRK_CHARACTER;
      break;
    case IteratorType::Line:
      ubrkType = UBRK_LINE;
      break;
    case IteratorType::Sentence:
      ubrkType = UBRK_SENTENCE;
      break;
    case IteratorType::Word:
      ubrkType = UBRK_WORD;
      break;
    default:
      ROCKET_FLOP(type, "Invalid iterator type {}", static_cast<i32>(type));
  }

  status = U_ZERO_ERROR;
  iter_.reset(ubrk_open(ubrkType, locName.data(), nullptr, 0, &status));
  ROCKET_EXPECT(U_SUCCESS(status));

  // 4. Assign the text to the break iterator

  if constexpr (same_as<C, char>) {
    // Iterate over the UTF-8 input directly. A UTF-8 `UText` uses byte offsets as native indices, so the
    // break positions are positions in `input_`. The break iterator makes a shallow clone of the `UText`, so
    // we may close ours right away; the text itself must stay valid, which is a precondition anyway
    UText* text = utext_openUTF8(nullptr, input.data(), inputLength, &status);
    ROCKET_EXPECT(U_SUCCESS(status));
    ubrk_setUText(iter_.get(), text, &status);
    utext_close(text);
  } else {
    ubrk_setText(iter_.get(), reinterpret_cast<const UChar*>(u16_.data()), static_cast<i32>(u16_.size()), &status);
  }
  ROCKET_EXPECT(U_SUCCESS(status));
}

template<typename C> requires IsChar<C>
u64
Iterator<C>::current() const {
  return toInput(ubrk_current(iter_.get()));
}

template<typename C> requires IsChar<C>
u64
Iterator<C>::first() {
  return toInput(ubrk_first(iter_.get()));
}

template<typename C> requires IsChar<C>
u64
Iterator<C>::last() {
  return toInput(ubrk_last(iter_.get()));
}

template<typename C> requires IsChar<C>
u64
Iterator<C>::next() {
  return toInput(ubrk_next(iter_.get()));
}

template<typename C> requires IsChar<C>
u64
Iterator<C>::previous() {
  return toInput(ubrk_previous(iter_.get()));
}

template<typename C> requires IsChar<C>
u64
Iterator<C>::toInput(i32 pos) const {
  if (pos == UBRK_DONE) {
    return NPOS;
  }
  if constexpr (same_as<C, char>) {
    // UTF-8 positions are native `UText` indices, i.e., byte offsets into `input_`
    return static_cast<u64>(pos);
  } else {
    // Each surrogate pair before `pos` takes two UTF-16 units but only one UTF-32 unit. Break positions
    // never fall inside a surrogate pair
    const auto n = ranges::lower_bound(supplementaries_, pos) - supplementaries_.begin();
    return pos - n;
  }
}

// `Iterator::BreakIteratorCloser` --------------------------------------------------------------------------

template<typename C> requires IsChar<C>
void
Iterator<C>::BreakIteratorCloser::operator()(UBreakIterator* val) const noexcept {
  ubrk_close(val);
}

// Template instantiations ----------------------------------------------------------------------------------

template struct Iterator<char>;
template struct Iterator<char32>;

} // namespace rocket::unicode

// EOF
