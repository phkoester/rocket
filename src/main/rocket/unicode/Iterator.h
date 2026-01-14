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
  Character, ///< Iterate over characters
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
template<typename C> requires IsChar<C>
struct Iterator {
  /**
   * @ctor
   *
   * @param type the type of iterator
   * @param input the input string to iterate over. It must remain valid for the lifetime of the iterator
   */
  Iterator(IteratorType type, std::basic_string_view<C> input) : Iterator(type, input, std::locale()) {}

  /**
   * @ctor
   *
   * @param type the type of iterator
   * @param input the input string to iterate over. It must remain valid for the lifetime of the iterator
   * @param loc the locale to use
   */
  Iterator(IteratorType type, std::basic_string_view<C> input, const std::locale& loc);

  /**
   * Returns the current position in the input string.
   *
   * @return the current position
   */
  size_t current() const { return usToInput_.left.at(iter_->current()); }

  /**
   * Sets the iterator to the first position in the input string.
   *
   * @return the first position
   */
  size_t first() { auto v = iter_->first(); return usToInput_.left.at(v); }

  /**
   * Returns the input string.
   *
   * @return the input string
   */
  std::basic_string_view<C> input() const { return input_; }

  /**
   * Sets the iterator to the last position in the input string.
   *
   * @return the last position
   */
  size_t last() { auto v = iter_->last(); return usToInput_.left.at(v); }

  /**
   * Advances the iterator to the next position in the input string.
   *
   * @return the next position
   */
  size_t
  next() {
    auto pos = iter_->next();
    if (pos == icu::BreakIterator::DONE) {
      return NPOS;
    }
    return usToInput_.left.at(pos);
  }

  /**
   * Returns the next segment in the input string
   *
   * @return the next segment. If it is empty, the iterator is exhausted
   */
  std::basic_string_view<C>
  nextSegment()  {
    auto current = this->current();
    auto next = this->next();
    if (next == NPOS) {
      return std::basic_string_view<C>();
    }
    return input_.substr(current, next - current);
  }

  /**
   * Returns the next segments in the input string
   *
   * @return the next segments
   */
  std::vector<std::basic_string_view<C>>
  nextSegments() {
    std::vector<std::basic_string_view<C>> ret;
    for (auto seg = nextSegment(); not seg.empty(); seg = nextSegment()) {
      ret.push_back(seg);
    }
    return ret;
  }

  /**
   * Advances the iterator to the previous position in the input string.
   *
   * @return the previous position
   */
  size_t
  previous() {
    auto pos = iter_->previous();
    if (pos == icu::BreakIterator::DONE) {
      return NPOS;
    }
    return usToInput_.left.at(pos);
  }

  /**
   * Returns the previous segment in the input string
   *
   * @return the previous segment. If it is empty, the iterator is exhausted
   */
  std::basic_string_view<C>
  previousSegment() {
    auto current = this->current();
    auto previous = this->previous();
    if (previous == NPOS) {
      return std::basic_string_view<C>();
    }
    return input_.substr(previous, current - previous);
  }

  /**
   * Returns the previous segments in the input string
   *
   * @return the previous segments
   */
  std::vector<std::basic_string_view<C>>
  previousSegments() {
    std::vector<std::basic_string_view<C>> ret;
    for (auto seg = previousSegment(); not seg.empty(); seg = previousSegment()) {
      ret.push_back(seg);
    }
    return ret;
  }

private:

  std::basic_string_view<C> input_;
  icu::UnicodeString us_;
  UnorderedBimap<size_t, size_t> usToInput_;
  std::unique_ptr<icu::BreakIterator> iter_;
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Concatenates a vector of segments into a single string.
 *
 * @param segments the segments to concatenate
 * @param pos the position to start concatenating from
 * @param n the number of segments to concatenate
 * @return the concatenated string
 */
template<typename C> requires IsChar<C>
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
