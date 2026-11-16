/**
 * @file format-int128.h
 *
 * {fmt} formatters for 128-bit integers.
 */

#pragma once

#include "rocket/type-traits.h"

#include <fmt/format.h>

#include <climits>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

#if FMT_USE_LOCALE
  #include <locale>
#endif

#ifndef ROCKET_HAS_FMT_FORMATTER_INT128

namespace rocket::internal::format::int128 {

// `Formatter` ----------------------------------------------------------------------------------------------

/**
 * A {fmt} formatter for a 128-bit integer type that {fmt} does not support natively.
 *
 * The formatter accepts the same format specifications as the {fmt} formatters of built-in integers,
 * reports the same errors, and produces the same output:
 *
 * ```
 * [[fill]align][sign]["#"]["0"][width]["L"][type]
 * ```
 *
 * - `align` is one of `<`, `>`, and `^`; numbers are right-aligned by default
 * - `sign` is one of `+`, `-`, and ` `, and is only allowed for signed types
 * - `#` adds the prefix `0b`, `0B`, `0`, `0x`, or `0X` for the binary, octal, and hexadecimal types
 * - `0` pads with zeros after the sign and the prefix; it is ignored if `align` is given
 * - `width` is a number or a nested replacement field `{}`, `{n}`, or `{name}`
 * - `L` groups the digits according to the locale of the context
 * - `type` is one of `d`, `b`, `B`, `o`, `x`, `X`, and `c`
 *
 * @tparam T the integer type
 * @tparam U the unsigned integer type corresponding to `T`
 * @tparam C the character type
 */
template<typename T, typename U, typename C>
class Formatter {
public:

  /// @cond undocumented

  constexpr const C*
  parse(fmt::parse_context<C>& ctx) {
    auto it = ctx.begin();
    const auto end = ctx.end();
    if (it == end || *it == '}') {
      return it;
    }

    State state = State::start;
    const auto enter = [&state](State next, bool valid = true) {
      if (state >= next || not valid) {
        fmt::report_error("invalid format specifier");
      }
      state = next;
    };

    // If the second character is an alignment, the first one is the fill character
    C c = end - it > 1 && toAlign(it[1]) != Align::none ? C() : *it;
    for (;;) {
      switch (c) {
      case '<':
      case '>':
      case '^':
        enter(State::align);
        align_ = toAlign(c);
        ++it;
        break;
      case '+':
      case ' ':
        sign_ = c == ' ' ? Sign::space : Sign::plus;
        [[fallthrough]];
      case '-':
        enter(State::sign, std::is_signed_v<T> || std::numeric_limits<T>::is_signed);
        ++it;
        break;
      case '#':
        enter(State::hash);
        alt_ = true;
        ++it;
        break;
      case '0':
        enter(State::zero);
        // `0` is ignored if an alignment is given, for compatibility with `std::format`
        if (align_ == Align::none) {
          align_ = Align::numeric;
          fill_[0] = '0';
          fillSize_ = 1;
        }
        ++it;
        break;
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '{':
        enter(State::width);
        it = parseWidth(it, end, ctx);
        break;
      case '.':
        enter(State::precision, false);
        break;
      case 'L':
        enter(State::locale);
        localized_ = true;
        ++it;
        break;
      case 'd':
      case 'b':
      case 'B':
      case 'o':
      case 'x':
      case 'X':
      case 'c':
        type_ = static_cast<char>(c);
        return it + 1;
      case 'e':
      case 'E':
      case 'f':
      case 'F':
      case 'g':
      case 'G':
      case 'a':
      case 'A':
      case 's':
      case 'p':
      case '?':
        fmt::report_error("invalid format specifier");
        return it;
      case '}':
        return it;
      default: {
        if (*it == '}') {
          return it;
        }
        // Fill and alignment
        const auto fillEnd = it + codePointLength(it);
        if (end - fillEnd <= 0) {
          fmt::report_error("invalid format specifier");
          return it;
        }
        if (*it == '{') {
          fmt::report_error("invalid fill character '{'");
          return it;
        }
        const Align align = toAlign(*fillEnd);
        enter(State::align, align != Align::none);
        fillSize_ = 0;
        for (auto p = it; p != fillEnd; ++p) {
          fill_[fillSize_++] = *p;
        }
        align_ = align;
        it = fillEnd + 1;
      }
      }
      if (it == end) {
        return it;
      }
      c = *it;
    }
  }

  template<typename FormatContext>
  FormatContext::iterator
  format(T val, FormatContext& ctx) const {
    const int width = resolveWidth(ctx);

    // Sign and absolute value
    char sign = '\0';
    U abs = static_cast<U>(val);
    if constexpr (not std::is_same_v<T, U>) {
      if (val < T(0)) {
        sign = '-';
        abs = U(0) - abs;
      }
    }
    if (sign == '\0') {
      sign = sign_ == Sign::plus ? '+' : sign_ == Sign::space ? ' ' : '\0';
    }

    auto out = ctx.out();

    if (type_ == 'c') {
      const auto ch = static_cast<C>(static_cast<unsigned long long>(abs));
      return pad(out, width, false, 1, [ch](auto o) {
        *o++ = ch;
        return o;
      });
    }

    // Prefix: sign and base prefix
    char prefix[3];
    u64 prefixSize = 0;
    if (sign != '\0') {
      prefix[prefixSize++] = sign;
    }
    if (alt_) {
      if (type_ == 'x' || type_ == 'X' || type_ == 'b' || type_ == 'B') {
        prefix[prefixSize++] = '0';
        prefix[prefixSize++] = type_;
      } else if (type_ == 'o' && abs != U(0)) {
        prefix[prefixSize++] = '0';
      }
    }

    // Digits, written from the back
    char buf[128];
    char* const bufEnd = buf + sizeof(buf);
    char* digits = bufEnd;
    const char* const chars = type_ == 'X' ? "0123456789ABCDEF" : "0123456789abcdef";
    const int shift =
      type_ == 'x' || type_ == 'X' ? 4 :
      type_ == 'o'                 ? 3 :
      type_ == 'b' || type_ == 'B' ? 1 :
                                     0;
    if (shift == 0) {
      do {
        *--digits = chars[static_cast<unsigned long long>(abs % U(10))];
        abs /= U(10);
      } while (abs != U(0));
    } else {
      const U mask((1u << shift) - 1);
      do {
        *--digits = chars[static_cast<unsigned long long>(abs & mask)];
        abs >>= shift;
      } while (abs != U(0));
    }
    const int digitCount = static_cast<int>(bufEnd - digits);

    const auto writePrefix = [&](auto o) {
      for (u64 i = 0; i < prefixSize; ++i) {
        *o++ = static_cast<C>(prefix[i]);
      }
      return o;
    };

    // Localized output, which {fmt} only supports for `char`. Like {fmt}, the digits of all types are
    // grouped, and the zero padding of `0` precedes the sign and the prefix
#if FMT_USE_LOCALE
    if constexpr (std::is_same_v<C, char>) {
      if (localized_) {
        const std::locale locale = ctx.locale().template get<std::locale>();
        const auto& numpunct = std::use_facet<std::numpunct<char>>(locale);
        const std::string grouping = numpunct.grouping();
        const char sep = grouping.empty() ? '\0' : numpunct.thousands_sep();

        // Positions of the separators, counted from the back; the first entry is a sentinel
        int seps[sizeof(buf) + 1] = { 0 };
        int sepCount = 0;
        if (sep != '\0') {
          u64 group = 0;
          int pos = 0;
          for (;;) {
            char size = group < grouping.size() ? grouping[group++] : grouping.back();
            if (size <= 0 || size == CHAR_MAX) {
              break;
            }
            pos += size;
            if (pos >= digitCount) {
              break;
            }
            seps[++sepCount] = pos;
          }
        }

        return pad(out, width, true, prefixSize + static_cast<u64>(digitCount + sepCount), [&](auto o) {
          o = writePrefix(o);
          for (int i = 0, sepIndex = sepCount; i < digitCount; ++i) {
            if (digitCount - i == seps[sepIndex]) {
              *o++ = sep;
              --sepIndex;
            }
            *o++ = digits[i];
          }
          return o;
        });
      }
    }
#endif

    // The zero padding of `0` goes between the prefix and the digits
    u64 size = prefixSize + static_cast<u64>(digitCount);
    u64 zeros = 0;
    if (align_ == Align::numeric && static_cast<u64>(width) > size) {
      zeros = static_cast<u64>(width) - size;
      size = static_cast<u64>(width);
    }
    return pad(out, width, true, size, [&](auto o) {
      o = writePrefix(o);
      for (u64 i = 0; i < zeros; ++i) {
        *o++ = static_cast<C>('0');
      }
      for (const char* p = digits; p != bufEnd; ++p) {
        *o++ = static_cast<C>(*p);
      }
      return o;
    });
  }

  /// @endcond

private:

  enum class Align : u8 { none, left, right, center, numeric };
  enum class Sign : u8 { none, plus, space };
  enum class State : u8 { start, align, sign, hash, zero, width, precision, locale };
  enum class WidthKind : u8 { none, index, name };

  /// Converts a width argument to an integer, as {fmt} does.
  struct WidthGetter {
    template<typename V>
    constexpr unsigned long long
    operator()(V val) const {
      if constexpr (
        std::is_integral_v<V> &&
        not std::is_same_v<V, bool> &&
        not std::is_same_v<V, char> &&
        not std::is_same_v<V, wchar_t>) {
        if constexpr (std::is_signed_v<V>) {
          if (val < 0) {
            return ~0ull;
          }
        }
        return static_cast<unsigned long long>(val);
      } else {
        fmt::report_error("width/precision is not integer");
        return 0;
      }
    }
  };

  C fill_[4] = { ' ' };
  u8 fillSize_ = 1;
  Align align_ = Align::none;
  Sign sign_ = Sign::none;
  bool alt_ = false;
  bool localized_ = false;
  char type_ = 'd';
  int width_ = 0;
  WidthKind widthKind_ = WidthKind::none;
  int widthIndex_ = 0;
  std::basic_string_view<C> widthName_;

  static constexpr Align
  toAlign(C c) {
    switch (c) {
    case '<':
      return Align::left;
    case '>':
      return Align::right;
    case '^':
      return Align::center;
    default:
      return Align::none;
    }
  }

  static constexpr int
  codePointLength(const C* p) {
    if constexpr (sizeof(C) != 1) {
      return 1;
    } else {
      const auto c = static_cast<unsigned char>(*p);
      return static_cast<int>((0x3a55000000000000ull >> (2 * (c >> 3))) & 3) + 1;
    }
  }

  static constexpr bool
  isDigit(C c) {
    return '0' <= c && c <= '9';
  }

  static constexpr bool
  isNameStart(C c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
  }

  /// Parses a nonnegative integer, returning `errorValue` on overflow.
  static constexpr int
  parseNonnegativeInt(const C*& it, const C* end, int errorValue) {
    unsigned long long val = 0;
    do {
      val = val * 10 + static_cast<unsigned>(*it - '0');
      if (val > INT_MAX) {
        val = static_cast<unsigned long long>(INT_MAX) + 1;
      }
      ++it;
    } while (it != end && isDigit(*it));
    return val > INT_MAX ? errorValue : static_cast<int>(val);
  }

  /// Parses `integer | "{" [arg-id] "}"`.
  constexpr const C*
  parseWidth(const C* it, const C* end, fmt::parse_context<C>& ctx) {
    if (isDigit(*it)) {
      const int val = parseNonnegativeInt(it, end, -1);
      if (val == -1) {
        fmt::report_error("number is too big");
      }
      width_ = val;
      return it;
    }

    ++it; // `{`
    if (it != end) {
      const C c = *it;
      if (c == '}' || c == ':') {
        widthKind_ = WidthKind::index;
        widthIndex_ = ctx.next_arg_id();
        ctx.check_dynamic_spec(widthIndex_);
      } else if (isDigit(c)) {
        int index = 0;
        if (c != '0') {
          index = parseNonnegativeInt(it, end, INT_MAX);
        } else {
          ++it;
        }
        if (it == end || (*it != '}' && *it != ':')) {
          fmt::report_error("invalid format string");
        }
        widthKind_ = WidthKind::index;
        widthIndex_ = index;
        ctx.check_arg_id(index);
        ctx.check_dynamic_spec(index);
      } else if (isNameStart(c)) {
        const auto begin = it;
        do {
          ++it;
        } while (it != end && (isNameStart(*it) || isDigit(*it)));
        widthKind_ = WidthKind::name;
        widthName_ = std::basic_string_view<C>(begin, static_cast<u64>(it - begin));
        ctx.check_arg_id(widthName_);
      } else {
        fmt::report_error("invalid format string");
      }
    }
    if (it != end && *it == '}') {
      return ++it;
    }
    fmt::report_error("invalid format string");
    return it;
  }

  template<typename FormatContext>
  int
  resolveWidth(FormatContext& ctx) const {
    if (widthKind_ == WidthKind::none) {
      return width_;
    }
    const auto arg = widthKind_ == WidthKind::index ? ctx.arg(widthIndex_) : ctx.arg(widthName_);
    if (not arg) {
      fmt::report_error("argument not found");
    }
    const unsigned long long val = arg.visit(WidthGetter());
    if (val > INT_MAX) {
      fmt::report_error("width/precision is out of range");
    }
    return static_cast<int>(val);
  }

  /// Writes `size` characters with `write`, padded to `width` according to the fill and the alignment.
  template<typename Out, typename Write>
  Out
  pad(Out out, int width, bool rightByDefault, u64 size, Write write) const {
    const u64 padding = static_cast<u64>(width) > size ? static_cast<u64>(width) - size : 0;
    u64 left = 0;
    switch (align_) {
    case Align::none:
      left = rightByDefault ? padding : 0;
      break;
    case Align::left:
      left = 0;
      break;
    case Align::center:
      left = padding / 2;
      break;
    case Align::right:
    case Align::numeric:
      left = padding;
      break;
    }
    const auto writeFill = [this](Out o, u64 n) {
      for (u64 i = 0; i < n; ++i) {
        for (u8 j = 0; j < fillSize_; ++j) {
          *o++ = fill_[j];
        }
      }
      return o;
    };
    out = writeFill(out, left);
    out = write(out);
    return writeFill(out, padding - left);
  }
};

} // namespace rocket::internal::format::int128

// `fmt::formatter<rocket::i128>` ---------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{rocket::i128}
 *
 * This formatter supports the same format specifications as the {fmt} formatters of built-in integers.
 */
template<typename C> requires rocket::IsChar<C>
struct fmt::formatter<rocket::i128, C> :
  rocket::internal::format::int128::Formatter<rocket::i128, rocket::u128, C> {};

// `fmt::formatter<rocket::u128>` ---------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{rocket::u128}
 *
 * This formatter supports the same format specifications as the {fmt} formatters of built-in integers.
 */
template<typename C> requires rocket::IsChar<C>
struct fmt::formatter<rocket::u128, C> :
  rocket::internal::format::int128::Formatter<rocket::u128, rocket::u128, C> {};

#endif // ROCKET_HAS_FMT_FORMATTER_INT128

// EOF
