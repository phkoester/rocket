/**
 * @file unicode.h
 *
 * A Unicode API.
 */

#pragma once

#include "rocket/Cow.h"
#include "rocket/Bimap.h"
#include "rocket/assert.h"

#include <fmt/format.h>
#include <fmt/xchar.h>

#include <limits>

namespace rocket::unicode {

// #CodePoint -----------------------------------------------------------------------------------------------

/**
 * A code-point type.
 *
 * Instances of #rocket::unicode::CodePoint are immutable and always valid. Constructing a code point with an
 * invalid `char` or `char32` value will throw an exception.
 *
 * Note there is a literal operator for #rocket::unicode::CodePoint:
 *
 * ```
 * use namespace rocket::unicode;
 * auto cp = 'a'_cp;
 * assert(cp.upper() == 'A'_cp);
 * ```
 */
struct CodePoint {
  using Type = char32; ///< The representation type of the code point.

  /**
   * Checks if a `char32` value is a valid code-point value.
   *
   * A code point is valid if it is less than or equal to U+10FFFF and not a surrogate in the range
   * U+D800–U+DFFF.
   *
   * @param val the `char32` value to check
   * @return whether @p val is a valid code-point value
   */
  [[nodiscard]] static constexpr bool
  valid(char32 val) {
    return val <= 0x10FFFFU && (val < 0xD800U || val > 0xDFFFU);
  }

  /// @ctor_default
  constexpr CodePoint() : val_(0) {}

  /**
   * @ctor
   *
   * @param val a `char` value. This must be an ASCII character in the range @f$[0,127]@f$
   * @throws #rocket::InvalidArgument if @p val is not an ASCII character in the range @f$[0,127]@f$
   */
  constexpr CodePoint(char val) : // NOLINT
    val_(val) {
    ROCKET_CHECK(val, ascii(), "Invalid ASCII value 0x{:X}", val);
  }

  /**
   * @ctor
   *
   * @param val a `char32` value. This must be a valid code-point value
   * @throws #rocket::InvalidArgument if @p val is not a valid code-point value
   *
   * @see #rocket::unicode::CodePoint::valid
   */
  constexpr CodePoint(char32 val) : // NOLINT
    val_(val) {
    ROCKET_CHECK(val, valid(val), "Invalid code-point value 0x{:X}", static_cast<u32>(val));
  }

  /// @member_op_cast{#char32}
  constexpr operator char32() const { return val_; } // NOLINT

  /// @member_op_cast{#std::string}
  explicit operator std::string() const;

  /// @member_op_cast{#std::u32string}
  constexpr explicit operator std::u32string() const { return { val_ }; }

  /**
   * Checks if the code point is an ASCII character.
   *
   * @return whether the code point is an ASCII character
   */
  [[nodiscard]] constexpr bool ascii() const { return val_ < 0x80; }

  /**
   * Checks if the code point equals the specified ASCII character.
   *
   * @param c the ASCII character to check
   * @return whether the code point equals the specified ASCII character
   */
  [[nodiscard]] constexpr bool
  eq(char c) const {
    return ascii() && val_ == static_cast<char32>(c);
  }

  /**
   * Checks if the code point is printable.
   *
   * @return whether the code point is printable
   */
  [[nodiscard]] bool isPrint() const;

  /**
   * Checks if the code point is whitespace.
   *
   * @return whether the code point is whitespace
   */
  [[nodiscard]] bool isWhitespace() const;

  /**
   * Returns a lower-case code point for this code point.
   *
   * @return a code point in lower case
   */
  [[nodiscard]] CodePoint lower() const;

  /**
   * Returns an upper-case code point for this code point.
   *
   * @return a code point in upper case
   */
  [[nodiscard]] CodePoint upper() const;

  /**
   * Calculates the display width for a code point.
   *
   * @return the code point's display width, in the range @f$[0,2]@f$
   */
  [[nodiscard]] u8 width() const;

private:

  char32 val_; ///< The code-point value.
};

/**
 * Literal operator for #rocket::unicode::CodePoint.
 *
 * @param val the `char` value
 * @return a #rocket::unicode::CodePoint
 */
constexpr CodePoint
operator""_cp(char val) {
  return { val };
}

/**
 * Literal operator for #rocket::unicode::CodePoint.
 *
 * @param val the `char32` value
 * @return a #rocket::unicode::CodePoint
 */
constexpr CodePoint
operator""_cp(char32 val) {
  return { val };
}

/// @op_output{#rocket::unicode::CodePoint}
std::ostream& operator<<(std::ostream& lhs, CodePoint rhs);

// Functions ------------------------------------------------------------------------------------------------

/**
 * Converts the UTF-8 string @p str to a UTF-32 string.
 *
 * @param str a UTF-8 string
 * @return a UTF-32 string
 */
std::u32string utf8To32(std::string_view str); // NOLINT

/**
 * Converts the UTF-32 string @p str to a UTF-8 string.
 *
 * @param str a UTF-32 string
 * @return a UTF-8 string
 */
std::string utf32To8(std::u32string_view str); // NOLINT

// UTF8 .....................................................................................................

namespace utf8 {

/**
 * Returns the length of the UTF-8 sequence starting with the byte @p c.
 *
 * @param c the first byte
 * @return the length of the UTF-8 sequence starting with the byte @p c
 * @throw #rocket::InvalidArgument if the byte @p c is neither a single nor a UTF-8 lead byte
 */
u64 lengthFromByte(char c);

/**
 * Returns the next code point from the UTF-8 string @p str at the position @p pos.
 *
 * @param str a UTF-8 string
 * @param pos the position to get the next code point from. This must be less than the size of @p str. The
 *   position is updated to the position of the next code point
 * @return the next code point
 * @throw #rocket::InvalidArgument if @p pos is out of bounds
 * @throw #rocket::InvalidState if the UTF-8 sequence is invalid
 */
CodePoint nextCodePoint(std::string_view str, u64& pos);

/**
 * Validates the UTF-8 string @p str.
 *
 * If the string @p str is found to be valid, the result contains a reference to the original string @p str.
 *
 * If the string @p str is found to be invalid, the result contains a modified, corrected version of the
 * string. Invalid or incomplete UTF-8 byte sequences, as well as invalid code points, are replaced by the
 * replacement character `�` (U+FFFD).
 *
 * @param str the string to validate. The string must remain valid for the lifetime of the returned
 *   #rocket::Cow
 * @param positions if nonnull, then the left index of this map translates `char` offsets from @p str to
 *   `char` offsets in the result for each code point and the end of the string
 * @return a #rocket::Cow result
 */
Cow<std::string_view, std::string>
validate(std::string_view str, UnorderedBimap<u64, u64>* positions = nullptr);

} // namespace utf8

// UTF-32 ...................................................................................................

namespace utf32 {

/**
 * Returns the next code point from the UTF-32 string @p str at the position @p pos.
 *
 * @param str a UTF-32 string
 * @param pos the position to get the next code point from. This must be less than the size of @p str. The
 *   position is updated to the position of the next code point
 * @return the next code point
 */
CodePoint nextCodePoint(std::u32string_view str, u64& pos);

/**
 * Validates the UTF-32 string @p str.
 *
 * If the string @p str is found to be valid, the result contains a reference to the original string @p str.
 *
 * If the string @p str is found to be invalid, the result contains a modified, corrected version of the
 * string. Invalid code points are replaced by the replacement character `�` (U+FFFD).
 *
 * @param str the string to validate. The string must remain valid for the lifetime of the returned
 *   #rocket::Cow
 * @param positions if nonnull, then the left index of this map translates `char32` offsets from @p str to
 *   `char32` offsets in the result for each code point and the end of string (trivial, but provided for
 *   completeness)
 * @return a #rocket::Cow result
 */
Cow<std::u32string_view, std::u32string>
validate(std::u32string_view str, UnorderedBimap<u64, u64>* positions = nullptr);

} // namespace utf32

// Merge functions from #utf8 and #utf32 so they can be used as overloads -----------------------------------

using utf8::nextCodePoint;
using utf32::nextCodePoint;

using utf8::validate;
using utf32::validate;

} // namespace rocket::unicode

// #fmt::formatter<#CodePoint> ------------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::unicode::CodePoint}
 *
 * This formatter uses the same format specifiers as the underlying formatter for type #std::string.
 */
template<typename C>
struct fmt::formatter<rocket::unicode::CodePoint, C> {
  /// @cond undocumented

  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const rocket::unicode::CodePoint& val, FormatContext& ctx) const {
    // This is simple enough that we don't need a #FormattedCodec for it
    if constexpr (std::is_same_v<C, char>) {
      return underlying_.format(fmt::format("U+{:0>4X}", static_cast<u32>(val)), ctx);
    } else {
      return underlying_.format(fmt::format(U"U+{:0>4X}", static_cast<u32>(val)), ctx);
    }
  }

  constexpr const C*
  parse(fmt::parse_context<C>& ctx) {
    return underlying_.parse(ctx);
  }

  constexpr void
  set_debug_format(bool val = true) {
    underlying_.set_debug_format(val);
  }

  /// @endcond

private:

  fmt::formatter<basic_string_view<C>, C> underlying_;
};

// #std::hash<#CodePoint> -----------------------------------------------------------------------------------

/// @spec_std_hash{#rocket::unicode::CodePoint}
template<>
struct std::hash<rocket::unicode::CodePoint> {
  /**
   * Returns a hash value for @p val.
   *
   * @param val the value to hash
   * @return a hash value
   */
  u64
  operator()(rocket::unicode::CodePoint val) const {
    return std::hash<char32>()(static_cast<char32>(val));
  }
};

// #std::numeric_limits<#CodePoint> -------------------------------------------------------------------------

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
   * Returns the maximum code-point value, which is U+10FFFF, or decimal 1,114,111.
   *
   * @return the maximum code-point value
   */
  static consteval rocket::unicode::CodePoint max() { return U'\U0010FFFF'; }
};

// EOF
