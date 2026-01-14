/**
 * @file Character.h
 *
 * Unicode characters, or grapheme clusters.
 */

#pragma once

#include "rocket/format/format.h"
#include "rocket/unicode/unicode.h"

#include <iostream>

namespace rocket::unicode {

// `Character` ----------------------------------------------------------------------------------------------

/**
 * A Unicode character, or a grapheme cluster, consisting of one or more code points.
 *
 * A string suitable for a `Char` can be obtained as a segment from a #rocket::unicode::Iterator with the
 * iterator type #rocket::unicode::IteratorType::Character.
 */
template<typename C> requires IsChar<C>
struct Character {
  /**
   * @ctor
   *
   * This constructor does not check that @p s is a valid Unicode character. If the character is invalid,
   * the behavior is undefined.
   *
   * @param s a string. The string must remain valid for the lifetime of the character.
   */
  explicit constexpr Character(std::basic_string_view<C> s) : s_(s) {}

  /// @member_op_cast{`std::basic_string_view<C>`}
  constexpr operator std::basic_string_view<C>() const { return s_; }

  /**
   * Checks if the character is an ASCII character.
   *
   * @return `true` if the character is an ASCII character
   */
  bool
  ascii() const {
    return s_.size() == 1 && static_cast<uint32_t>(s_[0]) < 0x80;
  }

  /**
   * Checks if the character is a CR/LF.
   *
   * @return `true` if the character is a CR/LF
   */
  bool
  crLf() const {
    return s_.size() == 2 && s_[0] == '\r' && s_[1] == '\n';
  }

  /**
   * Checks if the character is empty.
   *
   * @return `true` if the character is empty
   */
  constexpr bool empty() const { return s_.empty(); }

  /**
   * Checks if the character is an end of line (EOL)
   *
   * @return `true` if the character is an EOL
   */
  bool
  eol() const {
    return lf() || crLf();
  }

  /**
   * Checks if the character is the specified ASCII character.
   *
   * @param c the ASCII character to check
   * @return `true` if the character is the specified ASCII character
   */
  bool
  is(char c) const {
    return ascii() && s_[0] == c;
  }

  /**
   * Checks if the character is whitespace.
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
   * Checks if the character is a hexadecimal digit.
   *
   * @return `true` if the character is a hexadecimal digit
   */
  bool
  isXdigit() const {
    return s_.size() == 1 && std::isxdigit(s_[0]);
  }

  /**
   * Checks if the character is a LF.
   *
   * @return `true` if the character is a LF
   */
  bool
  lf() const {
    return s_.size() == 1 && s_[0] == '\n';
  }

  /**
   * Checks if the character is a no-break space
   *
   * @return `true` if the character is a no-break space
   */
  bool
  nbsp() const {
    if (empty()) {
      return false;
    }
    size_t pos = 0;
    auto cp = nextCodePoint(s_, pos);
    if (cp != U'\u00A0') { // U+00A0 (NO-BREAK SPACE)
      return false;
    }
    return pos == s_.size();
  }

  /**
   * Returns the size of the underlying string, in code units (`char` or `char32_t`)
   *
   * @return the size of the underlying string
   */
  size_t size() const { return s_.size(); }

  /**
   * Checks if the character is a tab.
   *
   * @return `true` if the character is a tab
   */
  bool
  tab() const {
    return s_.size() == 1 && s_[0] == '\t';
  }

  /**
   * Tries to convert the character to a code point.
   *
   * @return a code point if the character consists of exctly one codepoint, null otherwise
   */
  std::optional<CodePoint>
  toCodePoint() const {
    if (empty()) {
      return std::nullopt;
    }
    size_t pos = 0;
    auto cp = nextCodePoint(s_, pos);
    if (pos == s_.size()) {
      return CodePoint(cp);
    }
    return std::nullopt;
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

ROCKET_TESTING_PRIVATE:

  std::basic_string_view<C> s_; ///< The string.

  /**
   * Returns the number of code points in the character.
   *
   * @return the number of code points in the character
   */
  size_t
  countCodePoints() const {
    size_t ret = 0;
    for (size_t pos = 0; pos < s_.size(); /* Empty */) {
      nextCodePoint(s_, pos);
      ++ret;
    }
    return ret;
  }
};

/// @op_output{#rocket::unicode::Character}
template<typename C> requires IsChar<C>
inline std::ostream&
operator<<(std::ostream& lhs, Character<C> rhs) {
  return lhs << static_cast<std::basic_string_view<C>>(rhs);
}

/// @fn_format_as{#rocket::unicode::Character}
template<typename C> requires IsChar<C>
constexpr auto
format_as(Character<C> v) {
  return static_cast<std::basic_string_view<C>>(v);
}

} // namespace rocket::unicode

// EOF
