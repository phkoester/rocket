/**
 * @file Iterator.h
 *
 * A Unicode-aware text iterator.
 */

#pragma once

#include "rocket/UnorderedBimap.h"

#include <unicode/brkiter.h>

namespace rocket::unicode {

// `IteratorType` -------------------------------------------------------------------------------------------

/// Iterator types.
enum class IteratorType {
  Char, ///< Iterate over characters, i.e. grapheme clusters
  Line, ///< Iterate over lines
  Sentence, ///< Iterate over sentences
  Title, ///< Iterate over title boundaries
  Word ///< Iterate over words
};

// `Iterator` -----------------------------------------------------------------------------------------------

/**
 * A Unicode-aware text iterator.
 *
 * @tparam C the character type
 */
template<typename C> requires Character<C>
struct Iterator {
  /**
   * @ctor
   *
   * @param type the type of iterator
   * @param text the text to iterate over. It must remain valid for the lifetime of the iterator
   * @throw #rocket::InvalidArgument if @p text is invalid
   * @throw #rocket::InvalidState if something goes wrong
   */
  Iterator(IteratorType type, std::basic_string_view<C> text) : Iterator(type, text, std::locale()) {}

  /**
   * @ctor
   *
   * @param type the type of iterator
   * @param text the text to iterate over. It must remain valid for the lifetime of the iterator
   * @param loc the locale to use
   * @throw #rocket::InvalidArgument if @p text or @p loc are invalid
   * @throw #rocket::InvalidState if something goes wrong
   */
  Iterator(IteratorType type, std::basic_string_view<C> text, const std::locale& loc);

  size_t current() const { return usToText_.left.at(iter_->current()); }

  size_t first() const { auto v = iter_->first(); return usToText_.left.at(v); }

  size_t last() const { auto v = iter_->last(); return usToText_.left.at(v); }

  size_t
  next() const {
    auto pos = iter_->next();
    if (pos == icu::BreakIterator::DONE) {
      return NPOS;
    }
    return usToText_.left.at(pos);
  }

  std::basic_string_view<C>
  nextSegment() const {
    auto current = this->current();
    auto next = this->next();
    if (next == NPOS) {
      return std::basic_string_view<C>();
    }
    return text_.substr(current, next - current);
  }

  std::vector<std::basic_string_view<C>>
  nextSegments() const {
    std::vector<std::basic_string_view<C>> ret;
    for (auto seg = nextSegment(); not seg.empty(); seg = nextSegment()) {
      ret.push_back(seg);
    }
    return ret;
  }

  size_t
  previous() const {
    auto pos = iter_->previous();
    if (pos == icu::BreakIterator::DONE) {
      return NPOS;
    }
    return usToText_.left.at(pos);
  }

  std::basic_string_view<C>
  previousSegment() const {
    auto current = this->current();
    auto previous = this->previous();
    if (previous == NPOS) {
      return std::basic_string_view<C>();
    }
    return text_.substr(previous, current - previous);
  }

  std::vector<std::basic_string_view<C>>
  previousSegments() const {
    std::vector<std::basic_string_view<C>> ret;
    for (auto seg = previousSegment(); not seg.empty(); seg = previousSegment()) {
      ret.push_back(seg);
    }
    return ret;
  }

  std::basic_string_view<C> text() const { return text_; }

private:

  std::basic_string_view<C> text_;
  icu::UnicodeString us_;
  UnorderedBimap<size_t, size_t> usToText_;
  std::unique_ptr<icu::BreakIterator> iter_;
};

// Functions ------------------------------------------------------------------------------------------------

template<typename C> requires Character<C>
std::basic_string<C>
concat(const std::vector<std::basic_string_view<C>>& segments, size_t pos = 0, size_t n = NPOS) {
  std::basic_string<C> ret;
  pos = min(pos, segments.size());
  n = min(n, segments.size() - pos);
  for (size_t i = pos, end = pos + n; i < end; ++i) {
    ret.append(segments[i]);
  }
  return ret;
}

} // namespace rocket::unicode

// EOF
