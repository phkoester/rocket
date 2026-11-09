/**
 * @file split.h
 *
 * An iterator for splitting strings into tokens.
 */

#include "rocket/assert.h"
#include "rocket/type-traits.h"

#include <string>
#include <vector>

#pragma once

namespace rocket::str {

// `SplitIterator` ------------------------------------------------------------------------------------------

/**
 * The iterator used by #rocket::str::SplitResult.
 *
 * @tparam C the character type
 */
template<typename C> requires IsChar<C>
struct SplitIterator {
  /// @cond undocumented

  SplitIterator(std::basic_string_view<C> str, size_t pos, std::basic_string_view<C> sep) :
      str_(str), pos_(pos), sep_(sep) {
    ROCKET_CHECK(pos, pos == NPOS || pos <= str.size());
    ROCKET_CHECK(sep, not sep.empty());

    if (pos == NPOS) {
      // The end
      end_ = NPOS;
      return;
    }

    if (pos_ == str_.size()) {
      // Empty token at the end
      end_ = pos_;
    } else {
      // A token
      end_ = str_.find(sep_, pos_);
      if (end_ == NPOS) {
        end_ = str_.size();
      }
    }
  }

  bool operator!=(const SplitIterator& rhs) const { return pos_ != rhs.pos_ || end_ != rhs.end_; }

  SplitIterator&
  operator++() {
    if (end_ == str_.size()) {
      // The end
      pos_ = end_ = NPOS;
      return *this;
    }

    pos_ = end_ + sep_.size();
    if (pos_ == str_.size()) {
      // Empty token at the end
      end_ = pos_;
    } else {
      // A token
      end_ = str_.find(sep_, pos_);
      if (end_ == NPOS) {
        end_ = str_.size();
      }
    }

    return *this;
  }

  std::basic_string_view<C> operator*() const { return str_.substr(pos_, end_ - pos_); }

  /// @endcond

private:

  /// The input string to split.
  std::basic_string_view<C> str_;
  /**
   * The current position in the string.
   *
   * If this is #rocket::NPOS, the iterator is exhausted.
   */
  size_t pos_;
  /**
   * The end of the current token.
   *
   * If #pos_ is #rocket::NPOS, this is also #rocket::NPOS.
   */
  size_t end_;
  /// The separator.
  std::basic_string<C> sep_;
};

// `SplitResult` --------------------------------------------------------------------------------------------

/**
 * An object returned by #split that can be used to iterate over the string-view tokens.
 *
 * @tparam C the character type
 */
template<typename C> requires IsChar<C>
struct SplitResult {
  /// @cond undocumented

  SplitResult(std::basic_string_view<C> str, std::basic_string_view<C> sep) : str_(str), sep_(sep) {}

  [[nodiscard]] SplitIterator<C> begin() const { return SplitIterator<C>(str_, 0, sep_); }

  [[nodiscard]] SplitIterator<C> end() const { return SplitIterator<C>(str_, NPOS, sep_); }

  /// @endcond

private:

  /// The input string to split.
  std::basic_string_view<C> str_;
  /// The separator.
  std::basic_string<C> sep_;
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Splits a string into tokens.
 *
 * No strings are ever allocated, except for the separator, so this is a very efficient way to split a string
 * into tokens.
 *
 * The underlying string of @p str must remain valid for the lifetime of the returned result.
 *
 * @tparam C the character type
 * @param str the string to split
 * @param sep the separator to use
 * @return a result object that can be used to iterate over the tokens
 */
template<typename C> requires IsChar<C>
[[nodiscard]] SplitResult<C>
split(std::basic_string_view<C> str, std::basic_string_view<C> sep) {
  return SplitResult<C>(str, sep);
}

} // namespace rocket::str

// EOF
