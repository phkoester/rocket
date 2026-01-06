/**
 * @file str.h
 *
 * A string library, ready for `char` (UTF-8) and `char32_t` (UTF-32).
 */

#include "rocket/TypeTraits.h"

#include <limits>
#include <string>
#include <vector>

#pragma once

namespace rocket::str {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Returns `true` if string @p s begins with substring @p sub.
 *
 * @tparam C the character type
 * @param s a string
 * @param sub the substring to look for
 * @return `true` if @p s begins with substring @p sub
 */
template<typename C> requires Character<C>
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
 * Returns `true` if string @p s ends with substring @p sub.
 *
 * @tparam C the character type
 * @param s a string
 * @param sub the substring to look for
 * @return `true` if @p s ends with substring @p sub
 */
template<typename C> requires Character<C>
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
template<typename C> requires Character<C>
std::basic_string_view<C>
removeLeading(
    std::basic_string_view<C> s,
    std::basic_string_view<C> sub,
    size_t max = std::numeric_limits<size_t>::max()) {
  if (sub.empty() || sub.size() > s.size())
    return s;

  std::basic_string_view<C> ret(s);
  for (size_t i = 0; i < max; ++i) {
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
template<typename C> requires Character<C>
std::basic_string_view<C>
removeTrailing(
    std::basic_string_view<C> s,
    std::basic_string_view<C> sub,
    size_t max = std::numeric_limits<size_t>::max()) {
  if (sub.empty() || sub.size() > s.size())
    return s;

  std::basic_string_view<C> ret(s);
  for (size_t i = 0; i < max; ++i) {
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
template<typename C> requires Character<C>
[[nodiscard]] std::basic_string<C>
repeat(const std::basic_string_view<C> s, size_t n) {
  std::basic_string<C> ret;
  ret.reserve(n * s.size());
  for (size_t i = 0; i < n; ++i)
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
template<typename C> requires Character<C>
size_t
replaceIn(
    std::basic_string<C>& s,
    std::basic_string_view<C> from,
    std::basic_string_view<C> to,
    size_t max = std::numeric_limits<size_t>::max()) {
  size_t pos = 0, ret = 0;
  while ((pos = s.find(from, pos)) != std::string::npos) {
    s.replace(pos, from.size(), to);
    if (++ret >= max)
      break;
    pos += to.size();
  }
  return ret;
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
std::string wrap(std::string_view s, size_t leftIndent = 0, size_t width = 80);

} // namespace rocket::str

// EOF
