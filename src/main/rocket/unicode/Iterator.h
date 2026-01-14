/**
 * @file Iterator.h
 *
 * A Unicode-aware string iterator.
 */

#pragma once

#include "rocket/UnorderedBimap.h"

#include <unicode/brkiter.h>

#include <algorithm> // `std::min`

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
 * A Unicode-aware string iterator.
 *
 * @tparam C the character type
 */
template<typename C> requires Character<C>
struct Iterator {
  /**
   * @ctor
   *
   * @param type the type of iterator
   * @param input the input string to iterate over. It must remain valid for the lifetime of the iterator
   * @throw #rocket::InvalidArgument if @p input is invalid
   * @throw #rocket::InvalidState if something goes wrong
   */
  Iterator(IteratorType type, std::basic_string_view<C> input) : Iterator(type, input, std::locale()) {}

  /**
   * @ctor
   *
   * @param type the type of iterator
   * @param input the input string to iterate over. It must remain valid for the lifetime of the iterator
   * @param loc the locale to use
   * @throw #rocket::InvalidArgument if @p input or @p loc are invalid
   * @throw #rocket::InvalidState if something goes wrong
   */
  Iterator(IteratorType type, std::basic_string_view<C> input, const std::locale& loc);

  size_t current() const { return usToInput_.left.at(iter_->current()); }

  size_t first() { auto v = iter_->first(); return usToInput_.left.at(v); }

  size_t last() { auto v = iter_->last(); return usToInput_.left.at(v); }

  size_t
  next() {
    auto pos = iter_->next();
    if (pos == icu::BreakIterator::DONE) {
      return NPOS;
    }
    return usToInput_.left.at(pos);
  }

  std::basic_string_view<C>
  nextSegment()  {
    auto current = this->current();
    auto next = this->next();
    if (next == NPOS) {
      return std::basic_string_view<C>();
    }
    return input_.substr(current, next - current);
  }

  std::vector<std::basic_string_view<C>>
  nextSegments() {
    std::vector<std::basic_string_view<C>> ret;
    for (auto seg = nextSegment(); not seg.empty(); seg = nextSegment()) {
      ret.push_back(seg);
    }
    return ret;
  }

  size_t
  previous() {
    auto pos = iter_->previous();
    if (pos == icu::BreakIterator::DONE) {
      return NPOS;
    }
    return usToInput_.left.at(pos);
  }

  std::basic_string_view<C>
  previousSegment() {
    auto current = this->current();
    auto previous = this->previous();
    if (previous == NPOS) {
      return std::basic_string_view<C>();
    }
    return input_.substr(previous, current - previous);
  }

  std::vector<std::basic_string_view<C>>
  previousSegments() {
    std::vector<std::basic_string_view<C>> ret;
    for (auto seg = previousSegment(); not seg.empty(); seg = previousSegment()) {
      ret.push_back(seg);
    }
    return ret;
  }

  std::basic_string_view<C> input() const { return input_; }

private:

  std::basic_string_view<C> input_;
  icu::UnicodeString us_;
  UnorderedBimap<size_t, size_t> usToInput_;
  std::unique_ptr<icu::BreakIterator> iter_;
};

// Functions ------------------------------------------------------------------------------------------------

template<typename C> requires Character<C>
std::basic_string<C>
concat(const std::vector<std::basic_string_view<C>>& segments, size_t pos = 0, size_t n = NPOS) {
  std::basic_string<C> ret;
  pos = std::min(pos, segments.size());
  n = std::min(n, segments.size() - pos);
  for (size_t i = pos, end = pos + n; i < end; ++i) {
    ret.append(segments[i]);
  }
  return ret;
}

} // namespace rocket::unicode

// EOF
