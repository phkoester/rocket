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

// `SplitIterator` ------------------------------------------------------------------------------------------

template<typename C> requires IsChar<C>
struct SplitIterator {
  SplitIterator(std::basic_string_view<C> s, size_t pos, std::basic_string_view<C> sep) :
      s_(s), pos_(pos), sep_(sep) {
    if (pos == NPOS) {
      // The end
      pos_ = s_.size();
      end_ = NPOS;
      return;
    }

    pos_ = std::min(pos_, s_.size());
    if (pos_ == s_.size()) {
      // Empty token at the end
      end_ = pos_;
    } else {
      // A token
      end_ = s_.find(sep_, pos_);
      if (end_ == NPOS) {
        end_ = s_.size();
      }
    }
  }

  bool operator==(const SplitIterator& rhs) const { return pos_ == rhs.pos_ && end_ == rhs.end_; }

  bool operator!=(const SplitIterator& rhs) const { return not operator==(rhs); }

  SplitIterator&
  operator++() {
    if (end_ == s_.size()) {
      // The end
      pos_ = end_;
      end_ = NPOS;
      return *this;
    }

    pos_ = end_ + sep_.size();
    if (pos_ == s_.size()) {
      // Empty token at the end
      end_ = pos_;
    } else {
      // A token
      end_ = s_.find(sep_, pos_);
      if (end_ == NPOS) {
        end_ = s_.size();
      }
    }

    return *this;
  }

  std::basic_string_view<C> operator*() const { return s_.substr(pos_, end_ - pos_); }

private:

  /// The input string to split.
  std::basic_string_view<C> s_;
  size_t pos_; ///< The current position in the string.
  /**
   * The end of the current token.
   *
   * If this is #rocket::NPOS, the iterator is exhausted.
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
  SplitResult(std::basic_string_view<C> s, std::basic_string_view<C> sep) : s_(s), sep_(sep) {}

  SplitIterator<C> begin() const { return SplitIterator<C>(s_, 0, sep_); }

  SplitIterator<C> end() const { return SplitIterator<C>(s_, NPOS, sep_); }

private:

  std::basic_string_view<C> s_;
  std::basic_string<C> sep_;
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Checks if string @p s begins with substring @p sub.
 *
 * @tparam C the character type
 * @param s a string
 * @param sub the substring to look for
 * @return whether @p s begins with substring @p sub
 */
template<typename C> requires IsChar<C>
bool
beginsWith(std::basic_string_view<C> s, std::basic_string_view<C> sub) {
  if (sub.empty())
    return true;
  if (sub.size() > s.size())
    return false;
  return s.substr(0, sub.size()) == sub;
}

/**
 * Makes a new string from @p s that has its first letter capitalized
 *
 * @param s a UTF-8 string
 * @return a new string
 */
std::string capitalize(std::string_view s);

/**
 * Makes a new string from @p s that has its first letter capitalized
 *
 * @param s a UTF-32 string
 * @return a new string
 */
std::u32string capitalize(std::u32string_view s);

/**
 * Checks if string @p s ends with substring @p sub.
 *
 * @tparam C the character type
 * @param s a string
 * @param sub the substring to look for
 * @return whether @p s ends with substring @p sub
 */
template<typename C> requires IsChar<C>
bool
endsWith(std::basic_string_view<C> s, std::basic_string_view<C> sub) {
  if (sub.empty())
    return true;
  if (sub.size() > s.size())
    return false;
  return s.substr(s.size() - sub.size()) == sub;
}

/**
 * Converts a UTF-8 string to lower case, handling Unicode characters correctly.
 *
 * @param s the string to convert
 * @return a new string
 */
[[nodiscard]] std::string lower(std::string_view s);

/**
 * Converts a UTF-32 string to lower case, handling Unicode characters correctly.
 *
 * @param s the string to convert
 * @return a new string
 */
[[nodiscard]] std::u32string lower(std::u32string_view s);

/**
 * Converts a UTF-32 string to lower case, handling Unicode characters correctly.
 *
 * @param s the string to convert
 */
void lowerIn(std::u32string& s);

/**
 * Converts a UTF-32 string to lower case, handling Unicode characters correctly.
 *
 * @param s the string to convert
 */
void lowerIn(std::u32string& s);

/**
 * Splits a string into paragraphs.
 *
 * @param s the string to split. The string must be UTF-8-encoded, using LF (`"\n"`) or CRLF (`"\r\n"`) as
 *     line breaks. Non-breaking spaces (U+00A0) are recognized. Tabs are replaced by spaces. Consecutive
 *     whitespace is collapsed.
 * @return a vector of paragraphs. Each paragraph is a vector of words. Each word is a UTF-8 string.
 */
std::vector<std::vector<std::string>> paragraphs(std::string_view s);

/**
 * Makes a string view such that it has up to @p max leading occurrencies of @p sub removed.
 *
 * This is a function template that works both with UTF-8 and UTF-32 strings.
 *
 * @tparam C the character type
 * @param s a string view
 * @param sub the substring to look for
 * @param max maximum amount of removals
 * @return a string view
 */
template<typename C> requires IsChar<C>
std::basic_string_view<C>
removeLeading(
    std::basic_string_view<C> s,
    std::basic_string_view<C> sub,
    u64 max = std::numeric_limits<u64>::max()) {
  if (sub.empty() || sub.size() > s.size())
    return s;

  std::basic_string_view<C> ret(s);
  for (u64 i = 0; i < max; ++i) {
    std::basic_string_view<C> leading(ret.begin(), sub.size());
    if (leading == sub)
      ret.remove_prefix(sub.size());
    else
      return ret;
  }
  return ret;
}

/**
 * Makes a string view such that it has up to @p max trailing occurrencies of @p sub removed.
 *
 * This is a function template that works both with UTF-8 and UTF-32 strings.
 *
 * @tparam C the character type
 * @param s a string view
 * @param sub the substring to look for
 * @param max maximum amount of removals
 * @return a string view
 */
template<typename C> requires IsChar<C>
std::basic_string_view<C>
removeTrailing(
    std::basic_string_view<C> s,
    std::basic_string_view<C> sub,
    u64 max = std::numeric_limits<u64>::max()) {
  if (sub.empty() || sub.size() > s.size())
    return s;

  std::basic_string_view<C> ret(s);
  for (u64 i = 0; i < max; ++i) {
    auto begin = ret.end() - sub.size();
    std::basic_string_view<C> trailing(begin, ret.end());
    if (trailing == sub)
      ret.remove_suffix(sub.size());
    else
      return ret;
  }
  return ret;
}

/**
 * Repeats the string @p s @p n times.
 *
 * @tparam C the character type
 * @param s a string
 * @param n a number
 * @return a new string
 */
template<typename C> requires IsChar<C>
[[nodiscard]] std::basic_string<C>
repeat(const std::basic_string_view<C> s, u64 n) {
  std::basic_string<C> ret;
  ret.reserve(n * s.size());
  for (u64 i = 0; i < n; ++i)
    ret.append(s);
  return ret;
}

/**
 * Modifies the string @p s such that it has up to @p max occurrencies of @p from replaced by @p to.
 *
 * This is a function template that works both with UTF-8 and UTF-32 strings.
 *
 * @tparam C the character type
 * @param s the string to modify
 * @param from the substring to look for
 * @param to the new substring to replace the old substring
 * @param max maximum amount of replacements
 * @return the number of replacements made
 */
template<typename C> requires IsChar<C>
u64
replaceIn(
    std::basic_string<C>& s,
    std::basic_string_view<C> from,
    std::basic_string_view<C> to,
    u64 max = std::numeric_limits<u64>::max()) {
  u64 pos = 0, ret = 0;
  while ((pos = s.find(from, pos)) != std::string::npos) {
    s.replace(pos, from.size(), to);
    if (++ret >= max)
      break;
    pos += to.size();
  }
  return ret;
}

/**
 * Splits a string into tokens.
 *
 * No strings are ever allocated, so this is a very efficient way to split a string into tokens.
 *
 * @tparam C the character type
 * @param s the string to split
 * @param sep the separator to use
 * @return a result object that can be used to iterate over the string-view tokens
 */
template<typename C> requires IsChar<C>
SplitResult<C>
split(std::basic_string_view<C> s, std::basic_string_view<C> sep) {
  return SplitResult<C>(s, sep);
}

/**
 * Converts a UTF-8 string to upper case, handling Unicode characters correctly.
 *
 * @param s the string to convert
 * @return a new string
 */
[[nodiscard]] std::string upper(std::string_view s);

/**
 * Converts a UTF-32 string to upper case, handling Unicode characters correctly.
 *
 * @param s the string to convert
 * @return a new string
 */
[[nodiscard]] std::u32string upper(std::u32string_view s);

/**
 * Converts a UTF-32 string to upper case, handling Unicode characters correctly.
 *
 * @param s the string to convert
 */
void upperIn(std::u32string& s);

/**
 * Wraps the string @p s to fit the width specified by @p params.
 *
 * - Line breaks (`"\n"`, `"\r\n"`) are recognized.
 * - Non-breaking spaces (U+00A0) are recognized.
 * - Tabs are replaced by spaces.
 * - Consecutive whitespace is collapsed.
 *
 * @param s the string to wrap
 * @param leftIndent the left indentation
 * @param width the width to wrap to
 * @return a new string, containing the wrapped lines separated by `'\n'`.
 */
std::string wrap(std::string_view s, u64 leftIndent = 0, u64 width = 80);

} // namespace rocket::str

// EOF
