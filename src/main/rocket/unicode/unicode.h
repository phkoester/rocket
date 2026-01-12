/**
 * @file unicode.h
 *
 * A Unicode API.
 */

#pragma once

#include "rocket/Cow.h"
#include "rocket/UnorderedBimap.h"
#include "rocket/rocket.h"
#include "rocket/format/format.h"
#include "rocket/nio/nio-fwd.h"

#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace rocket::unicode {

// `CodePoint` ----------------------------------------------------------------------------------------------

/**
 * A code-point type.
 */
struct CodePoint {
  /// @ctor_default
  constexpr CodePoint() : v_(0) {}

  /**
   * @ctor
   *
   * @param v a `char` value. This must be an ASCII character in the range @f$[0,127]@f$
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

  /// @member_op_cast{`char32_t`}
  operator char32_t() const { return v_; }

  /// @member_op_cast{`std::string`}
  explicit operator std::string() const;

  /// @member_op_cast{`std::u32string`}
  inline explicit operator std::u32string() const { return { v_ }; }

  /// @member_fn_hash
  inline size_t hash() const { return std::hash<char32_t>()(v_); }

  bool isAscii() const { return v_ < 0x80; }

  /**
   * Returns `true` if this code point is printable.
   *
   * @return `true` if this code point is printable
   */
  bool isPrint() const;

  /**
   * Returns `true` if this code point is whitespace.
   *
   * @return `true` if this code point is whitespace
   */
  bool isWhitespace() const;

  /**
   * Returns a lower-case code point for this code point.
   *
   * @return a code point in lower case
   */
  CodePoint lower() const;

  /**
   * Returns an upper-case code point for this code point.
   *
   * @return a code point in upper case
   */
  CodePoint upper() const;

  /**
   * Returns `true` if this code point is valid.
   *
   * A code point is valid if it is less than or equal to U+10FFFF and not a surrogate in the range
   * [U+D800,U+DFFF].
   *
   * @return `true` if this code point is valid
   */
  bool valid() const;

  /**
   * Calculates the display width for a code point.
   *
   * @return a width in the range @f$[0,2]@f$
   */
  uint8_t width() const;

private:

  char32_t v_; ///< The code-point value.
};

/// @op_output{#rocket::unicode::CodePoint}
std::ostream& operator<<(std::ostream& lhs, CodePoint rhs);

/// @fn_read{#rocket::unicode::CodePoint}
size_t read(nio::Source& in, CodePoint& out);

} // namespace rocket::unicode

// `fmt::formatter<CodePoint>` ------------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::unicode::CodePoint}
 *
 * This formatter uses the same format specifiers as the underlying formatter for type `std::string`.
 */
template<typename C>
struct fmt::formatter<rocket::unicode::CodePoint, C> {
  /// @cond undocumented

  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const rocket::unicode::CodePoint& v, FormatContext& ctx) const {
    if constexpr (std::is_same_v<C, char>) {
      return underlying_.format(fmt::format("U+{:0>4X}", static_cast<uint32_t>(v)), ctx);
    } else {
      return underlying_.format(fmt::format(U"U+{:0>4X}", static_cast<uint32_t>(v)), ctx);
    }
  }

  constexpr const C*
  parse(fmt::parse_context<C>& ctx) {
    return underlying_.parse(ctx);
  }

  constexpr void
  set_debug_format(bool v = true) {
    underlying_.set_debug_format(v);
  }

  /// @endcond

private:

  fmt::formatter<basic_string_view<C>, C> underlying_;
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
  static consteval rocket::unicode::CodePoint min() { return U'\u0000'; }

  /**
   * Returns the maximum code-point value, which is U+10FFFF.
   *
   * @return the maximum code-point value
   */
  static consteval rocket::unicode::CodePoint max() { return U'\U0010FFFF'; }
};

namespace rocket::unicode {

// `CodePoints`----------------------------------------------------------------------------------------------

/**
 * A code-point container.
 */
using CodePoints = std::vector<CodePoint>;

// `Grapheme` -----------------------------------------------------------------------------------------------

/**
 * A grapheme, more precisely a grapheme cluster, consisting of one or more code points.
 */
struct Grapheme {
  /// @ctor_default
  constexpr Grapheme() {}

  /**
   * @ctor
   *
   * @param cps a code-point container
   */
  explicit Grapheme(const CodePoints& cps) : codePoints_(cps) {}

  /**
   * @ctor
   *
   * @param cps a code-point container
   */
  explicit Grapheme(CodePoints&& cps) : codePoints_(std::move(cps)) {}

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
  inline bool operator==(const Grapheme& rhs) const { return codePoints_ == rhs.codePoints_; }

  /// @member_op_ne
  inline bool operator!=(const Grapheme& rhs) const { return codePoints_ != rhs.codePoints_; }

  /**
   * Returns the code point from this grapheme if there is exactly one, otherwise returns null.
   *
   * @return a code point if there is exactly one, otherwise null
   */
  inline std::optional<CodePoint>
  codePoint() const {
    if (codePoints_.size() == 1) {
      return codePoints_[0];
    }
    return std::nullopt;
  }

  /**
   * Returns `true` if this grapheme is a CRLF (carriage return / line feed, `"\r\n"`).
   *
   * @return `true` if this grapheme is a CRLF
   */
  inline bool
  crlf() const {
    return codePoints_.size() == 2 && codePoints_[0] == '\r' && codePoints_[1] == '\n';
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
    return codePoints_.size() == 1 && codePoints_[0] == '\n';
  }

  /**
   * Returns `true` if this grapheme is an NBSP (non-breaking space, U+00A0).
   *
   * @return `true` if this grapheme is an NBSP
   */
  inline bool
  nbsp() const {
    return codePoints_.size() == 1 && codePoints_[0] == U'\u00a0';
  }

  /**
   * Returns `true` if this grapheme is printable.
   *
   * @return `true` if this grapheme is printable
   */
  bool print() const;

  /**
   * Returns the grapheme's size in code points.
   *
   * @return the grapheme's size in code points
   */
  inline size_t size() const { return codePoints_.size(); }

  /**
   * Returns `true` if this grapheme is a tab
   *
   * @return `true` if this grapheme is a tab
   */
  inline bool
  tab() const {
    return codePoints_.size() == 1 && codePoints_[0] == '\t';
  }

  /**
   * Returns `true` if this grapheme is whitespace.
   *
   * @return `true` if this grapheme is whitespace
   */
  // XXX
  inline bool
  whitespace() const {
    return codePoints_.size() == 1 && codePoints_[0].isWhitespace();
  }

  /**
   * Calculates the grapheme's display width.
   *
   * @return the grapheme's display width, in the range @f$[0,2]@f$
   */
  uint8_t width() const;

private:

  /// The code points this grapheme consists of.
  CodePoints codePoints_;
};

/// @op_output{#rocket::unicode::Grapheme}
std::ostream& operator<<(std::ostream& lhs, const Grapheme& rhs);

/// @fn_read{#rocket::unicode::Grapheme}
size_t read(nio::Source& in, Grapheme& out);

} // namespace rocket::unicode

// `fmt::formatter<Grapheme>` -------------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::unicode::Grapheme}
 *
 * This formatter uses the same format specifiers as the underlying formatter for type `std::string`.
 */
template<typename C>
struct fmt::formatter<rocket::unicode::Grapheme, C> {
  /// @cond undocumented

  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const rocket::unicode::Grapheme& v, FormatContext& ctx) const {
    return underlying_.format(static_cast<std::basic_string<C>>(v), ctx);
  }

  constexpr const C*
  parse(fmt::parse_context<C>& ctx) {
    return underlying_.parse(ctx);
  }

  constexpr void
  set_debug_format(bool v = true) {
    underlying_.set_debug_format(v);
  }

  /// @endcond

private:

  fmt::formatter<basic_string_view<C>, C> underlying_;
};

namespace rocket::unicode {

// `Graphemes` ----------------------------------------------------------------------------------------------

/**
 * A grapheme container.
 */
using Graphemes = std::vector<Grapheme>;

// Functions ------------------------------------------------------------------------------------------------

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
 * Returns the display width for graphemes that make up a string.
 *
 * @param grs graphemes that make up a string
 * @param index index of the first element
 * @param n the number of elementss
 * @return a width
 */
// XXX Weg
size_t width(const Graphemes& grs, size_t index = 0, size_t n = NPOS);

// UTF8 .....................................................................................................

namespace utf8 {

/**
 * Decodes a UTF-8 byte sequence from a buffer to a code point.
 *
 * The function returns -1 if
 * - @p numBytes cannot be determined (in which case it is set to 0),
 * - the buffer is too small to contain the full byte sequence,
 * - the byte sequence is invalid
 *
 * @param buf a buffer pointing to a UTF-8 byte sequence
 * @param size the size of the buffer in bytes
 * @param numBytes the number of bytes needed to encode the code point, as indicated by the first byte in the
 *     buffer, or 0 if the value cannot be determined
 * @return the decoded code point, or -1 if the result cannot be determined. The result is not checked for
 *     valid code points
 */
char32_t decodeBuffer(const char* buf, size_t size, size_t& numBytes);

/**
 * Encodes a code point to a UTF-8 byte sequence into a buffer.
 *
 * @param cp a code point
 * @param buf a buffer to encode the code point into. The buffer must be large enough to contain the encoded
 *     code point, so there should be at least 4 bytes available
 * @return the number of bytes encoded, or 0 if the code point is invalid
 */
size_t encodeBuffer(char32_t cp, char* buf);

/**
 * Given the first byte @p c in a UTF-8 byte sequence, returns the size in bytes of the UTF-8 encoded code
 * point, including @p c itself.
 *
 * If @p c is not a valid first byte of a UTF-8 encoded code point, this function returns 0.
 *
 * @param c a character from a UTF-8 byte sequence
 * @return a value in the range @f$[0,4]@f$
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
CodePoints codePoints(std::string_view s, UnorderedBimap<size_t, size_t>* positions = nullptr);

/**
 * Returns `true` if the byte @p c is a UTF-8 continuation byte.
 *
 * @param c a character from a UTF-8 byte sequence
 * @return `true` if @p c is a UTF-8 continuation byte
 */
inline bool continuationByte(char c) { return (c & 0xC0) == 0x80; }

/**
 * Counts the number of code points in a UTF-8 string.
 *
 * @param s a UTF-8 string
 * @return the number of code points
 */
// XXX Am besten weg
size_t countCodePoints(std::string_view s);

/**
 * Counts the number of graphemes in a UTF-8 string.
 *
 * @param s a UTF-8 string
 * @return the number of graphemes
 */
// XXX Am besten weg
size_t countGraphemes(std::string_view s);

/**
 * Returns the graphemes of a UTF-8 string.
 *
 * @param s a UTF-8 string
 * @param positions if nonnull, then the left index of this map translates grapheme positions to `char`
 *     positions after the functions returns
 * @return a grapheme container
 */
// XXX Weg
Graphemes graphemes(std::string_view s, UnorderedBimap<size_t, size_t>* positions = nullptr);

/**
 * Validates the UTF-8 string @p s.
 *
 * If the string @p s is found to be valid, the result contains a reference to the original string @p s.
 *
 * If the string @p s is found to be invalid, the result contains a modified, corrected version of the
 * string. Invalid or incomplete UTF-8 byte sequences, as well as invalid code points, are replaced by the
 * replacement character `�` (U+FFFD).
 *
 * @attention The `std::string_view` @p s must remain valid for the lifetime of the returned #rocket::Cow!
 *
 * @param s the string to validate
 * @param positions if nonnull, then the left index of this map translates `char` offsets from @p s
 *   to `char` offsets in the result for each code point and the end of string.
 */
Cow<std::string_view, std::string>
validate(const std::string_view& s, UnorderedBimap<size_t, size_t>* positions = nullptr);

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
CodePoints codePoints(std::u32string_view s, UnorderedBimap<size_t, size_t>* positions = nullptr);

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
Graphemes graphemes(std::u32string_view s, UnorderedBimap<size_t, size_t>* positions = nullptr);

/**
 * Validates the UTF-32 string @p s.
 *
 * If the string @p s is found to be valid, the result contains a reference to the original string @p s.
 *
 * If the string @p s is found to be invalid, the result contains a modified, corrected version of the
 * string. Invalid code points are replaced by the replacement character `�` (U+FFFD).
 *
 * @attention The `std::u32string_view` @p s must remain valid for the lifetime of the returned #rocket::Cow!
 *
 * @param s the string to validate
 */
Cow<std::u32string_view, std::u32string>
validate(const std::u32string_view& s);

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

using utf8::validate;
using utf32::validate;

} // namespace rocket::unicode

// EOF
