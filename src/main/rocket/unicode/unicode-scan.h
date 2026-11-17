/**
 * @file unicode-scan.h
 *
 * Scanning support for the Unicode API.
 *
 * scnlib has no counterpart to the `format_as` customization point of {fmt}, so the scanners are written out
 * here.
 */

#pragma once

#include "rocket/unicode/Iterator.h"
#include "rocket/unicode/Character.h"
#include "rocket/unicode/unicode.h"

#include <scn/scan.h>

#include <cstddef>
#include <exception>
#include <iterator>
#include <string>
#include <string_view>

// `scn::scanner<rocket::unicode::CodePoint>` ---------------------------------------------------------------

/**
 * @spec_scn_scanner{#rocket::unicode::CodePoint}
 *
 * The scanner reads what the formatter writes: `U+` followed by hexadecimal digits, for example `U+00E4` or
 * `U+1F600`. Upper- and lowercase digits are accepted, and whitespace before the `U+` is skipped. The value
 * must be a valid code point, see #rocket::unicode::CodePoint.
 *
 * - No format specifiers are accepted.
 */
template<>
struct scn::scanner<rocket::unicode::CodePoint, char> {
  /// @cond undocumented

  template<typename ParseContext>
  constexpr ParseContext::iterator
  parse(ParseContext& pctx) {
    return pctx.begin();
  }

  template<typename Context>
  scan_expected<typename Context::iterator>
  scan(rocket::unicode::CodePoint& val, Context& ctx) const {
    auto skipped = detail::internal_skip_classic_whitespace(ctx.range(), false);
    if (not skipped) {
      return unexpected(skipped.error());
    }
    auto it = *skipped;
    const auto end = ctx.end();

    for (const char c : { 'U', '+' }) {
      if (it == end) {
        return unexpected(scan_error(scan_error::end_of_input, "expected a code point"));
      }
      if (*it != c) {
        return unexpected(scan_error(scan_error::invalid_scanned_value, "expected a code point"));
      }
      ++it;
    }

    // At most 0x10FFFF; leading zeros are allowed
    rocket::u32 cp = 0;
    bool hasDigits = false;
    for (; it != end; ++it) {
      const int digit = hexDigit(*it);
      if (digit < 0) {
        break;
      }
      hasDigits = true;
      cp = cp * 16 + static_cast<rocket::u32>(digit);
      if (cp > 0x10FFFF) {
        return unexpected(scan_error(scan_error::invalid_scanned_value, "invalid code point"));
      }
    }
    if (not hasDigits) {
      return unexpected(scan_error( it == end ?
        scan_error::end_of_input :
        scan_error::invalid_scanned_value, "expected hexadecimal digits"));
    }
    if (cp >= 0xD800 && cp <= 0xDFFF) {
      return unexpected(scan_error(scan_error::invalid_scanned_value, "invalid code point"));
    }

    val = rocket::unicode::CodePoint(static_cast<rocket::char32>(cp));
    return it;
  }

  /// @endcond

private:

  static constexpr int
  hexDigit(char c) {
    if (c >= '0' && c <= '9') {
      return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
      return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
      return c - 'a' + 10;
    }
    return -1;
  }
};

// `scn::scanner<rocket::unicode::Character>` ---------------------------------------------------------------

/**
 * @spec_scn_scanner{#rocket::unicode::Character}
 *
 * The scanner reads one grapheme cluster, which is what the formatter writes, for example `a`, `€`, or
 * `👨‍👩‍👦`. Whitespace before the character is skipped, so a whitespace character cannot be scanned.
 *
 * Like the enum scanner in `rocket/enum-scan.h`, this scanner uses the same format specifiers as the
 * underlying string scanner. It scans a word and then gives back the characters that follow its first
 * grapheme cluster so that, for example, `ab` scans as `a` and leaves `b` in the input.
 */
template<>
struct scn::scanner<rocket::unicode::Character<char>, char> : scn::scanner<std::string, char> {
  /// @cond undocumented

  template<typename Context>
  scan_expected<typename Context::iterator>
  scan(rocket::unicode::Character<char>& val, Context& ctx) const {
    // This must scan a #std::string, not a #std::string_view, because the latter does not work when scanning
    // from a #std::istream
    std::string str;
    auto result = scn::scanner<std::string, char>::scan(str, ctx);
    if (not result) {
      return unexpected(result.error());
    }

    std::string_view first;
    try {
      auto iter = rocket::unicode::Iterator<char>(rocket::unicode::IteratorType::Character, str);
      first = iter.nextSegment();
    } catch (const std::exception&) {
      return unexpected(scan_error(scan_error::invalid_scanned_value, "invalid character"));
    }
    if (first.empty()) {
      return unexpected(scan_error(scan_error::invalid_scanned_value, "expected a character"));
    }
    val = rocket::unicode::Character<char>(first);

    // Give back what follows the first grapheme cluster. The iterators of scnlib are bidirectional
    auto it = *result;
    std::advance(it, static_cast<std::ptrdiff_t>(first.size()) - static_cast<std::ptrdiff_t>(str.size()));
    return it;
  }

  /// @endcond
};

// EOF
