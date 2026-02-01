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

// #BasicCharacter ------------------------------------------------------------------------------------------

/**
 * The implementation of both #rocket::unicode::Character and #rocket::unicode::CharacterView.
 *
 * @tparam C the character type
 */
template<typename C, typename String> requires IsChar<C>
struct BasicCharacter {
  /// A compile-time constant that is `true` if the character is a view, `false` otherwise.
  static constexpr bool IS_VIEW = std::is_same_v<String, std::basic_string_view<C>>;

  /**
   * @ctor
   *
   * This constructor does not check that @p str describes a valid Unicode character. If the character is
   * invalid, its behavior is undefined. Use #rocket::unicode::Iterator to obtain segments suitable for a
   * character.
   *
   * @param str a string. The string must not be empty. If the instance is a character view, the string must
   *     remain valid for the lifetime of the instance
   */
  constexpr explicit BasicCharacter(std::basic_string_view<C> str) : str_(str) {
    ROCKET_CHECK(str, not str.empty());
  }

  /**
   * @ctor
   *
   * Implicitly constructs a view from a string.
   *
   * @param rhs the right-hand side, which holds a string
   */
  template<typename Char> requires std::is_same_v<Char, C> && IS_VIEW
  constexpr explicit BasicCharacter(const BasicCharacter<Char, std::basic_string<Char>>& rhs ) :
      str_(static_cast<std::basic_string_view<C>>(rhs)) {}

  /// @member_op_cast{#std::basic_string_view}
  constexpr operator std::basic_string_view<C>() const noexcept { return str_; } // NOLINT

  /**
   * Checks if the character is an ASCII character.
   *
   * @return whether the character is an ASCII character
   */
  [[nodiscard]] bool
  constexpr ascii() const noexcept {
    return str_.size() == 1 && static_cast<u32>(str_[0]) < 0x80;
  }

  /**
   * Returns the number of code points in the character.
   *
   * @note You usually don't need this function. If you do, something might be wrong with your code. Using
   *     this function on a segment returned by #rocket::unicode::Iterator is redundant and inefficient.
   *
   * @return the number of code points in the character
   */
  [[nodiscard]] u64
  countCodePoints() const {
    u64 ret = 0;
    for (u64 pos = 0; pos < str_.size(); /* Empty */) {
      nextCodePoint(str_, pos);
      ++ret;
    }
    return ret;
  }

  /**
   * Checks if the character is a carriage return / line feed sequence (CR/LF).
   *
   * @return whether the character is a carriage return / line feed sequence
   */
  [[nodiscard]] constexpr bool
  crLf() const noexcept {
    return str_.size() == 2 && str_[0] == '\r' && str_[1] == '\n';
  }

  /**
   * Checks if the character is an end of line (EOL)
   *
   * @return whether the character is an end of line
   */
  [[nodiscard]] constexpr bool
  eol() const noexcept {
    return lf() || crLf();
  }

  /**
   * Checks if the character equals the specified ASCII character.
   *
   * @param c the ASCII character to check
   * @return whether the character equals the specified ASCII character
   */
  [[nodiscard]] constexpr bool
  eq(char c) const noexcept {
    return ascii() && str_[0] == c;
  }

  /**
   * Checks if the character is whitespace.
   *
   * @return whether the character is whitespace
   */
  [[nodiscard]] bool
  isWhitespace() const {
    for (u64 pos = 0; pos < str_.size(); /* Empty */) {
      auto cp = nextCodePoint(str_, pos);
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
  [[nodiscard]] bool
  isXdigit() const noexcept {
    return str_.size() == 1 && std::isxdigit(str_[0]);
  }

  /**
   * Checks if the character is a line feed (LF).
   *
   * @return whether the character is a line feed
   */
  [[nodiscard]] constexpr bool
  lf() const noexcept {
    return str_.size() == 1 && str_[0] == '\n';
  }

  /**
   * Checks if the character is a no-break space
   *
   * @return whether the character is a no-break space
   */
  [[nodiscard]] bool
  nbsp() const {
    u64 pos = 0;
    auto cp = nextCodePoint(str_, pos);
    if (cp != U'\u00A0') { // U+00A0 (NO-BREAK SPACE)
      return false;
    }
    return pos == str_.size();
  }

  /**
   * Returns the size of the underlying string, in code units (`char` or `char32`)
   *
   * @return the size of the underlying string
   */
  [[nodiscard]] constexpr u64 size() const noexcept { return str_.size(); }

  /**
   * Checks if the character is a tab.
   *
   * @return whether the character is a tab
   */
  [[nodiscard]] constexpr bool
  tab() const noexcept {
    return str_.size() == 1 && str_[0] == '\t';
  }

  /**
   * Tries to convert the character to a code point.
   *
   * @return a code point if the character consists of exactly one code point, null otherwise
   */
  [[nodiscard]] std::optional<CodePoint>
  toCodePoint() const {
    u64 pos = 0;
    auto cp = nextCodePoint(str_, pos);
    if (pos == str_.size()) {
      return CodePoint(cp);
    }
    return std::nullopt;
  }

  /**
   * Calculates the character's display width.
   *
   * @return the character's display width, in the range @f$[0,2]@f$
   */
  [[nodiscard]] u8
  width() const {
    u8 ret = 0;
    for (u64 pos = 0; pos < str_.size(); /* Empty */) {
      auto cp = nextCodePoint(str_, pos);
      ret = std::max(ret, CodePoint(cp).width());
      if (ret == 2) {
        return 2;
      }
      /*
       * In the rust crate `unicode-display-width`, the following code is used to handle U+FE0F:
       *
       *  // emoji style variation selector
       *  if scalar_value == '\u{FE0F}' {
       *    return 2;
       */
    }
    return ret;
  }

private:

  /**
   * The string, which is either a #std::basic_string or a #std::basic_string_view.
   *
   * The string must not be empty.
   */
  String str_;
};

/// @op_output{#rocket::unicode::BasicCharacter}
template<typename C, typename String> requires IsChar<C>
inline std::ostream&
operator<<(std::ostream& lhs, const BasicCharacter<C, String>& rhs) {
  return lhs << static_cast<std::basic_string_view<C>>(rhs);
}

/// @fn_format_as{#rocket::unicode::BasicCharacter}
template<typename C, typename String> requires IsChar<C>
constexpr auto
format_as(const BasicCharacter<C, String>& val) {
  return static_cast<std::basic_string_view<C>>(val);
}

// #Character -----------------------------------------------------------------------------------------------

/**
 * A Unicode character, or a grapheme cluster, consisting of one or more code points.
 *
 * A string suitable for a #rocket::unicode::Character can be obtained as a segment from a
 * #rocket::unicode::Iterator with the iterator type #rocket::unicode::IteratorType::Character.
 *
 * Think of this as a basic text-processing element that is superior to `char`, `char32`, or even
 * #rocket::unicode::CodePoint. Use it whenever you can, and make your code Unicode-ready.
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
using Character = BasicCharacter<C, std::basic_string<C>>;

/**
 * Literal operator for #rocket::unicode::Character.
 *
 * @param p the string literal
 * @param len the length of the string literal
 * @return a #rocket::unicode::Character
 */
constexpr Character<char>
operator""_c(const char* p, u64 len) {
  return Character<char>(std::string_view(p, len));
}

/**
 * Literal operator for #rocket::unicode::Character.
 *
 * @param p the string literal
 * @param len the length of the string literal
 * @return a #rocket::unicode::Character
*/
constexpr Character<char32>
operator""_c(const char32* p, u64 len) {
  return Character<char32>(std::u32string_view(p, len));
}

// #CharacterView -------------------------------------------------------------------------------------------

/**
 * This is very much the same as #rocket::unicode::Character, except that it is a view into a string, not a
 * string itself.
 *
 * @tparam C the character type
 *
 * ## Examples
 *
 * ```
 * auto c = "a"_cv; // ASCII character 'a'
 * assert(c1.width() == 1);
 * ```
 */
template<typename C> requires IsChar<C>
using CharacterView = BasicCharacter<C, std::basic_string_view<C>>;

/**
 * Literal operator for #rocket::unicode::CharacterView.
 *
 * @param p the string literal
 * @param len the length of the string literal
 * @return a #rocket::unicode::CharacterView
 */
constexpr CharacterView<char>
operator""_cv(const char* p, u64 len) {
  return CharacterView<char>(std::string_view(p, len));
}

/**
 * Literal operator for #rocket::unicode::CharacterView.
 *
 * @param p the string literal
 * @param len the length of the string literal
 * @return a #rocket::unicode::CharacterView
*/
constexpr CharacterView<char32>
operator""_cv(const char32* p, u64 len) {
  return CharacterView<char32>(std::u32string_view(p, len));
}

} // namespace rocket::unicode

// EOF
