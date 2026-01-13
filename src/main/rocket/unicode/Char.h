/**
 * @file Char.h
 *
 * Unicode characters, i.e. grapheme clusters.
 */

#pragma once

#include "rocket/format/format.h"
#include "rocket/unicode/unicode.h"

#include <algorithm>

namespace rocket::unicode {

// `Char` ---------------------------------------------------------------------------------------------------

/**
 * A Unicode character, or a grapheme cluster, consisting of one or more code points.
 *
 * A string suitable for a `Char` can be obtained as a segment from a #rocket::unicode::Iterator with the
 * iterator type #rocket::unicode::IteratorType::Char.
 */
template<typename C> requires Character<C>
struct Char {
  /**
   * @ctor
   *
   * @param s a string. The string must remain valid for the lifetime of the character.
   */
  explicit constexpr Char(std::basic_string_view<C> s) : s_(s) {}

  /// @member_op_cast{`std::basic_string_view<C>`}
  constexpr operator std::basic_string_view<C>() const { return s_; }

  /**
   * If this character consists of a single code point, returns it. Otherwise, returns null.
   *
   * @return the code point, or null if the character is empty or consists of multiple code points
   */
  std::optional<CodePoint>
  codePoint() const {
    if (empty()) {
      return std::nullopt;
    }
    size_t pos = 0;
    auto cp = nextCodePoint(s_, pos);
    if (pos == s_.size()) {
      return cp;
    }
    return std::nullopt;
  }

  /**
   * Returns `true` if the character is a CRLF.
   *
   * @return `true` if the character is a CRLF
   */
  bool
  crlf() const {
    size_t pos = 0;
    auto cp = nextCodePoint(s_, pos);
    if (cp != '\r') {
      return false;
    }
    cp = nextCodePoint(s_, pos);
    if (cp != '\n') {
      return false;
    }
    return pos == s_.size();
  }

  constexpr bool empty() const { return s_.empty(); }

  /**
   * Returns `true` if the character is an EOL (end of line).
   *
   * @return `true` if the character is an EOL
   */
  bool
  eol() const {
    return lf() || crlf();
  }

  /**
   * Returns `true` if the character is whitespace.
   *
   * @return `true` if the character is whitespace
   */
  bool
  isWhitespace() const {
    for (size_t pos = 0; pos < s_.size(); /* Empty */) {
      auto cp = nextCodePoint(s_, pos);
      if (not CodePoint(cp).isWhitespace()) {
        return false;
      }
    }
    return true;
  }

  /**
   * Returns `true` if the character is a LF.
   *
   * @return `true` if the character is a LF
   */
  bool
  lf() const {
    size_t pos = 0;
    auto cp = nextCodePoint(s_, pos);
    if (cp != '\n') {
      return false;
    }
    return pos == s_.size();
  }

  /**
   * Returns `true` if the character is a no-break space
   *
   * @return `true` if the character is a no-break space
   */
  bool
  nbsp() const {
    size_t pos = 0;
    auto cp = nextCodePoint(s_, pos);
    if (cp != U'\u00A0') { // U+00A0 (NO-BREAK SPACE)
      return false;
    }
    return pos == s_.size();
  }

  size_t size() const { return s_.size(); }

  /**
   * Returns `true` if the character is a tab.
   *
   * @return `true` if the character is a tab
   */
  bool
  tab() const {
    size_t pos = 0;
    auto cp = nextCodePoint(s_, pos);
    if (cp != '\t') {
      return false;
    }
    return pos == s_.size();
  }

  /**
   * Calculates the character's display width.
   *
   * @return the character's display width, in the range @f$[0,2]@f$
   */
  uint8_t
  width() const {
    uint8_t ret = 0;
    for (size_t pos = 0; pos < s_.size(); /* Empty */) {
      auto cp = nextCodePoint(s_, pos);
      ret = std::max(ret, CodePoint(cp).width());
      if (ret == 2) {
        return 2;
      }
      /*
       * In the rust crate `unicode-display-width`, the following code is used to handle U+FEOF:
       *
       *  // emoji style variation selector
       *  if scalar_value == '\u{FE0F}' {
       *    return 2;
       */
    }
    return ret;
  }

private:

  std::basic_string_view<C> s_;
};

/// @fn_format_as{`Char<C>`}
template<typename C> requires Character<C>
constexpr auto
format_as(Char<C> v) {
  return static_cast<std::basic_string_view<C>>(v);
}

} // namespace rocket::unicode

// EOF
