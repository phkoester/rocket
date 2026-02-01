/**
 * @file str.h
 *
 * A string library, ready for `char` (UTF-8) and `char32` (UTF-32).
 */

#include "rocket/type-traits.h"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#pragma once

namespace rocket::str {

// #SplitIterator -------------------------------------------------------------------------------------------

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
    if (pos == NPOS) {
      // The end
      end_ = NPOS;
      return;
    }

    pos_ = std::min(pos_, str_.size());
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

// #SplitResult ---------------------------------------------------------------------------------------------

/**
 * An object returned by #split that can be used to iterate over the string-view tokens.
 *
 * @tparam C the character type
 */
template<typename C> requires IsChar<C>
struct SplitResult {
  /// @cond undocumented

  SplitResult(std::basic_string_view<C> str, std::basic_string_view<C> sep) : str_(str), sep_(sep) {}

  SplitIterator<C> begin() const { return SplitIterator<C>(str_, 0, sep_); }

  SplitIterator<C> end() const { return SplitIterator<C>(str_, NPOS, sep_); }

  /// @endcond

private:

  /// The input string to split.
  std::basic_string_view<C> str_;
  /// The separator.
  std::basic_string<C> sep_;
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Makes a new string from @p str that has its first letter capitalized
 *
 * @param str a UTF-8 string
 * @return a new string
 */
std::string capitalize(std::string_view str);

/**
 * Makes a new string from @p str that has its first letter capitalized
 *
 * @param str a UTF-32 string
 * @return a new string
 */
std::u32string capitalize(std::u32string_view str);

/**
 * Joins elements of a range into a string, using a a set of different separators.
 *
 *
 * ## Examples
 *
 * To enumerate a list with an Oxford comma:
 *
 * ```
 * set<string> s = { "red", "green", "blue" };
 * string str = str::join(s.begin(), s.end(), ", ", " and ", ", and");
 * nio::out.println("The colors are {}.", str); // "The colors are red, green, and blue."
 * ```
 *
 *
 * @param begin the beginning of the range
 * @param end the end of the range
 * @param sep the separator to use between elements
 * @param lastSepIfTwo the last separator to use if there are two elements
 * @param lastSepIfMore the last sseparator to use if there are more than two elements
 * @return a new string
 */
template<typename Iterator>
std::string join(
  Iterator begin,
  Iterator end,
  std::string_view sep,
  std::string_view lastSepIfTwo,
  std::string_view lastSepIfMore) {
  std::string ret;

  u64 count = 0;
  u64 n = std::distance(begin, end);
  for (Iterator it = begin; it != end; ++it) {
    if (++count > 1) {
      if (count == n) {
        ret.append(count == 2 ? lastSepIfTwo : lastSepIfMore);
      } else {
        ret.append(sep);
      }
    }
    ret.append(*it);
  }

  return ret;
}

/**
 * Converts a UTF-8 string to lower case, handling Unicode characters correctly.
 *
 * @param str the string to convert
 * @return a new string
 */
[[nodiscard]] std::string lower(std::string_view str);

/**
 * Converts a UTF-32 string to lower case, handling Unicode characters correctly.
 *
 * @param str the string to convert
 * @return a new string
 */
[[nodiscard]] std::u32string lower(std::u32string_view str);

/**
 * Converts a UTF-32 string to lower case, handling Unicode characters correctly.
 *
 * @param str the string to convert
 */
void lowerIn(std::u32string& str);

/**
 * Splits a string into paragraphs.
 *
 * @param str the string to split. The string must be UTF-8-encoded, using LF (`"\n"`) or CRLF (`"\r\n"`) as
 *     line breaks. No-break spaces (U+00A0) are recognized. Tabs are replaced by spaces. Consecutive
 *     whitespace is collapsed
 * @return a vector of paragraphs. Each paragraph is a vector of words. Each word is a UTF-8 string
 */
std::vector<std::vector<std::string>> paragraphs(std::string_view str);

/**
 * Makes a string view such that it has up to @p max leading occurrencies of @p sub removed.
 *
 * This is a function template that works both with UTF-8 and UTF-32 strings.
 *
 * @tparam C the character type
 * @param str a string view
 * @param sub the substring to look for
 * @param max maximum amount of removals
 * @return a string view
 */
template<typename C> requires IsChar<C>
std::basic_string_view<C>
removeLeading(
    std::basic_string_view<C> str,
    std::basic_string_view<C> sub,
    u64 max = std::numeric_limits<u64>::max()) {
  if (sub.empty() || sub.size() > str.size()) {
    return str;
  }

  std::basic_string_view<C> ret(str);
  for (u64 i = 0; i < max; ++i) {
    // MSVC doesn't like the ctor with pointer and size here
    std::basic_string_view<C> leading(ret.begin(), ret.begin() + sub.size());
    if (leading == sub) {
      ret.remove_prefix(sub.size());
    } else {
      return ret;
    }
  }
  return ret;
}

/**
 * Makes a string view such that it has up to @p max trailing occurrencies of @p sub removed.
 *
 * This is a function template that works both with UTF-8 and UTF-32 strings.
 *
 * @tparam C the character type
 * @param str a string view
 * @param sub the substring to look for
 * @param max maximum amount of removals
 * @return a string view
 */
template<typename C> requires IsChar<C>
std::basic_string_view<C>
removeTrailing(
    std::basic_string_view<C> str,
    std::basic_string_view<C> sub,
    u64 max = std::numeric_limits<u64>::max()) {
  if (sub.empty() || sub.size() > str.size()) {
    return str;
  }

  std::basic_string_view<C> ret(str);
  for (u64 i = 0; i < max; ++i) {
    auto begin = ret.end() - sub.size();
    std::basic_string_view<C> trailing(begin, ret.end());
    if (trailing == sub) {
      ret.remove_suffix(sub.size());
    } else {
      return ret;
    }
  }
  return ret;
}

/**
 * Repeats the string @p str @p n times.
 *
 * @tparam C the character type
 * @param str a string
 * @param n a number
 * @return a new string
 */
template<typename C> requires IsChar<C>
[[nodiscard]] std::basic_string<C>
repeat(const std::basic_string_view<C> str, u64 n) {
  std::basic_string<C> ret;
  ret.reserve(n * str.size());
  for (u64 i = 0; i < n; ++i) {
    ret.append(str);
  }
  return ret;
}

/**
 * Splits a string into tokens.
 *
 * No strings are ever allocated, except for the separator, so this is a very efficient way to split a string
 * into tokens.
 *
 * @tparam C the character type
 * @param str the string to split
 * @param sep the separator to use
 * @return a result object that can be used to iterate over the string-view tokens
 */
template<typename C> requires IsChar<C>
SplitResult<C>
split(std::basic_string_view<C> str, std::basic_string_view<C> sep) {
  return SplitResult<C>(str, sep);
}

/**
 * Converts a UTF-8 string to upper case, handling Unicode characters correctly.
 *
 * @param str the string to convert
 * @return a new string
 */
[[nodiscard]] std::string upper(std::string_view str);

/**
 * Converts a UTF-32 string to upper case, handling Unicode characters correctly.
 *
 * @param str the string to convert
 * @return a new string
 */
[[nodiscard]] std::u32string upper(std::u32string_view str);

/**
 * Converts a UTF-32 string to upper case, handling Unicode characters correctly.
 *
 * @param str the string to convert
 */
void upperIn(std::u32string& str);

/**
 * Wraps the string @p str to fit the width specified by @p width.
 *
 * - Line breaks (`"\n"`, `"\r\n"`) are recognized.
 * - No-break spaces (U+00A0) are recognized.
 * - Tabs are replaced by spaces.
 * - Consecutive whitespace is collapsed.
 *
 * @param str the string to wrap
 * @param leftIndent the left indentation
 * @param width the width to wrap to
 * @return a new string, containing the wrapped lines separated by @c '\\n'
 */
std::string wrap(std::string_view str, u64 leftIndent = 0, u64 width = 80);

} // namespace rocket::str

// EOF
