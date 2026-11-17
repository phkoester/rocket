/**
 * @file int128-scan.h
 *
 * scnlib scanners for 128-bit integers.
 */

#pragma once

#include "rocket/type-traits.h"

#include <scn/scan.h>

#include <cstddef>
#include <limits>
#include <locale>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>

#ifndef ROCKET_HAS_SCN_SCANNER_INT128

namespace rocket::internal::scan::int128 {

// `Scanner` ------------------------------------------------------------------------------------------------

/**
 * A scnlib scanner for a 128-bit integer type that scnlib does not support natively.
 *
 * The scanner accepts the same format specifications as the scnlib scanners of built-in integers, reports
 * the same errors, and consumes the same input. It mirrors the integer reader of scnlib, including fill,
 * alignment, width (the minimum field width), precision (the maximum field width), base prefixes, and
 * localized digit grouping.
 *
 * @tparam T the integer type
 * @tparam U the unsigned integer type corresponding to `T`
 * @tparam C the character type
 */
template<typename T, typename U, typename C>
struct Scanner {
  /// @cond undocumented

  template<typename ParseCtx>
  constexpr ParseCtx::iterator
  parse(ParseCtx& pctx) {
    auto begin = pctx.begin();
    const auto end = pctx.end();
    // scnlib passes an empty specification for `{}`, which is read by default, unlike `{:}`
    if (begin == end) {
      return begin;
    }
    hasSpecs_ = true;
    if (*begin == '}') {
      return begin;
    }

    // As in `scn::detail::scanner_parse_for_builtin_type`
    using Setter = scn::detail::specs_setter;
    constexpr auto type = isSigned ? scn::detail::arg_type::int128_type : scn::detail::arg_type::uint128_type;
    auto checker = scn::detail::specs_checker<Setter>(Setter(specs_), type);
    const auto it =
      scn::detail::parse_format_specs(scn::detail::to_address(begin), scn::detail::to_address(end), checker);
    scn::detail::check_int_type_specs(specs_, checker);
    if (const auto result = checker.get_error(); not result) {
      pctx.on_error(result.error().msg());
    }
    return it;
  }

  template<typename Context>
  scn::scan_expected<typename Context::iterator>
  scan(T& val, Context& ctx) const {
    using It = Context::iterator;
    using Sent = Context::sentinel;
    using Cur = Cursor<It, Sent>;

    bool contiguous = false;
    if constexpr (std::ranges::contiguous_range<typename Context::range_type>) {
      contiguous = true;
    } else {
      const auto begin = ctx.begin();
      contiguous = not begin.stores_parent() || begin.parent()->is_contiguous();
    }

    // Default format, as in `scn::impl::default_arg_reader`
    if (not hasSpecs_) {
      Cur cur(ctx.begin(), ctx.end());
      if (cur.atEnd()) {
        return eofError();
      }
      skipWhitespace(cur, true);
      if (cur.atEnd()) {
        return eofError();
      }
      auto result = readInt(cur, val, ctx, contiguous);
      if (not result) {
        return scn::unexpected(result.error());
      }
      return result->it;
    }

    // Format specifications, as in `scn::impl::arg_reader`
    const scn::detail::format_specs& specs = specs_;
    const bool needWidth = specs.width != 0 || specs.precision != 0;
    // A contiguous source is read like a contiguous range, unless the precision limits the field width
    const bool sourceContiguous = contiguous;
    contiguous = contiguous && specs.precision == 0;

    // Prefix
    Cur cur(ctx.begin(), ctx.end());
    if (specs.precision != 0) {
      cur.limit(specs.precision);
    }
    std::ptrdiff_t prefixWidth = 0;
    if (specs.align == scn::detail::align_type::right || specs.align == scn::detail::align_type::center) {
      prefixWidth = skipFill(cur, specs.precision, needWidth);
    } else if (specs.align == scn::detail::align_type::none) {
      if (specs.precision != 0) {
        skipWhitespace(cur, false);
        if (cur.atEnd()) {
          return eofError();
        }
        prefixWidth = specs.precision - cur.count;
      } else {
        const It begin = cur.it;
        skipWhitespace(cur, sourceContiguous);
        if (cur.atEnd()) {
          return eofError();
        }
        if (needWidth) {
          prefixWidth = textWidth(begin, cur.it);
        }
      }
    }
    const It prefixEnd = cur.it;

    // Value
    It it;
    std::ptrdiff_t valueWidth = 0;
    if (specs.precision != 0) {
      if (specs.precision <= prefixWidth) {
        return scn::unexpected(scn::scan_error(
          scn::scan_error::invalid_fill,
          "Too many fill characters before value, precision exceeded before reading value"));
      }
      const std::ptrdiff_t initialWidth = specs.precision - prefixWidth;
      Cur valueCur(prefixEnd, ctx.end());
      valueCur.limit(initialWidth);
      auto result = readInt(valueCur, val, ctx, false);
      if (not result) {
        return scn::unexpected(result.error());
      }
      it = result->it;
      valueWidth = initialWidth - result->count;
    } else {
      auto result = readInt(Cur(prefixEnd, ctx.end()), val, ctx, contiguous);
      if (not result) {
        return scn::unexpected(result.error());
      }
      it = result->it;
      if (needWidth) {
        valueWidth = textWidth(prefixEnd, it);
      }
    }

    // Postfix
    std::ptrdiff_t postfixWidth = 0;
    if (it != ctx.end() &&
        (specs.align == scn::detail::align_type::left || specs.align == scn::detail::align_type::center) &&
        not(specs.precision != 0 && specs.precision - valueWidth - prefixWidth == 0)) {
      Cur postfixCur(it, ctx.end());
      postfixWidth = skipFill(postfixCur, specs.precision - valueWidth - prefixWidth, needWidth);
      it = postfixCur.it;
    }

    if (specs.width != 0 && prefixWidth + valueWidth + postfixWidth < specs.width) {
      return scn::unexpected(scn::scan_error(
        scn::scan_error::length_too_short,
        "Scanned value too narrow, width did not exceed what was specified in the format string"));
    }
    return it;
  }

  /// @endcond

private:

  static constexpr bool isSigned = not std::is_same_v<T, U>;

  enum class Sign : u8 { minus, plus };

  /**
   * A position in the input, optionally limited to a field width, as `scn::impl::counted_width_iterator`.
   *
   * Like scnlib, a code point counts with its display width, and a negative limit is never reached.
   */
  template<typename It, typename Sent>
  struct Cursor {
    It it;
    Sent end;
    std::ptrdiff_t count = 0;
    std::ptrdiff_t multibyteLeft = 0;
    bool limited = false;

    Cursor(It it, Sent end) :
      it(it),
      end(end) {}

    void
    limit(std::ptrdiff_t width) {
      count = width;
      multibyteLeft = 0;
      limited = true;
    }

    bool
    atEnd() const {
      if (it == end) {
        return true;
      }
      if (not limited || multibyteLeft != 0) {
        return false;
      }
      return count == 0 || (count == 1 && widthAt(it, end, codePointLength(*it)) == 2);
    }

    void
    advance() {
      if (limited) {
        if (multibyteLeft == 0) {
          const auto length = codePointLength(*it);
          multibyteLeft = length - 1;
          count -= widthAt(it, end, length);
        } else {
          --multibyteLeft;
        }
      }
      ++it;
    }
  };

  template<typename It>
  struct ReadResult {
    It it;
    std::ptrdiff_t count;
  };

  scn::detail::format_specs specs_;
  bool hasSpecs_ = false;

  static scn::unexpected<scn::scan_error>
  eofError() {
    return scn::unexpected(scn::scan_error(scn::scan_error::end_of_input, "EOF"));
  }

  static scn::unexpected<scn::scan_error>
  invalidValue(const char* msg) {
    return scn::unexpected(scn::scan_error(scn::scan_error::invalid_scanned_value, msg));
  }

  static constexpr unsigned
  charToInt(C c) {
    const auto u = static_cast<std::make_unsigned_t<C>>(c);
    if (u >= '0' && u <= '9') {
      return u - '0';
    }
    if (u >= 'a' && u <= 'z') {
      return u - 'a' + 10;
    }
    if (u >= 'A' && u <= 'Z') {
      return u - 'A' + 10;
    }
    return 255;
  }

  static constexpr std::ptrdiff_t
  codePointLength(C c) {
    return static_cast<std::ptrdiff_t>(scn::detail::code_point_length_by_starting_code_unit(c));
  }

  /// The display width of a code point, as `scn::impl::calculate_text_width_for_fmt_v10`.
  static constexpr std::ptrdiff_t
  cpWidth(char32_t cp) {
    if (cp >= 0x1100 &&
        (cp <= 0x115f ||
         cp == 0x2329 ||
         cp == 0x232a ||
         (cp >= 0x2e80 && cp <= 0xa4cf && cp != 0x303f) ||
         (cp >= 0xac00 && cp <= 0xd7a3) ||
         (cp >= 0xf900 && cp <= 0xfaff) ||
         (cp >= 0xfe10 && cp <= 0xfe19) ||
         (cp >= 0xfe30 && cp <= 0xfe6f) ||
         (cp >= 0xff00 && cp <= 0xff60) ||
         (cp >= 0xffe0 && cp <= 0xffe6) ||
         (cp >= 0x20000 && cp <= 0x2fffd) ||
         (cp >= 0x30000 && cp <= 0x3fffd) ||
         (cp >= 0x1f300 && cp <= 0x1f64f) ||
         (cp >= 0x1f900 && cp <= 0x1f9ff))) {
      return 2;
    }
    return 1;
  }

  /// Decodes the `length` code units at `it`, returning `scn::detail::invalid_code_point` if impossible.
  template<typename It, typename Sent>
  static char32_t
  decode(It it, Sent end, std::ptrdiff_t length) {
    C buf[4];
    for (std::ptrdiff_t i = 0; i < length; ++i, ++it) {
      if (it == end) {
        return scn::detail::invalid_code_point;
      }
      buf[i] = *it;
    }
    const std::basic_string_view<C> str(buf, static_cast<u64>(length));
    return scn::detail::decode_code_point_exhaustive(str);
  }

  /// The display width of the code point starting at `it`, as `counted_width_iterator`.
  template<typename It, typename Sent>
  static std::ptrdiff_t
  widthAt(It it, Sent end, std::ptrdiff_t length) {
    if (length == 0) {
      return 0;
    }
    if (length == 1) {
      return cpWidth(static_cast<char32_t>(static_cast<std::make_unsigned_t<C>>(*it)));
    }
    for (auto p = it; length > 0; --length, ++p) {
      if (p == end) {
        return 0;
      }
    }
    return cpWidth(decode(it, end, codePointLength(*it)));
  }

  /// The display width of `[begin, end)`, as `scn::impl::calculate_text_width`.
  template<typename It>
  static std::ptrdiff_t
  textWidth(It begin, It end) {
    std::ptrdiff_t width = 0;
    while (begin != end) {
      std::ptrdiff_t length = codePointLength(*begin);
      if (length == 0) {
        length = 1;
      }
      const char32_t cp = decode(begin, end, length);
      width += cp == scn::detail::invalid_code_point ? 1 : cpWidth(cp);
      for (; length > 0 && begin != end; --length) {
        ++begin;
      }
    }
    return width;
  }

  /**
   * Skips whitespace, as `scn::impl::read_while_classic_space`.
   *
   * scnlib reads narrow whitespace either with a fast algorithm that ends at invalid code units, or code
   * point by code point, skipping invalid code units; `fast` selects the former.
   */
  template<typename Cur>
  static void
  skipWhitespace(Cur& cur, bool fast) {
    const bool stopAtInvalid = std::is_same_v<C, char> && fast && not cur.limited;
    while (not cur.atEnd()) {
      std::ptrdiff_t length = codePointLength(*cur.it);
      const char32_t cp = length == 0 ? scn::detail::invalid_code_point : decode(cur.it, cur.end, length);
      if (cp == scn::detail::invalid_code_point) {
        if (stopAtInvalid) {
          return;
        }
        length = length == 0 ? 1 : length;
      } else if (not scn::detail::is_cp_space(cp)) {
        return;
      }
      for (std::ptrdiff_t i = 0; i < length && not cur.atEnd(); ++i) {
        cur.advance();
      }
    }
  }

  /// Skips fill characters and returns their width, as `scn::impl::skip_fill`.
  template<typename Cur>
  std::ptrdiff_t
  skipFill(Cur& cur, std::ptrdiff_t maxWidth, bool wantWidth) const {
    const auto& fill = specs_.fill;
    if (maxWidth != 0) {
      cur.limit(maxWidth);
    }
    std::ptrdiff_t fillCount = 0;
    std::ptrdiff_t fillWidth = 0;

    if (fill.size() <= sizeof(C)) {
      const C fillCh = fill.template get_code_unit<C>();
      while (not cur.atEnd() && *cur.it == fillCh) {
        cur.advance();
        ++fillCount;
      }
      fillWidth = cpWidth(static_cast<char32_t>(static_cast<std::make_unsigned_t<C>>(fillCh)));
    } else {
      const auto fillChars = fill.template get_code_units<C>();
      for (;;) {
        Cur next = cur;
        bool match = true;
        for (const C c : fillChars) {
          if (next.atEnd() || *next.it != c) {
            match = false;
            break;
          }
          next.advance();
        }
        if (not match) {
          break;
        }
        cur = next;
        ++fillCount;
      }
      fillWidth = textWidth(fillChars.begin(), fillChars.end());
    }

    if (not wantWidth) {
      return 0;
    }
    if (maxWidth == 0) {
      return fillWidth * fillCount;
    }
    return maxWidth - cur.count;
  }

  /// Matches `str` case-insensitively, as `scn::impl::read_matching_string_classic_nocase`.
  template<typename Cur>
  static bool
  matchNoCase(Cur& cur, std::string_view str, bool contiguous) {
    Cur next = cur;
    unsigned char diff = 0;
    for (const char expected : str) {
      if (next.atEnd()) {
        return false;
      }
      const C c = *next.it;
      if (contiguous && std::is_same_v<C, char>) {
        const auto u = static_cast<unsigned char>(c);
        diff |= static_cast<unsigned char>(u ^ static_cast<unsigned char>(expected));
      } else {
        const C lower = c >= 'A' && c <= 'Z' ? static_cast<C>(c + ('a' - 'A')) : c;
        if (lower != static_cast<C>(expected)) {
          return false;
        }
      }
      next.advance();
    }
    if (diff != 0 && diff != 32) {
      return false;
    }
    cur = next;
    return true;
  }

  /// Reads an integer, as `scn::impl::reader_impl_for_int::read_specs`.
  template<typename Cur, typename Context>
  scn::scan_expected<ReadResult<decltype(std::declval<Cur>().it)>>
  readInt(Cur cur, T& val, Context& ctx, bool contiguous) const {
    using It = decltype(cur.it);
    const auto type = hasSpecs_ ? specs_.type : scn::detail::presentation_type::none;
    const int base = hasSpecs_ ? specs_.get_base() : 10;

    // Sign
    if (cur.atEnd()) {
      return eofError();
    }
    Sign sign = Sign::plus;
    if (*cur.it == '-' || *cur.it == '+') {
      sign = *cur.it == '-' ? Sign::minus : Sign::plus;
      cur.advance();
    }

    // Base prefix
    const Cur basePrefixBegin = cur;
    int parsedBase = base;
    bool parsedZero = false;
    const auto matchZero = [&cur] {
      if (cur.atEnd() || *cur.it != '0') {
        return false;
      }
      cur.advance();
      return true;
    };
    switch (base) {
    case 2:
      matchNoCase(cur, "0b", contiguous);
      break;
    case 8:
      if (not matchNoCase(cur, "0o", contiguous)) {
        parsedZero = matchZero();
      }
      break;
    case 16:
      matchNoCase(cur, "0x", contiguous);
      break;
    case 0:
      if (matchNoCase(cur, "0x", contiguous)) {
        parsedBase = 16;
      } else if (matchNoCase(cur, "0b", contiguous)) {
        parsedBase = 2;
      } else if (matchNoCase(cur, "0o", contiguous)) {
        parsedBase = 8;
      } else if (matchZero()) {
        parsedBase = 8;
        parsedZero = true;
      } else {
        parsedBase = 10;
      }
      break;
    default:
      break;
    }
    if (parsedZero) {
      if (cur.atEnd() || charToInt(*cur.it) >= 8) {
        cur = basePrefixBegin;
      } else {
        parsedZero = false;
      }
    } else if (cur.atEnd() || charToInt(*cur.it) >= static_cast<unsigned>(parsedBase)) {
      cur = basePrefixBegin;
    }

    if (sign == Sign::minus) {
      if constexpr (not isSigned) {
        return invalidValue("Unexpected '-' sign when parsing an unsigned value");
      } else if (type == scn::detail::presentation_type::int_unsigned_decimal) {
        return invalidValue("'u'-option disallows negative values");
      }
    }

    if (parsedZero) {
      val = T(0);
      cur.advance();
      return ReadResult<It> { cur.it, cur.count };
    }

    const auto digitsBegin = cur;
    const bool negative = sign == Sign::minus;

    // Localized, with thousands separators
    if (hasSpecs_ && specs_.localized) {
      std::string grouping;
      C sep = C();
#if !SCN_DISABLE_LOCALE
      if constexpr (std::is_same_v<C, char> || std::is_same_v<C, wchar_t>) {
        const auto locale = ctx.locale().template get<std::locale>();
        if (std::has_facet<std::numpunct<C>>(locale)) {
          const auto& numpunct = std::use_facet<std::numpunct<C>>(locale);
          grouping = numpunct.grouping();
          sep = grouping.empty() ? C() : numpunct.thousands_sep();
        }
      }
#endif
      std::basic_string<C> digits;
      std::ptrdiff_t sepCount = 0;
      while (not cur.atEnd()) {
        const C c = *cur.it;
        if (c == sep) {
          ++sepCount;
        } else if (charToInt(c) >= static_cast<unsigned>(parsedBase)) {
          break;
        } else {
          digits.push_back(c);
        }
        cur.advance();
      }
      if (digits.empty()) {
        return invalidValue("Failed to parse integer: No digits found");
      }
      auto parsed = parseValue(digits.begin(), digits.end(), val, negative, parsedBase);
      if (not parsed) {
        return scn::unexpected(parsed.error());
      }
      cur = digitsBegin;
      for (std::ptrdiff_t i = 0; i < *parsed + sepCount; ++i) {
        cur.advance();
      }
      return ReadResult<It> { cur.it, cur.count };
    }

    (void) ctx;

    if (contiguous) {
      // All of the remaining input is passed on
      if (cur.atEnd()) {
        return invalidValue("Failed to parse integer: No digits found");
      }
      if (charToInt(*cur.it) >= static_cast<unsigned>(parsedBase)) {
        return invalidValue("Invalid integer value");
      }
    } else {
      if (cur.atEnd() || charToInt(*cur.it) >= static_cast<unsigned>(parsedBase)) {
        return invalidValue("Failed to parse integer: No digits found");
      }
    }

    // Collect the digits
    std::basic_string<C> digits;
    for (Cur p = cur; not p.atEnd() && charToInt(*p.it) < static_cast<unsigned>(parsedBase); p.advance()) {
      digits.push_back(*p.it);
    }
    auto parsed = parseValue(digits.begin(), digits.end(), val, negative, parsedBase);
    if (not parsed) {
      return scn::unexpected(parsed.error());
    }
    for (std::ptrdiff_t i = 0; i < *parsed; ++i) {
      cur.advance();
    }
    return ReadResult<It> { cur.it, cur.count };
  }

  /**
   * Parses digits, skipping leading zeros and checking for overflow, as `scn::impl::parse_integer_value`.
   *
   * @return the number of code units consumed
   */
  template<typename DigitIt>
  static scn::scan_expected<std::ptrdiff_t>
  parseValue(DigitIt begin, DigitIt end, T& val, bool negative, int base) {
    auto p = begin;
    while (p != end && *p == '0') {
      ++p;
    }
    if (p == end) {
      val = T(0);
      return static_cast<std::ptrdiff_t>(p - begin);
    }

    const U uintMax = std::numeric_limits<U>::max();
    const U intMax = uintMax >> 1;
    const U absMin = intMax + U(1);
    const U max = not isSigned ? uintMax : negative ? absMin : intMax;
    const U b(static_cast<unsigned>(base));
    const U limit = max / b;
    const U maxDigit = max % b;

    U acc(0);
    for (; p != end; ++p) {
      const U digit(charToInt(*p));
      if (acc < limit || (acc == limit && digit <= maxDigit)) {
        acc = acc * b + digit;
      } else {
        return scn::unexpected(scn::scan_error(
          negative ? scn::scan_error::value_negative_overflow : scn::scan_error::value_positive_overflow,
          "Integer overflow"));
      }
    }
    val = static_cast<T>(negative ? U(0) - acc : acc);
    return static_cast<std::ptrdiff_t>(p - begin);
  }
};

} // namespace rocket::internal::scan::int128

// `scn::scanner<rocket::i128>` -----------------------------------------------------------------------------

/**
 * @spec_scn_scanner{#rocket::i128}
 *
 * This scanner supports the same format specifications as the scnlib scanners of built-in integers.
 */
template<typename C> requires rocket::IsChar<C>
struct scn::scanner<rocket::i128, C> :
  rocket::internal::scan::int128::Scanner<rocket::i128, rocket::u128, C> {};

// `scn::scanner<rocket::u128>` -----------------------------------------------------------------------------

/**
 * @spec_scn_scanner{#rocket::u128}
 *
 * This scanner supports the same format specifications as the scnlib scanners of built-in integers.
 */
template<typename C> requires rocket::IsChar<C>
struct scn::scanner<rocket::u128, C> :
  rocket::internal::scan::int128::Scanner<rocket::u128, rocket::u128, C> {};

#endif // ROCKET_HAS_SCN_SCANNER_INT128

// EOF
