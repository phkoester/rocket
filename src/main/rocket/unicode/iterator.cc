/*
 * iterator.cc
 */

#include "iterator.h"

#include "rocket/numeric.h"

#include <unicodelib.h>
#include <unicodelib_encodings.h>

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

namespace unicodelib = ::unicode;

namespace {

// Local functions ------------------------------------------------------------------------------------------

template<typename C> requires Character<C>
CodePointIterator<C> nextWordBreakProperty(const CodePointIterator<C>&);

template<typename C> requires Character<C>
CodePointIterator<C> previousWordBreakProperty(const CodePointIterator<C>&);

/**
 * This implementation is an iterator-based version of `is_grapheme_boundary` in `unicodelib.h`.
 *
 * It works with both UTF-8 and UTF-32 iterators.
 */
#if 0
bool is_grapheme_boundary(const char32_t *s32, size_t l, size_t i)
#endif
template<typename C> requires Character<C>
bool
graphemeBoundary(const CodePointIterator<C>& it) {
  using namespace unicodelib;

  if (it.input().empty())
    return false;

  //---------------------------------------------------------------------------
  // Break at the start and end of text, unless the text empty.
  //---------------------------------------------------------------------------

  // GB1: sot ÷
  if (it.begin())
    return true;

  // GB2: ÷ eot
  if (it.end())
    return true;

#if 0
  const auto lp = _grapheme_break_properties::get_value(s32[i - 1]);
  const auto rp = _grapheme_break_properties::get_value(s32[i]);
#endif

  const auto lp = _grapheme_break_properties::get_value(*(it - 1));
  const auto rp = _grapheme_break_properties::get_value(*it);

  //---------------------------------------------------------------------------
  // Do not break between a CR and LF. Otherwise, break before and after
  // controls.
  //---------------------------------------------------------------------------

  // GB3: CR × LF
  if ((lp == GraphemeBreak::CR) && (rp == GraphemeBreak::LF))
    return false;

  // GB4: (Control|CR|LF) ÷
  if ((lp == GraphemeBreak::Control || lp == GraphemeBreak::CR || lp == GraphemeBreak::LF))
    return true;

  // GB5: ÷ (Control|CR|LF)
  if ((rp == GraphemeBreak::Control || rp == GraphemeBreak::CR || rp == GraphemeBreak::LF))
    return true;

  //---------------------------------------------------------------------------
  // Do not break Hangul syllable sequences.
  //---------------------------------------------------------------------------

  // GB6: L × (L|V|LV|LVT)
  if ((lp == GraphemeBreak::L) &&
      (rp == GraphemeBreak::L || rp == GraphemeBreak::V ||
       rp == GraphemeBreak::LV || rp == GraphemeBreak::LVT))
    return false;

  // GB7: (LV|V) × (V|T)
  if ((lp == GraphemeBreak::LV || lp == GraphemeBreak::V) &&
      (rp == GraphemeBreak::V || rp == GraphemeBreak::T))
    return false;

  // GB8: (LVT|T) × T
  if ((lp == GraphemeBreak::LVT || lp == GraphemeBreak::T) && (rp == GraphemeBreak::T))
    return false;

  //---------------------------------------------------------------------------
  // Do not break before extending characters or ZWJ.
  //---------------------------------------------------------------------------

  // GB9: × Extend
  if (rp == GraphemeBreak::Extend || rp == GraphemeBreak::ZWJ)
    return false;

  //---------------------------------------------------------------------------
  // The GB9a and GB9b rules only apply to extended grapheme clusters:
  // Do not break before SpacingMakrs, or after Prepend characters.
  //---------------------------------------------------------------------------

  // GB9a: × SpacingMark
  if (rp == GraphemeBreak::SpacingMark)
    return false;

  // GB9b: Prepend ×
  if (lp == GraphemeBreak::Prepend)
    return false;

  //---------------------------------------------------------------------------
  // The GB9c rule only applies to extended grapheme clusters: Do not break
  // within certain combinations with Indic_Conjunct_Break (InCB)=Linker.
  //---------------------------------------------------------------------------

#if 0
  // GB9c: \p{InCB=Consonant} [ \p{InCB=Extend} \p{InCB=Linker} ]* \p{InCB=Linker} [ \p{InCB=Extend} \p{InCB=Linker} ]* × \p{InCB=Consonant}
  {
    if (i < l && is_indic_conjunct_break_consonant(s32[i])) {
      auto ok = false;
      auto pos = static_cast<int>(i) - 1;
      while (pos >= 0) {
        auto cp = s32[pos];
        if (is_indic_conjunct_break_linker(cp)) {
          ok = true;
        } else if (is_indic_conjunct_break_extend(cp)) {
        } else {
          break;
        }
        pos--;
      }
      if (ok && pos >= 0 && is_indic_conjunct_break_consonant(s32[pos])) {
        return false;
      }
    }
  }
#endif

  // GB9c: \p{InCB=Consonant} [ \p{InCB=Extend} \p{InCB=Linker} ]* \p{InCB=Linker} [ \p{InCB=Extend} \p{InCB=Linker} ]* × \p{InCB=Consonant}
  {
    if (is_indic_conjunct_break_consonant(*it)) {
      auto ok = false;
      auto posIt = it - 1;
      bool negative = false;
      while (not negative) {
        auto cp = *posIt;
        if (is_indic_conjunct_break_linker(cp))
          ok = true;
        else if (is_indic_conjunct_break_extend(cp))
          ;
        else
          break;
        if (not posIt.decrement())
          negative = true;
      }
      if (ok && not negative && is_indic_conjunct_break_consonant(*posIt))
        return false;
    }
  }

  //---------------------------------------------------------------------------
  // Do not break within emoji modifier sequences or emoji zwj sewuences.
  //---------------------------------------------------------------------------

#if 0
  // GB11: \p{Extended_Pictographic} Extend* ZWJ x \p{Extended_Pictographic}
  {
    auto rpEmoji = _emoji_properties::get_value(s32[i]);

    if (lp == GraphemeBreak::ZWJ && rpEmoji == Emoji::Extended_Pictographic) {
      auto pos = static_cast<int>(i) - 2;
      while (pos >= 0 && _grapheme_break_properties::get_value(s32[pos]) == GraphemeBreak::Extend) {
        pos--;
      }
      if (pos >= 0) {
        auto lpEmoji = _emoji_properties::get_value(s32[pos]);
        if (lpEmoji == Emoji::Extended_Pictographic) {
          return false;
        }
      }
    }
  }
#endif

  // GB11: \p{Extended_Pictographic} Extend* ZWJ x \p{Extended_Pictographic}
  {
    auto rpEmoji = _emoji_properties::get_value(*it);

    if (lp == GraphemeBreak::ZWJ && rpEmoji == Emoji::Extended_Pictographic) {
      auto posIt(it);
      bool negative = false;
      if (not posIt.decrement(2))
        negative = true;
      while (not negative && _grapheme_break_properties::get_value(*posIt) == GraphemeBreak::Extend) {
        if (not posIt.decrement())
          negative = true;
      }
      if (not negative) {
        auto lpEmoji = _emoji_properties::get_value(*posIt);
        if (lpEmoji == Emoji::Extended_Pictographic) {
          return false;
        }
      }
    }
  }

  //---------------------------------------------------------------------------
  // Do not break within emoji flag sequences. That is, do not break between
  // regional indicator (RI) symbols if there is an odd number of RI
  // characters before the break point.
  //---------------------------------------------------------------------------

#if 0
  // GB12: ^ (RI RI)* RI x RI
  // GB13: [^RI] (RI RI)* RI x RI
  if (lp == GraphemeBreak::Regional_Indicator && rp == GraphemeBreak::Regional_Indicator) {
    auto pos = static_cast<int>(i) - 2;
    while (pos >= 1 &&
           _grapheme_break_properties::get_value(s32[pos]) == GraphemeBreak::Regional_Indicator &&
           _grapheme_break_properties::get_value(s32[pos - 1]) == GraphemeBreak::Regional_Indicator) {
      pos -= 2;
    }
    if (pos < 0) {
      return false;
    }
    if (_grapheme_break_properties::get_value(s32[pos]) != GraphemeBreak::Regional_Indicator) {
      return false;
    }
  }
#endif

  // GB12: ^ (RI RI)* RI x RI
  // GB13: [^RI] (RI RI)* RI x RI
  if (lp == GraphemeBreak::Regional_Indicator && rp == GraphemeBreak::Regional_Indicator) {
    auto posIt(it);
    bool negative = false;
    if (not posIt.decrement(2))
      negative = true;
    while (not negative &&
           not posIt.begin() &&
           _grapheme_break_properties::get_value(*posIt) == GraphemeBreak::Regional_Indicator &&
           _grapheme_break_properties::get_value(*(posIt - 1)) == GraphemeBreak::Regional_Indicator) {
      if (not posIt.decrement(2))
        negative = true;
    }
    if (negative)
      return false;
    if (_grapheme_break_properties::get_value(*posIt) != GraphemeBreak::Regional_Indicator)
      return false;
  }

  //---------------------------------------------------------------------------
  // Otherwise, break everywhere.
  //---------------------------------------------------------------------------

  // GB999: Any ÷ Any
  return true;
}

#if 0
size_t next_word_break_property_position(const char32_t *s32, size_t l, size_t i)
#endif
template<typename C> requires Character<C>
CodePointIterator<C>
nextWordBreakProperty(const CodePointIterator<C>& it) {
  using namespace unicodelib;

#if 0
  auto prop = WordBreak::Unassigned;
  auto pos = i + 1;
  while (pos < l) {
    prop = _word_break_properties::get_value(s32[pos]);
    if (prop != WordBreak::Extend && prop != WordBreak::Format &&
        prop != WordBreak::ZWJ) {
      break;
    }
    pos++;
  }
  return pos;
#endif

  if (it.end())
    return it;

  auto prop = WordBreak::Unassigned;
  auto posIt = it + 1;
  while (not posIt.end()) {
    prop = _word_break_properties::get_value(*posIt);
    if (prop != WordBreak::Extend && prop != WordBreak::Format && prop != WordBreak::ZWJ)
      break;
    ++posIt;
  }
  return posIt;
}

#if 0
int previous_word_break_property_position(const char32_t *s32, size_t i)
#endif
template<typename C> requires Character<C>
CodePointIterator<C> previousWordBreakProperty(const CodePointIterator<C>& it) {
  using namespace unicodelib;

#if 0
  auto prop = WordBreak::Unassigned;
  auto pos = static_cast<int>(i) - 1;
  while (pos >= 0) {
    prop = _word_break_properties::get_value(s32[pos]);
    if (prop != WordBreak::Extend && prop != WordBreak::Format &&
        prop != WordBreak::ZWJ) {
      break;
    }
    pos--;
  }
  return pos;
#endif

  if (it.begin())
    return CodePointIterator<C>(it.input(), it.input().size()); // End

  auto prop = WordBreak::Unassigned;
  auto posIt = it - 1;
  while (true) {
    prop = _word_break_properties::get_value(*posIt);
    if (prop != WordBreak::Extend && prop != WordBreak::Format && prop != WordBreak::ZWJ)
      break;
    if (posIt.begin())
      return CodePointIterator<C>(it.input(), it.input().size()); // End
    posIt--;
  }
  return posIt;
}

/**
 * This implementation is an iterator-based version of `is_word_boundary` in `unicodelib.h`.
 *
 * It works with both UTF-8 and UTF-32 iterators.
 */
#if 0
bool is_word_boundary(const char32_t *s32, size_t l, size_t i)
#endif
template<typename C> requires Character<C>
bool
wordBoundary(const CodePointIterator<C>& it) {
  using namespace unicodelib;

  if (it.input().empty())
    return false;

  //---------------------------------------------------------------------------
  // Break at the start and end of text, unless the text empty.
  //---------------------------------------------------------------------------

  // WB1: sot ÷
  if (it.begin())
    return true;

  // WB2: ÷ eot
  if (it.end())
    return true;

#if 0
  auto lp = _word_break_properties::get_value(s32[i - 1]);
  auto rp = _word_break_properties::get_value(s32[i]);
#endif

  auto lp = _word_break_properties::get_value(*(it - 1));
  const auto rp = _word_break_properties::get_value(*it);

  //---------------------------------------------------------------------------
  // Do not break within CRLF
  //---------------------------------------------------------------------------

  // WB3: CR × LF
  if ((lp == WordBreak::CR) && (rp == WordBreak::LF))
    return false;

  //---------------------------------------------------------------------------
  // Otherwise break before and after Newlines (including CR and LF)
  //---------------------------------------------------------------------------

  // WB3a: (Newline|CR|LF) ÷
  if ((lp == WordBreak::Newline || lp == WordBreak::CR || lp == WordBreak::LF))
    return true;

  // WB3b: ÷ (Newline|CR|LF)
  if ((rp == WordBreak::Newline || rp == WordBreak::CR || rp == WordBreak::LF))
    return true;

  //---------------------------------------------------------------------------
  // Do not break within emoji zwj sequences.
  //---------------------------------------------------------------------------

  // WB3c: ZWJ x \p{Extended_Pictographic}
  {
    auto rpEmoji = _emoji_properties::get_value(*it);

    if (lp == WordBreak::ZWJ && rpEmoji == Emoji::Extended_Pictographic)
      return false;
  }

  //---------------------------------------------------------------------------
  // Keep horizontal whitespace together.
  //---------------------------------------------------------------------------

  // WB3d: WSegSpace x WSegSpace
  if (lp == WordBreak::WSegSpace && rp == WordBreak::WSegSpace)
    return false;

  //---------------------------------------------------------------------------
  // Ignore Format and Extend characters, except after sot, CR, LF, and
  // Newline. (See Section 6.2, Replacing Ignore Rules.) This also has the
  // effect of: Any × (Format | Extend | ZWJ)
  //---------------------------------------------------------------------------

  // WB4: X (Extend|Format|ZWJ)* → X
  if ((rp == WordBreak::Extend || rp == WordBreak::Format || rp == WordBreak::ZWJ))
    return false;

#if 0
  // Find left property
  lp = WordBreak::Unassigned;
  auto lpos = previous_word_break_property_position(s32, i);
  if (lpos >= 0) {
    lp = _word_break_properties::get_value(s32[lpos]);
  }
#endif

  // Find left property
  lp = WordBreak::Unassigned;
  auto lIt = previousWordBreakProperty(it);
  if (not lIt.end())
    lp = _word_break_properties::get_value(*lIt);

  //---------------------------------------------------------------------------
  // Do not break between most letters.
  //---------------------------------------------------------------------------

  // WB5: AHLetter × AHLetter
  if (AHLetter(lp) && AHLetter(rp)) {
    return false;
  }

  //---------------------------------------------------------------------------
  // Do not break across certain punctuation.
  //---------------------------------------------------------------------------

#if 0
  auto rp1 = WordBreak::Unassigned;
  auto rpos = next_word_break_property_position(s32, l, i);
  if (rpos < l) {
    rp1 = _word_break_properties::get_value(s32[rpos]);
  }
#endif

  auto rp1 = WordBreak::Unassigned;
  auto rIt = nextWordBreakProperty(it);
  if (not rIt.end())
    rp1 = _word_break_properties::get_value(*rIt);

  // WB6: AHLetter × (MidLetter | MidNumLetQ) AHLetter
  if ((AHLetter(lp)) && ((rp == WordBreak::MidLetter || MidNumLetQ(rp)) && AHLetter(rp1)))
    return false;

#if 0
  auto lp1 = WordBreak::Unassigned;
  lpos = previous_word_break_property_position(s32, lpos);
  if (lpos >= 0) {
    lp1 = _word_break_properties::get_value(s32[lpos]);
  }
#endif

  auto lp1 = WordBreak::Unassigned;
  lIt = previousWordBreakProperty(lIt);
  if (not lIt.end())
    lp1 = _word_break_properties::get_value(*lIt);

  // WB7: AHLetter (MidLetter | MidNumLetQ) × AHLetter
  if ((AHLetter(lp1) && (lp == WordBreak::MidLetter || MidNumLetQ(lp))) && (AHLetter(rp)))
    return false;

  // WB7a: Hebrew_Letter × Single_Quote
  if ((lp == WordBreak::Hebrew_Letter) && (rp == WordBreak::Single_Quote))
    return false;

  // WB7b: Hebrew_Letter × Double_Quote Hebrew_Letter
  if ((lp == WordBreak::Hebrew_Letter) &&
      (rp == WordBreak::Double_Quote && rp1 == WordBreak::Hebrew_Letter))
    return false;

  // WB7c: Hebrew_Letter Double_Quote × Hebrew_Letter
  if ((lp1 == WordBreak::Hebrew_Letter && lp == WordBreak::Double_Quote) &&
      (rp == WordBreak::Hebrew_Letter))
    return false;

  //---------------------------------------------------------------------------
  // Do not break within sequences of digits, or digits adjacent to letters
  // ("3a", or "A3").
  //---------------------------------------------------------------------------

  // WB8: Numeric × Numeric
  if ((lp == WordBreak::Numeric) && (rp == WordBreak::Numeric))
    return false;

  // WB9: AHLetter × Numeric
  if ((AHLetter(lp)) && (rp == WordBreak::Numeric))
    return false;

  // WB10: Numeric × AHLetter
  if ((lp == WordBreak::Numeric) && (AHLetter(rp)))
    return false;

  //---------------------------------------------------------------------------
  // Do not break within sequences, such as "3.2" or "3,456.789"
  //---------------------------------------------------------------------------

  // WB11: Numeric (MidNum | MidNumLetQ) × Numeric
  if ((lp1 == WordBreak::Numeric && (lp == WordBreak::MidNum || MidNumLetQ(lp))) &&
      (rp == WordBreak::Numeric))
    return false;

  // WB12: Numeric × (MidNum | MidNumLetQ) Numeric
  if ((lp == WordBreak::Numeric) &&
      ((rp == WordBreak::MidNum || MidNumLetQ(rp)) && rp1 == WordBreak::Numeric))
    return false;

  //---------------------------------------------------------------------------
  // Do not break between Katakana.
  //---------------------------------------------------------------------------

  // WB13: Katakana × Katakana
  if ((lp == WordBreak::Katakana) && (rp == WordBreak::Katakana))
    return false;

  //---------------------------------------------------------------------------
  // Do not break from extenders.
  //---------------------------------------------------------------------------

  // WB13a: (AHLetter | Numeric | Katakana | ExtendNumLet) × ExtendNumLet
  if ((AHLetter(lp) || lp == WordBreak::Katakana || lp == WordBreak::Numeric ||
       lp == WordBreak::Katakana || lp == WordBreak::ExtendNumLet) &&
      (rp == WordBreak::ExtendNumLet))
    return false;

  // WB13b: ExtendNumLet × (AHLetter | Numeric | Katakana)
  if ((lp == WordBreak::ExtendNumLet) &&
      (AHLetter(rp) || rp == WordBreak::Numeric || rp == WordBreak::Katakana))
    return false;

  //---------------------------------------------------------------------------
  // Do not break within emoji flag sequences. That is, do not break between
  // regional indicator (RI) symbols if there is an odd number of RI
  // characters before the break point.
  //---------------------------------------------------------------------------

#if 0
  // WB15: ^ (RI RI)* RI x RI
  // WB16: [^RI] (RI RI)* RI x RI
  {
    if (lp == WordBreak::Regional_Indicator &&
        rp == WordBreak::Regional_Indicator) {
      lpos = previous_word_break_property_position(s32, i);

      while (true) {
        lpos = previous_word_break_property_position(s32, lpos);
        if (lpos < 0 || _word_break_properties::get_value(s32[lpos]) !=
                            WordBreak::Regional_Indicator) {
          return false;
        }

        lpos = previous_word_break_property_position(s32, lpos);
        if (lpos < 0 || _word_break_properties::get_value(s32[lpos]) !=
                            WordBreak::Regional_Indicator) {
          break;
        }
      }
    }
  }
#endif

  // WB15: ^ (RI RI)* RI x RI
  // WB16: [^RI] (RI RI)* RI x RI
  {
    if (lp == WordBreak::Regional_Indicator && rp == WordBreak::Regional_Indicator) {
      lIt = previousWordBreakProperty(it);

      while (true) {
        lIt = previousWordBreakProperty(lIt);
        if (lIt.end() || _word_break_properties::get_value(*lIt) != WordBreak::Regional_Indicator)
          return false;

        lIt = previousWordBreakProperty(lIt);
        if (lIt.end() || _word_break_properties::get_value(*lIt) != WordBreak::Regional_Indicator)
          break;
      }
    }
  }

  //---------------------------------------------------------------------------
  // Otherwise, break everywhere.
  //---------------------------------------------------------------------------

  // WB14: Any ÷ Any
  return true;
}

} // namespace

namespace rocket::unicode {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

thread_local CodePoint threadLocalCp;
thread_local Grapheme threadLocalGr;

} // namespace internal

// `CodePointIterator` --------------------------------------------------------------------------------------

// `CodePointIterator<char>` ................................................................................

CodePointIterator<char>::CodePointIterator(string_view input, size_t position) :
    input_(input),
    size_(input.size()),
    cpPos_(position == 0 ? 0 : NPOS) {
  go(position);
}

CodePointIterator<char>::operator std::string_view () const {
  ROCKET_EXPECT(not end(), "{}", str::message::iteratorOutOfBounds(*this, pos_));
  return input_.substr(pos_, cpSize_);
}

const CodePoint&
CodePointIterator<char>::operator*() const {
  ROCKET_EXPECT(not end(), "{}", str::message::iteratorOutOfBounds(*this, pos_));
  unicodelib::utf8::decode_codepoint(
      &input_[pos_], cpSize_, reinterpret_cast<char32_t&>(internal::threadLocalCp));
  return internal::threadLocalCp;
}

const CodePoint*
CodePointIterator<char>::operator->() const {
  ROCKET_EXPECT(not end(), "{}", str::message::iteratorOutOfBounds(*this, pos_));
  unicodelib::utf8::decode_codepoint(
      &input_[pos_], cpSize_, reinterpret_cast<char32_t&>(internal::threadLocalCp));
  return &internal::threadLocalCp;
}

const CodePoint&
CodePointIterator<char>::operator[](difference_type index) const {
  auto it(*this);
  it += index;
  *it;
  return internal::threadLocalCp;
}

CodePointIterator<char>&
CodePointIterator<char>::operator++() {
  ROCKET_EXPECT(not end(), "{}", str::message::iteratorAt(*this, pos_, "cannot increment"));
  go(pos_ + cpSize_);
  if (cpPos_ != NPOS)
    ++cpPos_;
  return *this;
}

CodePointIterator<char>
CodePointIterator<char>::operator++(int) {
  auto ret(*this);
  operator++();
  return ret;
}

CodePointIterator<char>&
CodePointIterator<char>::operator--() {
  ROCKET_EXPECT(not begin(), "{}", str::message::iteratorAt(*this, pos_, "cannot decrement"));
  size_t newPos = pos_ - 1;
  // Skip continuation bytes
  while (utf8::continuationByte(input_[newPos])) {
    ROCKET_EXPECT(newPos != 0, "{}", str::message::iteratorAt(*this, 0, "does not point to UTF-8 code-point boundary"));
    --newPos;
  }
  go(newPos);
  if (cpPos_ != NPOS)
    --cpPos_;
  return *this;
}

CodePointIterator<char>
CodePointIterator<char>::operator--(int) {
  auto ret(*this);
  operator--();
  return ret;
}

CodePointIterator<char>&
CodePointIterator<char>::operator+=(CodePointIterator<char>::difference_type rhs) {
  if (rhs < 0)
    return operator-=(-rhs);
  else if (rhs  > 0) {
    for (difference_type i = 0; i < rhs; ++i)
      operator++();
  }
  return *this;
}

CodePointIterator<char>&
CodePointIterator<char>::operator-=(CodePointIterator<char>::difference_type rhs) {
  if (rhs < 0)
    return operator+=(-rhs);
  else if (rhs > 0) {
    for (difference_type i = 0; i < rhs; ++i)
      operator--();
  }
  return *this;
}

size_t
CodePointIterator<char>::codePointPosition() const {
  if (cpPos_ == NPOS) {
    auto it = CodePointIterator<char>(input_);
    while (it.pos_ != pos_)
      ++it;
    // Copy known code-point position
    cpPos_ = it.cpPos_;
  }
  return cpPos_;
}

uint8_t
CodePointIterator<char>::codePointSize() const {
  ROCKET_EXPECT(not end(), "{}", str::message::iteratorOutOfBounds(*this, pos_));
  return cpSize_;
}

bool
CodePointIterator<char>::decrement(difference_type n) {
  if (n < 0)
    return increment(-n);
  else if (n > 0) {
    auto it(*this);
    for (difference_type i = 0; i < n; ++i) {
      if (not it.begin())
        --it;
      else
        return false;
    }
    *this = it;
  }
  return true;
}

void
CodePointIterator<char>::go(size_t newPos) {
  // NOTE: `cpPos_` may not be used inside this function

  // Check position
  ROCKET_EXPECT(newPos <= size_, "{}", str::message::iteratorOutOfBounds(*this, newPos));
  pos_ = newPos;
  if (end())
    return;

  // Check code-point boundary
  cpSize_ = utf8::codePointSize(input_[pos_]);
  ROCKET_EXPECT(cpSize_ != 0, "{}", str::message::iteratorAt(*this, pos_, "does not point to UTF-8 code-point boundary"));

  // Multi-byte sequence?
  if (cpSize_ > 1) {
    // Check string size
    ROCKET_EXPECT(pos_ + cpSize_ <= size_, "{}", str::message::iteratorAt(*this, pos_, "does not point to a complete UTF-8 byte sequence"));

    // Check continuation bytes
    for (size_t i = 1; i < cpSize_; ++i)
      ROCKET_EXPECT(utf8::continuationByte(input_[pos_ + i]), "{}", str::message::iteratorAt(*this, pos_, "does not point to a valid UTF-8 byte sequence"));
  }
}

bool
CodePointIterator<char>::graphemeBoundary() const {
  return ::graphemeBoundary(*this);
}

bool
CodePointIterator<char>::increment(difference_type n) {
  if (n < 0)
    return decrement(-n);
  else if (n > 0) {
    auto it(*this);
    for (difference_type i = 0; i < n; ++i) {
      if (not it.end())
        ++it;
      else
        return false;
    }
    *this = it;
  }
  return true;
}

bool
CodePointIterator<char>::wordBoundary() const {
  return ::wordBoundary(*this);
}

CodePointIterator<char>
operator+(const CodePointIterator<char>& lhs, CodePointIterator<char>::difference_type rhs) {
  auto ret(lhs);
  ret += rhs;
  return ret;
}

CodePointIterator<char>
operator+(CodePointIterator<char>::difference_type lhs, const CodePointIterator<char>& rhs) {
  auto ret(rhs);
  ret += lhs;
  return ret;
}

CodePointIterator<char>
operator-(const CodePointIterator<char>& lhs, CodePointIterator<char>::difference_type rhs) {
  auto ret(lhs);
  ret -= rhs;
  return ret;
}

CodePointIterator<char>::difference_type
operator-(const CodePointIterator<char>& lhs, const CodePointIterator<char>& rhs) {
  return lhs.codePointPosition() - rhs.codePointPosition();
}

// `CodePointIterator<char32_t>` ............................................................................

CodePointIterator<char32_t>::CodePointIterator(u32string_view input, size_t position) :
    input_(input),
    size_(input.size()) {
  go(position);
}

CodePointIterator<char32_t>::operator std::u32string_view() const {
  ROCKET_EXPECT(not end(), "{}", str::message::iteratorOutOfBounds(*this, pos_));
  return input_.substr(pos_, 1);
}

const CodePoint&
CodePointIterator<char32_t>::operator*() const {
  ROCKET_EXPECT(not end(), "{}", str::message::iteratorOutOfBounds(*this, pos_));
  return reinterpret_cast<const CodePoint&>(input_[pos_]);
}

const CodePoint*
CodePointIterator<char32_t>::operator->() const {
  ROCKET_EXPECT(not end(), "{}", str::message::iteratorOutOfBounds(*this, pos_));
  return reinterpret_cast<const CodePoint*>(&input_[pos_]);
}

const CodePoint&
CodePointIterator<char32_t>::operator[](difference_type index) const {
  auto ourIndex = tryAdd<size_t>(pos_, index);
  ROCKET_CHECK(index, ourIndex && *ourIndex < size_, "{}", str::message::iteratorOutOfBounds(*this, pos_ + index));
  return reinterpret_cast<const CodePoint&>(input_[*ourIndex]);
}

CodePointIterator<char32_t>&
CodePointIterator<char32_t>::operator++() {
  ROCKET_EXPECT(not end(), "{}", str::message::iteratorAt(*this, pos_, "cannot increment"));
  go(pos_ + 1);
  return *this;
}

CodePointIterator<char32_t>
CodePointIterator<char32_t>::operator++(int) {
  auto ret(*this);
  operator++();
  return ret;
}

CodePointIterator<char32_t>&
CodePointIterator<char32_t>::operator--() {
  ROCKET_EXPECT(not begin(), "{}", str::message::iteratorAt(*this, pos_, "cannot decrement"));
  go(pos_ - 1);
  return *this;
}

CodePointIterator<char32_t>
CodePointIterator<char32_t>::operator--(int) {
  auto ret(*this);
  operator--();
  return ret;
}

CodePointIterator<char32_t>&
CodePointIterator<char32_t>::operator+=(difference_type rhs) {
  if (rhs < 0) {
    return operator-=(-rhs);
  } else if (rhs > 0) {
    go(add<size_t>(pos_, rhs));
  }
  return *this;
}

CodePointIterator<char32_t>&
CodePointIterator<char32_t>::operator-=(difference_type rhs) {
  if (rhs < 0) {
    return operator+=(-rhs);
  } else if (rhs > 0) {
    go(sub<size_t>(pos_, rhs));
  }
  return *this;
}

bool
CodePointIterator<char32_t>::decrement(difference_type n) {
  if (n < 0) {
    return increment(-n);
  } else if (n > 0) {
    size_t newPos = pos_ - n;
    if (newPos >= pos_) {
      // `size_t` overflow
      return false;
    }
    pos_ = newPos;
  }
  return true;
}

void
CodePointIterator<char32_t>::go(size_t newPos) {
  // Check position
  ROCKET_EXPECT(newPos <= size_, "{}", str::message::iteratorOutOfBounds(*this, newPos));
  pos_ = newPos;
}

bool
CodePointIterator<char32_t>::graphemeBoundary() const {
  return ::graphemeBoundary(*this);
}

bool
CodePointIterator<char32_t>::increment(difference_type n) {
  if (n < 0) {
    return decrement(-n);
  } else if (n > 0) {
    size_t newPos = pos_ + n;
    if (newPos <= pos_ || newPos > size_) {
      // `size_t` overflow or out of bounds
      return false;
    }
    pos_ = newPos;
  }
  return true;
}

bool
CodePointIterator<char32_t>::wordBoundary() const {
  return ::wordBoundary(*this);
}

CodePointIterator<char32_t>
operator+(const CodePointIterator<char32_t>& lhs, CodePointIterator<char32_t>::difference_type rhs) {
  auto ret(lhs);
  ret += rhs;
  return ret;
}

CodePointIterator<char32_t>
operator+(CodePointIterator<char32_t>::difference_type lhs, const CodePointIterator<char32_t>& rhs) {
  auto ret(rhs);
  ret += lhs;
  return ret;
}

CodePointIterator<char32_t>
operator-(const CodePointIterator<char32_t>& lhs, CodePointIterator<char32_t>::difference_type rhs) {
  auto ret(lhs);
  ret -= rhs;
  return ret;
}

CodePointIterator<char>::difference_type
operator-(const CodePointIterator<char32_t>& lhs, const CodePointIterator<char32_t>& rhs) {
  return lhs.codePointPosition() - rhs.codePointPosition();
}

} // namespace rocket::unicode

// EOF
