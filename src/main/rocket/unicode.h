/**
 * @file unicode.h
 *
 * A Unicode API.
 */

#pragma once

#include "base.h"
#include "container.h"

#include "format.h"
#include "unicode-decl.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace rocket::unicode {

// `CodePoint` ----------------------------------------------------------------------------------------------

/**
 * A code-point type.
 */
// XXX Warum nicht char32_t?
struct CodePoint {
  /// @ctor_default
  constexpr CodePoint() : v_(0) {}

  /**
   * @ctor
   *
   * @param v a `char` value. This must be an ASCII character in the range [0,127]
   */
  // cppcheck-suppress noExplicitConstructor
  CodePoint(char v);

  /**
   * @ctor
   *
   * @param v a `char32_t` value
   */
  // cppcheck-suppress noExplicitConstructor
  constexpr CodePoint(char32_t v) : v_(v) {}

  /**
   * @ctor
   *
   * @param v a `uint32_t` value
   */
  // cppcheck-suppress noExplicitConstructor
  constexpr CodePoint(uint32_t v) : v_(v) {}

  // XXX Warum kein operator char32_t()?

  /// @member_op_cast{`uint32_t`}
  operator uint32_t() const { return v_; }

  /// @member_op_cast{`std::string`}
  explicit operator std::string() const;

  /// @member_op_cast{`std::u32string`}
  inline explicit operator std::u32string() const { return std::u32string { v_ }; }

  /// @member_fn_hash
  inline size_t hash() const { return std::hash<uint32_t>()(v_); }

  /**
   * Returns a lower-case code point for this code point.
   *
   * @return a code point in lower case
   */
  CodePoint lower() const;

  /**
   * Returns `true` if this code point is printable.
   *
   * @param width if nonnull, then this is assigned the display width of the code point.
   *
   * @return `true` if this code point is printable. If this function returns `true`, @p width is guaranteed
   *     to be positive
   */
  bool print(int8_t* width = nullptr) const;

  /**
   * Returns an upper-case code point for this code point.
   *
   * @return a code point in upper case
   */
  CodePoint upper() const;

  /**
   * Returns `true` if this code point is whitespace.
   *
   * @return `true` if this code point is whitespace
   */
  bool whitespace() const;

  /**
   * Returns the display width for a code point.
   *
   * This function defines the display width of a code point as follows:
   *
   * - The null character (U+0000) has a width of 0.
   * - Other C0/C1 control characters and DEL will lead to a return value of -1.
   * - Nonspacing and enclosing combining characters (general category code Mn or Me in the Unicode database)
   *   have a width of 0.
   * - SOFT HYPHEN (U+00AD) has a width of 1.
   * - Other format characters (general category code Cf in the Unicode database) and ZERO WIDTH SPACE
   *   (U+200B) have a width of 0.
   * - Hangul Jamo medial vowels and final consonants (U+1160-U+11FF) have a width of 0.
   * - Spacing characters in the East Asian Wide (W) or East Asian Full-width (F) category as defined in
   *   Unicode Technical Report #11 have a width of 2.
   * - Emoji characters in the Emoji_Presentation category as defined in Emoji Data for UTS #51 have a width
   *   of 2.
   * - All remaining characters (including all printable ISO 8859-1 and WGL4 characters, Unicode control
   *   characters, etc.) have a width of 1.
   *
   * This implementation is inspired by
   *
   * - [Markus Kuhn's work](http://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c)
   * - the Rust crate [`unicode-display-width`](https://crates.io/crates/unicode-display-width)
   *
   * @return a width in the range [-1,2]
   */
  int8_t width() const;

private:

  uint32_t v_;
};

// With this static assertion, we can safely use `reinterpret_cast` between `CodePoint`, `uint32_t`, and
// `char32_t`
static_assert(sizeof(CodePoint) == sizeof(uint32_t) && sizeof(uint32_t) == sizeof(char32_t));

/// @fn_hash_value{#rocket::unicode::CodePoint}
inline size_t
hash_value(CodePoint v) {
  return v.hash();
}

/**
 * UTF-8 input operator for #rocket::unicode::CodePoint.
 *
 * This operator reads a code point from a UTF-8 stream. If the reading fails, the fail bit of the input
 * stream @p is is set, and @p rhs is not assigned any value.
 *
 * @param lhs the input stream
 * @param rhs a code point
 * @return @p lhs
 */
std::istream& operator>>(std::istream& lhs, CodePoint& rhs);

/**
 * Output operator for #rocket::unicode::CodePoint.
 *
 * This operator writes a code point to a UTF-8 stream.
 *
 * @param lhs the output stream
 * @param rhs a code point
 * @return @p lhs
 */
std::ostream& operator<<(std::ostream& lhs, CodePoint rhs);

} // namespace rocket::unicode

// `fmt::formatter<CodePoint>` ------------------------------------------------------------------------------

/// @spec_fmt_formatter{#rocket::unicode::CodePoint}
template<typename Char>
struct fmt::formatter<rocket::unicode::CodePoint, Char> {
  template<typename FormatContext>
  constexpr auto
  format(const rocket::unicode::CodePoint& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    if (underlying_.specs().type() == fmt::presentation_type::debug) {
      // @todo This no longer respects format specs
      return fmt::format_to(ctx.out(), "U+{:0>4X}", static_cast<uint32_t>(v));
    } else {
      return underlying_.format(static_cast<std::string>(v), ctx);
    }
  }

  constexpr const Char*
  parse(fmt::parse_context<Char>& ctx) {
    return underlying_.parse(ctx);
  }

private:

  rocket::format::NativeFormatter<string_view, Char> underlying_;
};

// `std::hash<CodePoint>` -----------------------------------------------------------------------------------

/// @spec_std_hash{#rocket::unicode::CodePoint}
template<>
struct std::hash<rocket::unicode::CodePoint> {
  /**
   * Returns a hash value for @p v.
   *
   * @param v the value to hash
   * @return a hash value
   */
  inline size_t operator()(rocket::unicode::CodePoint v) const { return v.hash(); }
};

// `std::numeric_limits<CodePoint>` -------------------------------------------------------------------------

/// @spec_std_numeric_limits{#rocket::unicode::CodePoint}
template<>
struct std::numeric_limits<rocket::unicode::CodePoint> {
  /**
   * Returns the minimum code-point value, which is U+0000.
   *
   * @return the minimum code-point value
   */
  static consteval rocket::unicode::CodePoint min() { return 0U; }

  /**
   * Returns the maximum code-point value, which is U+10FFFF.
   *
   * @return the maximum code-point value
   */
  static consteval rocket::unicode::CodePoint max() { return 0x10ffffU; } // U+10FFFF (1,114,111)
};

namespace rocket::unicode {

// `CodePoints`----------------------------------------------------------------------------------------------

/**
 * A code-point container.
 */
using CodePoints = std::vector<CodePoint>;

// `Grapheme` -----------------------------------------------------------------------------------------------

/**
 * A grapheme consisting of one or more code points.
 */
struct Grapheme {
  /// The code points this grapheme consists of.
  CodePoints codePoints;
  /// The grapheme's display width, in the range [0,2].
  uint8_t width;

  /// @ctor_default
  constexpr Grapheme() : width(0) {}

  /**
   * @ctor
   *
   * @param cps a code-point container
   */
  explicit Grapheme(const CodePoints& cps);

  /**
   * @ctor
   *
   * @param cps a code-point container
   */
  explicit Grapheme(CodePoints&& cps);

  /**
   * @ctor
   *
   * @param s a UTF-8 string
   */
  explicit Grapheme(std::string_view s);

  /**
   * @ctor
   *
   * @param s a UTF-32 string
   */
  explicit Grapheme(std::u32string_view s);

  /// @member_op_cast{`std::string`}
  explicit operator std::string() const;

  /// @member_op_cast{`std::u32string`}
  explicit operator std::u32string() const;

  /// @member_op_eq
  inline bool operator==(const Grapheme& rhs) const { return codePoints == rhs.codePoints; }

  /// @member_op_ne
  inline bool operator!=(const Grapheme& rhs) const { return codePoints != rhs.codePoints; }

  /**
   * Returns the code point from this grapheme if there is exactly one, otherwise returns null.
   *
   * @return a code point if there is exactly one, otherwise null
   */
  // XXX Sieht unsinnig aus
  inline std::optional<CodePoint>
  codePoint() const {
    if (codePoints.size() == 1)
      return codePoints[0];
    return std::nullopt;
  }

  /**
   * Returns `true` if this grapheme is a CRLF (carriage return / line feed, `"\r\n"`).
   *
   * @return `true` if this grapheme is a CRLF
   */
  inline bool
  crlf() const {
    return codePoints.size() == 2 && codePoints[0] == '\r' && codePoints[1] == '\n';
  }

  /**
   * Returns `true` if this grapheme is an EOL (end of line).
   *
   * @return `true` if this grapheme is an EOL
   */
  inline bool
  eol() const {
    return lf() || crlf();
  }

  /**
   * Returns `true` if this grapheme is a LF (line feed, `"\n"`).
   *
   * @return `true` if this grapheme is a LF
   */
  inline bool
  lf() const {
    return codePoints.size() == 1 && codePoints[0] == '\n';
  }

  /**
   * Returns `true` if this grapheme is an NBSP (non-breaking space, U+00A0).
   *
   * @return `true` if this grapheme is an NBSP
   */
  inline bool
  nbsp() const {
    return codePoints.size() == 1 && codePoints[0] == U'\u00a0';
  }

  /**
   * Returns `true` if this grapheme is printable.
   *
   * @return `true` if this grapheme is printable
   */
  bool print() const;

  /**
   * Returns `true` if this grapheme is a tab (`"\t"`).
   *
   * @return `true` if this grapheme is a tab
   */
  inline bool
  tab() const {
    return codePoints.size() == 1 && codePoints[0] == '\t';
  }

  /**
   * Returns `true` if this grapheme is whitespace.
   *
   * @return `true` if this grapheme is whitespace
   */
  inline bool
  whitespace() const {
    return codePoints.size() == 1 && codePoints[0].whitespace();
  }
};

/**
 * UTF-8 input operator for #rocket::unicode::Grapheme.
 *
 * This operator reads a grapheme from a UTF-8 stream. If the reading fails, the fail bit of the input stream
 * @p is is set, and @p rhs is not assigned any value.
 *
 * @param lhs the input stream
 * @param rhs a grapheme
 * @return @p lhs
 */
std::istream& operator>>(std::istream& lhs, Grapheme& rhs);

/**
 * Output operator for #rocket::unicode::Grapheme.
 *
 * This operator writes a grapheme to a UTF-8 stream.
 *
 * @param lhs the output stream
 * @param rhs a grapheme
 * @return @p lhs
 */
std::ostream& operator<<(std::ostream& lhs, const Grapheme& rhs);

} // namespace rocket::unicode

// `fmt::formatter<Grapheme>` -------------------------------------------------------------------------------

/// @spec_fmt_formatter{#rocket::unicode::Grapheme}
template<typename Char>
struct fmt::formatter<rocket::unicode::Grapheme, Char> {
  template<typename FormatContext>
  constexpr auto
  format(const rocket::unicode::Grapheme& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return underlying_.format(static_cast<std::string>(v), ctx);
  }

  constexpr const Char*
  parse(fmt::parse_context<Char>& ctx) {
    return underlying_.parse(ctx);
  }

private:

  rocket::format::NativeFormatter<string_view, Char> underlying_;
};

namespace rocket::unicode {

// `Graphemes` ----------------------------------------------------------------------------------------------

/**
 * A grapheme container.
 */
using Graphemes = std::vector<Grapheme>;

// Functions ------------------------------------------------------------------------------------------------

/**
 * Converts the ASCII string @p s to a UTF-32 string.
 *
 * @param s an ASCII string
 * @return a UTF-32 string
 */
std::u32string asciiTo32(std::string_view s);

/**
 * Converts the UTF-8 string @p s to a UTF-32 string.
 *
 * @param s a UTF-8 string
 * @return a UTF-32 string
 */
std::u32string utf8To32(std::string_view s);

/**
 * Converts the UTF-32 string @p s to a UTF-8 string.
 *
 * @param s a UTF-32 string
 * @return a UTF-8 string
 */
std::string utf32To8(std::u32string_view s);

/**
 * Returns the display width for code points that make up a grapheme.
 *
 * This implementation is inspired by
 *
 * - the Rust crate [`unicode-display-width`](https://crates.io/crates/unicode-display-width).
 *
 * @param cps code points that make up a grapheme
 * @return a width in the range [0,2].
 */
uint8_t width(const CodePoints& cps);

/**
 * Returns the display width for graphemes that make up a string.
 *
 * @param grs graphemes that make up a string
 * @param index index of the first element
 * @param n the number of elementss
 * @return a width
 */
size_t width(const Graphemes& grs, size_t index = 0, size_t n = NPOS);

// UTF8 .....................................................................................................

namespace utf8 {

/**
 * Given the first byte @p c in a UTF-8 byte sequence, Returns the size in bytes of the code point.
 *
 * If @p c is a continuation byte, this function returns 0.
 *
 * @param c the first byte of a UTF-8 byte sequence
 * @return a value in the range [0,4]. A return value of 0 indicates a UTF-8 continuation byte
 */
uint8_t codePointSize(char c);

/**
 * Returns the code points of a UTF-8 string.
 *
 * @param s a UTF-8 string
 * @param positions if nonnull, then the left index of this map translates code-point positions to `char`
 *     positions after the functions returns
 * @return a code-point container
 */
CodePoints codePoints(std::string_view s, container::UnorderedBimap<size_t, size_t>* positions = nullptr);

/**
 * Returns `true` if the character @p c is a UTF-8 continuation byte.
 *
 * @param c a character
 * @return `true` if @p c is a UTF-8 continuation byte
 */
inline bool continuationByte(char c) { return (c & 0xc0) == 0x80; }

/**
 * Counts the number of code points in a UTF-8 string.
 *
 * @param s a UTF-8 string
 * @return the number of code points
 */
size_t countCodePoints(std::string_view s);

/**
 * Counts the number of graphemes in a UTF-8 string.
 *
 * @param s a UTF-8 string
 * @return the number of graphemes
 */
size_t countGraphemes(std::string_view s);

/**
 * Returns the graphemes of a UTF-8 string.
 *
 * @param s a UTF-8 string
 * @param positions if nonnull, then the left index of this map translates grapheme positions to `char`
 *     positions after the functions returns
 * @return a grapheme container
 */
Graphemes graphemes(std::string_view s, container::UnorderedBimap<size_t, size_t>* positions = nullptr);

/**
 * Checks if string @p s is a valid UTF-8 string.
 *
 * @param s a UTF-8 string; possibly invalid
 * @param out if nonnull, this is assigned a valid UTF-8 string. If @p s is valid, then @p out is assigned
 *    @p s. If @p s is not valid, then @p out is assigned a modified version of @p s where invalid or
 *    incomplete UTF-8 byte sequences are replaced by a sequence of replacement characters `�` (U+FFFD).
 * @return `true` if @p s is a valid UTF-8 string
 */
bool valid(std::string_view s, std::string* out = nullptr);

} // namespace utf8

// UTF-32 ...................................................................................................

namespace utf32 {

/**
 * Returns the code points of a UTF-32 string.
 *
 * @param s a UTF-32 string
 * @param positions if nonnull, then the left index of this map translates code-point positions to
 *     `char32_t` positions after the functions returns (trivial, but provided for completeness)
 * @return a code-point container
 */
CodePoints codePoints(std::u32string_view s, container::UnorderedBimap<size_t, size_t>* positions = nullptr);

/**
 * Counts the number of code points in a UTF-32 string.
 *
 * @param s a UTF-32 string
 * @return the number of code points
 */
inline size_t countCodePoints(std::u32string_view s) { return s.size(); }

/**
 * Counts the number of graphemes in a UTF-32 string.
 *
 * @param s a UTF-32 string
 * @return the number of graphemes
 */
size_t countGraphemes(std::u32string_view s);

/**
 * Returns the graphemes of a UTF-32 string.
 *
 * @param s a UTF-32 string
 * @param positions if nonnull, then the left index of this map translates grapheme positions to `char32_t`
 *     positions after the functions returns
 * @return a grapheme container
 */
Graphemes graphemes(std::u32string_view s, container::UnorderedBimap<size_t, size_t>* positions = nullptr);

} // namespace utf32

// Merge functions from `utf8` and `utf32` so they can be used as overloads ---------------------------------

using utf8::codePoints;
using utf32::codePoints;

using utf8::countCodePoints;
using utf32::countCodePoints;

using utf8::countGraphemes;
using utf32::countGraphemes;

using utf8::graphemes;
using utf32::graphemes;

} // namespace rocket::unicode

// EOF
