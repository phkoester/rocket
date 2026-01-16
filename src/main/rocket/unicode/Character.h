/**
 * @file Character.h
 *
 * Unicode characters, or grapheme clusters.
 */

#pragma once

#include "rocket/assert.h"
#include "rocket/format/format.h"
#include "rocket/unicode/unicode.h"

#include <ostream>

namespace rocket::unicode {

// `Character` ----------------------------------------------------------------------------------------------

/**
 * A Unicode character, or a grapheme cluster, consisting of one or more code points.
 *
 * A string suitable for a `Char` can be obtained as a segment from a #rocket::unicode::Iterator with the
 * iterator type #rocket::unicode::IteratorType::Character.
 *
 * Think of this as a basic text-processing element that is superior to `char`, `char32`, or even
 * #rocket::unicode::CodePoint. Use it whenever you can, and make your code Unicode-ready.
 *
 * @note Internally, this class is backed by a string view. It does not copy the string passed to its
 *     constructor. Therefore, the string must remain valid for the lifetime of the character.
 *
 * @tparam C the character type
 *
 * ## Examples
 *
 * ```
 * auto c1 = "a"_c; // ASCII character 'a'
 * assert(c1.width() == 1);
 * auto c2 = "€"_c; // U+20AC (EURO SIGN)
 * assert(c2.width() == 1);
 * auto c3 = "👨"_c; // U+1F468 (MAN)
 * assert(c3.width() == 2);
 * // U+1F468 (MAN), U+200D (ZERO WIDTH JOINER), U+1F469 (WOMAN), U+200D (ZERO WIDTH JOINER), U+1F466 (BOY)
 * auto c4 = "👨‍👩‍👦"_c;
 * assert(c4.countCodePoints() == 5);
 * assert(c4.width() == 2);
 * ```
 */
template<typename C> requires IsChar<C>
struct Character {
  /**
   * @ctor
   *
   * This constructor does not check that @p s describes a valid Unicode character. If the character is
   * invalid, its behavior is undefined. Use #rocket::unicode::Iterator to obtain segments suitable for
   * `Character`.
   *
   * @param s a string. The string must not be empty and must remain valid for the lifetime of the character.
   */
  explicit constexpr Character(std::basic_string_view<C> s) :
      s_(s) {
    ROCKET_CHECK(s, not s.empty());
  }

  /// @member_op_cast{`std::basic_string_view<C>`}
  constexpr operator std::basic_string_view<C>() const { return s_; }

  /**
   * Checks if the character is an ASCII character.
   *
   * @return whether the character is an ASCII character
   */
  bool
  ascii() const {
    return s_.size() == 1 && static_cast<uint32_t>(s_[0]) < 0x80;
  }

  /**
   * Returns the number of code points in the character.
   *
   * @note You usually don't need this function. If you do, something might be wrong with your code. Using
   *     this function on a segment returned by #rocket::unicode::Iterator is redundant and inefficient.
   *
   * @return the number of code points in the character
   */
  u64
  countCodePoints() const {
    u64 ret = 0;
    for (u64 pos = 0; pos < s_.size(); /* Empty */) {
      nextCodePoint(s_, pos);
      ++ret;
    }
    return ret;
  }

  /**
   * Checks if the character is a carriage return / line feed sequence (CR/LF).
   *
   * @return whether the character is a carriage return / line feed sequence
   */
  bool
  crLf() const {
    return s_.size() == 2 && s_[0] == '\r' && s_[1] == '\n';
  }

  /**
   * Checks if the character is an end of line (EOL)
   *
   * @return whether the character is an end of line
   */
  bool
  eol() const {
    return lf() || crLf();
  }

  /**
   * Checks if the character is the specified ASCII character.
   *
   * @param c the ASCII character to check
   * @return whether the character is the specified ASCII character
   */
  bool
  is(char c) const {
    return ascii() && s_[0] == c;
  }

  /**
   * Checks if the character is whitespace.
   *
   * @return whether the character is whitespace
   */
  bool
  isWhitespace() const {
    for (u64 pos = 0; pos < s_.size(); /* Empty */) {
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
   * @return whether the character is a hexadecimal digit
   */
  bool
  isXdigit() const {
    return s_.size() == 1 && std::isxdigit(s_[0]);
  }

  /**
   * Checks if the character is a line feed (LF).
   *
   * @return whether the character is a line feed
   */
  bool
  lf() const {
    return s_.size() == 1 && s_[0] == '\n';
  }

  /**
   * Checks if the character is a no-break space
   *
   * @return whether the character is a no-break space
   */
  bool
  nbsp() const {
    u64 pos = 0;
    auto cp = nextCodePoint(s_, pos);
    if (cp != U'\u00A0') { // U+00A0 (NO-BREAK SPACE)
      return false;
    }
    return pos == s_.size();
  }

  /**
   * Returns the size of the underlying string, in code units (`char` or `char32`)
   *
   * @return the size of the underlying string
   */
  u64 size() const { return s_.size(); }

  /**
   * Checks if the character is a tab.
   *
   * @return whether the character is a tab
   */
  bool
  tab() const {
    return s_.size() == 1 && s_[0] == '\t';
  }

  /**
   * Tries to convert the character to a code point.
   *
   * @return a code point if the character consists of exactly one code point, null otherwise
   */
  std::optional<CodePoint>
  toCodePoint() const {
    u64 pos = 0;
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
    for (u64 pos = 0; pos < s_.size(); /* Empty */) {
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

  std::basic_string_view<C> s_; ///< The string, not empty.
};

/**
 * Literal operator for #rocket::unicode::Character.
 *
 * @param p the string literal
 * @param len the length of the string literal
 * @return a #rocket::unicode::Character
 */
constexpr inline Character<char>
operator""_c(const char* p, u64 len) {
  return Character(std::string_view(p, len));
}

/**
 * Literal operator for #rocket::unicode::Character.
 *
 * @param p the string literal
 * @param len the length of the string literal
 * @return a #rocket::unicode::Character
*/
constexpr inline Character<char32>
operator""_c(const char32* p, u64 len) {
  return Character(std::u32string_view(p, len));
}

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
