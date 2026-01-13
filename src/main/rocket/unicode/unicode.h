/**
 * @file unicode.h
 *
 * A Unicode API.
 */

#pragma once

#include "rocket/Cow.h"
#include "rocket/UnorderedBimap.h"
#include "rocket/format/format.h"

#include <limits>

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
   * U+D800–U+DFFF.
   *
   * @return `true` if this code point is valid
   */
  bool valid() const { return v_ <= 0x10FFFFU && not (v_ >= 0xD800U && v_ <= 0xDFFFU); }

  /**
   * Calculates the display width for a code point.
   *
   * @return the code point's display width, in the range @f$[0,2]@f$
   */
  uint8_t width() const;

private:

  char32_t v_; ///< The code-point value.
};

/// @op_output{#rocket::unicode::CodePoint}
std::ostream& operator<<(std::ostream& lhs, CodePoint rhs);

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

// UTF8 .....................................................................................................

namespace utf8 {

CodePoint nextCodePoint(std::string_view s, size_t& pos);

/**
 * Validates the UTF-8 string @p s.
 *
 * If the string @p s is found to be valid, the result contains a reference to the original string @p s.
 *
 * If the string @p s is found to be invalid, the result contains a modified, corrected version of the
 * string. Invalid or incomplete UTF-8 byte sequences, as well as invalid code points, are replaced by the
 * replacement character `�` (U+FFFD).
 *
 * @param s the string to validate. The string must remain valid for the lifetime of the returned
 *    #rocket::Cow
 * @param positions if nonnull, then the left index of this map translates `char` offsets from @p s to `char`
 *   offsets in the result for each code point and the end of the string
 * @return a #rocket::Cow result
 */
Cow<std::string_view, std::string>
validate(std::string_view s, UnorderedBimap<size_t, size_t>* positions = nullptr);

} // namespace utf8

// UTF-32 ...................................................................................................

namespace utf32 {

CodePoint nextCodePoint(std::u32string_view s, size_t& pos);

/**
 * Validates the UTF-32 string @p s.
 *
 * If the string @p s is found to be valid, the result contains a reference to the original string @p s.
 *
 * If the string @p s is found to be invalid, the result contains a modified, corrected version of the
 * string. Invalid code points are replaced by the replacement character `�` (U+FFFD).
 *
 * @param s the string to validate. The string must remain valid for the lifetime of the returned
 *     #rocket::Cow
 * @param positions if nonnull, then the left index of this map translates `char32_t` offsets from @p s to
 *   char32_t` offsets in the result for each code point and the end of string (trivial, but provided for
 *   completeness)
 * @return a #rocket::Cow result
 */
Cow<std::u32string_view, std::u32string>
validate(std::u32string_view s, UnorderedBimap<size_t, size_t>* positions = nullptr);

} // namespace utf32

// Merge functions from `utf8` and `utf32` so they can be used as overloads ---------------------------------

using utf8::nextCodePoint;
using utf32::nextCodePoint;

using utf8::validate;
using utf32::validate;

} // namespace rocket::unicode

// EOF
