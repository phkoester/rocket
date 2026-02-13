/**
 * @file str.h
 *
 * A string library, ready for `char` (UTF-8) and `char32` (UTF-32).
 */

#include "rocket/literal.h"
#include "rocket/type-traits.h"

#include <algorithm>
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

  [[nodiscard]] SplitIterator<C> begin() const { return SplitIterator<C>(str_, 0, sep_); }

  [[nodiscard]] SplitIterator<C> end() const { return SplitIterator<C>(str_, NPOS, sep_); }

  /// @endcond

private:

  /// The input string to split.
  std::basic_string_view<C> str_;
  /// The separator.
  std::basic_string<C> sep_;
};

// #split ---------------------------------------------------------------------------------------------------

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
 * ## Examples
 *
 * To enumerate a list with an Oxford comma:
 *
 * ```
 * using namespace rocket;
 * using namespace std;
 *
 * set<string> s = { "red", "green", "blue" };
 * string str = str::join(s.begin(), s.end(), ", ", " and ", ", and");
 * nio::out.println("The colors are {}.", str); // "The colors are red, green, and blue."
 * ```
 *
 *
 * @param begin the beginning of the range
 * @param end the end of the range
 * @param sep the separator to use between elements
 * @param lastSepIfTwo the last separator to use if there are exactly two elements
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
  const u64 n = std::distance(begin, end);
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
 * Splits a string into lines.
 *
 * No strings are ever allocated, except for the separator, so this is a very efficient way to split a string
 * into lines.
 *
 * The underlying string of @p str must remain valid for the lifetime of the returned vector.
 *
 * @tparam C the character type
 * @param str the string to split
 * @return a vector of lines
 */
template<typename C> requires IsChar<C>
[[nodiscard]] std::vector<std::basic_string_view<C>>
lines(std::basic_string_view<C> str) {
  std::vector<std::basic_string_view<C>> ret;

  // Handle LF and CR/LF
  constexpr auto CR = LiteralString<C, '\r'>();
  constexpr auto LF = LiteralString<C, '\n'>();
  for (auto line : split<C>(str, LF)) {
    if (line.ends_with(CR)) {
      line.remove_suffix(CR.size());
    }
    ret.push_back(line);
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
 * - Line breaks (`"\n"` or `"\r\n"`) are recognized.
 * - No-break spaces (U+00A0) are recognized.
 * - Tabs are replaced by spaces.
 * - Consecutive whitespace is collapsed.
 *
 * @param str the string to split into paragraphs
 * @return a vector of paragraphs. Each paragraph is a vector of words. Each word is a UTF-8 string
 */
[[nodiscard]] std::vector<std::vector<std::string>> paragraphs(std::string_view str);

/**
 * Makes a string view such that it has up to @p count leading occurrencies of @p sub removed.
 *
 * The underlying string of @p str must remain valid for the lifetime of the returned string view.
 *
 * @tparam C the character type
 * @param str a string view
 * @param sub the substring to look for
 * @param count the maximum amount of removals
 * @return a string view
 */
template<typename C> requires IsChar<C>
[[nodiscard]] std::basic_string_view<C>
removeLeading(
    std::basic_string_view<C> str,
    std::basic_string_view<C> sub,
    u64 count = NPOS) {
  if (sub.empty()) {
    return str;
  }

  std::basic_string_view<C> ret(str);
  for (u64 i = 0; i < count; ++i) {
    if (ret.starts_with(sub)) {
      ret.remove_prefix(sub.size());
    } else {
      return ret;
    }
  }
  return ret;
}

/**
 * Makes a string view such that it has up to @p count trailing occurrencies of @p sub removed.
 *
 * The underlying string of @p str must remain valid for the lifetime of the returned string view.
 *
 * @tparam C the character type
 * @param str a string view
 * @param sub the substring to look for
 * @param count the maximum amount of removals
 * @return a string view
 */
template<typename C> requires IsChar<C>
[[nodiscard]] std::basic_string_view<C>
removeTrailing(
    std::basic_string_view<C> str,
    std::basic_string_view<C> sub,
    u64 count = NPOS) {
  if (sub.empty()) {
    return str;
  }

  std::basic_string_view<C> ret(str);
  for (u64 i = 0; i < count; ++i) {
    if (ret.ends_with(sub)) {
      ret.remove_suffix(sub.size());
    } else {
      return ret;
    }
  }
  return ret;
}

/**
 * Makes a string view such that it has a trailing end-of-line, if any, removed.
 *
 * The underlying string of @p str must remain valid for the lifetime of the returned string view.
 *
 * @tparam C the character type
 * @param str a string view
 * @return a string view
 */
template<typename C> requires IsChar<C>
[[nodiscard]] std::basic_string_view<C>
removeTrailingEol(std::basic_string_view<C> str) {
  constexpr auto CRLF = LiteralString<C, '\r', '\n'>();
  if (str.ends_with(CRLF)) {
    str.remove_suffix(CRLF.size());
    return str;
  }
  constexpr auto LF = LiteralString<C, '\n'>();
  if (str.ends_with(LF)) {
    str.remove_suffix(LF.size());
    return str;
  }
  return str;
}

/**
 * Repeats the string @p str @p count times.
 *
 * @tparam C the character type
 * @param str a string
 * @param count the number of repetitions
 * @return a new string
 */
template<typename C> requires IsChar<C>
[[nodiscard]] std::basic_string<C>
repeat(const std::basic_string_view<C> str, u64 count) {
  std::basic_string<C> ret;
  ret.reserve(count * str.size());
  for (u64 i = 0; i < count; ++i) {
    ret.append(str);
  }
  return ret;
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
 * - Line breaks (`"\n"` or `"\r\n"`) are recognized.
 * - No-break spaces (U+00A0) are recognized.
 * - Tabs are replaced by spaces.
 * - Consecutive whitespace is collapsed.
 *
 * @param str the string to wrap
 * @param leftIndent the left indentation
 * @param width the width to wrap to
 * @return a new string, containing the wrapped lines separated by @c '\\n'
 */
[[nodiscard]] std::string wrap(std::string_view str, u64 leftIndent = 0, u64 width = 80);

} // namespace rocket::str

// EOF
