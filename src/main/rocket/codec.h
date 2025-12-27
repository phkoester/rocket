/**
 * @file codec.h
 *
 * Declarations for encoding/decoding C++ objects.
 *
 * @attention This file must be included **after** all codec-related declarations and **before** all
 * codec-related definitions.
 *
 * | What?                         | How?
 * | :---------------------------- | :---
 * | Parse RON from `std::istream` | Supply a `parseRon` overload for the type
 * | Decode from RON string        | Use #rocket::codec::ron::parse
 */

#pragma once

// Nothing with a codec-related function overload may be included here!

#include "io.h"

/// @cond undocumented
#define ROCKET_CODEC_H
/// @endcond

#include <cstdlib>
#include <iomanip>
#include <limits>
#include <spanstream>
#include <utility>
#include <variant>

namespace rocket::codec {

// `Symbols` ------------------------------------------------------------------------------------------------

/**
 * Predefined symbols for parsing and printing.
 */
struct Symbols {
  /// Character sets.
  struct Chars {
    /**
     * Digits: <code>'0'</code> to <code>'9'</code>.
     */
    static const std::set<char> Digits;
    /**
     * Digits and apostrophe: <code>'0'</code> to <code>'9'</code>, <code>'\''</code>.
     */
    static const std::set<char> DigitsApostrophe;
    /**
     * E: <code>'e'</code>, <code>'E'</code>.
     */
    static const std::set<char> E;
    /**
     * Plus, minus: <code>'+'</code>, <code>'-'</code>.
     */
    static const std::set<char> PlusMinus;
  };

  /// Strings.
  struct String {
    /// String constant.
    static constexpr std::string_view EmptySet = "{}";
    /// String constant.
    static constexpr std::string_view EmptySetSymbol = "∅";
    /// String constant.
    static constexpr std::string_view False = "false";
    /// String constant.
    static constexpr std::string_view Infinity = "inf";
    /// String constant.
    static constexpr std::string_view InfinitySymbol = "∞";
    /// String constant.
    static constexpr std::string_view Nan = "nan";
    /// String constant.
    static constexpr std::string_view NegativeInfinity = "-inf";
    /// String constant.
    static constexpr std::string_view NegativeInfinitySymbol = "-∞";
    /// String constant.
    static constexpr std::string_view One = "1";
    /// String constant.
    static constexpr std::string_view PositiveInfinity = "+inf";
    /// String constant.
    static constexpr std::string_view PositiveInfinitySymbol = "+∞";
    /// String constant.
    static constexpr std::string_view Qnan = "qnan";
    /// String constant.
    static constexpr std::string_view Snan = "snan";
    /// String constant.
    static constexpr std::string_view True = "true";
    /// String constant.
    static constexpr std::string_view Zero = "0";
  };

  /// String sets.
  struct Strings {
    /**
     * Boolean values: `"0"`, `"1"`, `"false"`, `"true"`.
     */
    static const std::set<std::string_view> Bool;
    /**
     * Empty-set values: `"{}"`, `"∅"`.
     */
    static const std::set<std::string_view> EmptySet;
    /**
     * Floating-point values: `"-inf"`, `"inf"`, `"+inf"`, `"-∞"`, `"∞"`, `"+∞"`, `"nan"`, `"qnan"`,
     * `"snan"`.
     */
    static const std::set<std::string_view> FloatingPoint;
    /**
     * Infinity values: `"inf"`, `"+inf"`, `"∞"`, `"+∞"`.
     */
    static const std::set<std::string_view> Infinity;
    /**
     * Negative-infinity values: `"-inf"`, `"-∞"`.
     */
    static const std::set<std::string_view> NegativeInfinity;
  };
};

// `std::istream` utilities ---------------------------------------------------------------------------------

/**
 * Reads a boolean value from the input stream @p is.
 *
 * Boolean values must conform to the following grammar:
 *
 * ```
 * Boolean = "0" | "false" | "1" | "true"
 * ```
 *
 * @param is the input stream
 * @return a boolean value
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::io::ParseFailure if `is.eof()` returns `true` or if the input cannot be parsed as a
 *     boolean value
 */
bool getBool(std::istream& is);

/**
 * Reads an integer value from the input stream @p is.
 *
 * Signed integer values must conform to the following grammar:
 *
 * ```
 * SignedInteger = ["+" | "-"] Digit (Digit | "'")*
 *
 * Digit = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
 * ```
 *
 * Unsigned integer values must conform to the following grammar:
 *
 * ```
 * UnsignedInteger = ["+"] Digit (Digit | "'")*
 *
 * Digit = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
 * ```
 *
 * @tparam I the integer type
 * @param is the input stream
 * @return an integer value of type @p I
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::io::ParseFailure if `is.eof()` returns `true` or if the input cannot be parsed as an
 *     integer value
 */
template<typename I> requires Integer<I>
I
getInteger(std::istream& is) {
  size_t inputPos = io::tellg(is);
  std::string input;

  // Read sign, if any
  if constexpr (std::signed_integral<I>) {
    auto c = io::getOptionalChar(is, Symbols::Chars::PlusMinus);
    if (c)
      input.push_back(*c);
  } else {
    auto c = io::getOptionalChar(is, '+');
    if (c)
      input.push_back(*c);
  }

  // Read at least one digit
  std::string s = io::getWhile(is, Symbols::Chars::Digits, 1);
  input.append(s);

  // Read more digits and apostrophes, if any
  s = io::getWhile(is, Symbols::Chars::DigitsApostrophe, 0);
  input.append(s);

  // Copy input, remove apostrophes
  auto localInput = input;
  localInput.erase(std::remove(localInput.begin(), localInput.end(), '\''), localInput.end());

  // Use type-specific `operator>>`
  I ret;
  auto localIs = io::is(localInput);
  localIs >> ret;
  if (localIs.fail() || io::tellg(localIs) != localInput.size()) {
    throw io::ParseFailure(is, inputPos, { inputPos, inputPos + input.size() },
        message::cannotParseAs(input, Type::of<I>()));
  }
  return ret;
}

/**
 * Reads a floating-point value from the input stream @p is.
 *
 * Floating points must conform to the following grammar:
 *
 * ```
 * FloatingPoint = Infinity | Nan | Number
 *
 * Infinity = ["+" | "-"] ("inf" | "∞")
 * Nan = "nan" | "qnan" | "snan"
 * Number = ["+" | "-"] NumberSpec [Exp]
 * NumberSpec = Digits ["."] | "." Digits | Digits "." Digits
 * Digits = Digit (Digit | "'")*
 * Digit = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
 * Exp = ("e" | "E") ["+" | "-"] Digit+
 * ```
 *
 * @tparam F the floating-point type
 * @param is the input stream
 * @param precision the floating-point precision to use
 * @return a floating-point value of type @p F
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::io::ParseFailure if `is.eof()` returns `true` or if the input cannot be parsed as a
 *     floating-point value
 */
template<typename F> requires FloatingPoint<F>
F
getFloatingPoint(std::istream& is, int precision = DEFAULT_PRECISION) {
  size_t inputPos = io::tellg(is);
  std::string input;

  // Try symbols
  try {
    const std::numeric_limits<F> limits;
    auto s = io::getString(is, Symbols::Strings::FloatingPoint);
    if (s == Symbols::String::Infinity || s == Symbols::String::PositiveInfinity ||
        s == Symbols::String::InfinitySymbol || s == Symbols::String::PositiveInfinitySymbol)
      return limits.infinity();
    else if (s == Symbols::String::NegativeInfinity || s == Symbols::String::NegativeInfinitySymbol)
      return -limits.infinity();
    else if (s == Symbols::String::Qnan)
      return limits.quiet_NaN();
    else // `Symbols::Nan`, `Symbols::Snan`
      return limits.signaling_NaN();
  } catch (io::InputFailure&) {
    // Reset the stream, continue
    io::seekg(is, inputPos);
  }

  // Read sign, if any
  auto c = io::getOptionalChar(is, Symbols::Chars::PlusMinus);
  if (c)
    input.push_back(*c);

  // Read before dot, if any
  c = io::getOptionalChar(is, Symbols::Chars::Digits);
  if (c) {
    input.push_back(*c);
    std::string s = io::getWhile(is, Symbols::Chars::DigitsApostrophe, 0);
    input.append(s);
  }

  // Read dot, if any
  c = io::getOptionalChar(is, '.');
  if (c) {
    input.push_back(*c);

    // Read after dot, if any
    c = io::getOptionalChar(is, Symbols::Chars::Digits);
    if (c) {
      input.push_back(*c);
      std::string s = io::getWhile(is, Symbols::Chars::DigitsApostrophe, 0);
      input.append(s);
    }
  }

  // Read exponential part, if any
  c = io::getOptionalChar(is, Symbols::Chars::E);
  if (c) {
    input.push_back(*c);

    // Read sign, if any
    auto c = io::getOptionalChar(is, Symbols::Chars::PlusMinus); // cppcheck-suppress shadowVariable
    if (c)
      input.push_back(*c);

    // Read at least one digit
    std::string s = io::getWhile(is, Symbols::Chars::Digits, 1);
    input.append(s);
  }

  // Because each component is optional but the entire input may not be empty, read one character if nothing
  // has been read up to this point
  if (input.empty()) {
    char c = io::getChar(is); // cppcheck-suppress shadowVariable
    if (is.eof()) {
      throw io::ParseFailure(is, inputPos, "Expected a character, got EOF");
    }
    io::check(is);
    input.push_back(c);
  }

  // Copy input, remove apostrophes
  auto localInput = input;
  localInput.erase(std::remove(localInput.begin(), localInput.end(), '\''), localInput.end());

  // Use type-specific `operator>>`
  F ret;
  auto localIs = io::is(localInput);
  localIs >> std::setprecision(DEFAULT_PRECISION) >> ret;
  if (localIs.fail() || io::tellg(localIs) != localInput.size()) {
    throw io::ParseFailure(is, inputPos, { inputPos, inputPos + input.size() },
        fmt::format("{}", message::cannotParseAs(input, Type::of<F>())));
  }
  return ret;
}

namespace ron {

// RON parsing ----------------------------------------------------------------------------------------------

namespace parsing {

void skip(std::istream&, bool = true);

/**
 * An instance of this class is returned by #parseEnum.
 */
struct EnumResult {
  /**
   * The entire input string with quotation marks, e.g. `"\"red\""`.
   */
  std::string actualInput;
  /**
   * The position of the entire input string.
   */
  size_t actualInputPos;
  /**
   * The parseable input string without quotation marks, e.g. `"red"`.
   */
  std::string input;
  /**
   * The position of the parseable input string.
   */
  size_t inputPos;
};

/**
 * Parses an enum value from the input stream @p is, which has to be enclosed in quotation marks.
 *
 * @param is the input stream
 * @return an instance of #rocket::codec::ron::parsing::EnumResult
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::io::ParseFailure if `is.eof()` returns `true` or if the input cannot be parsed as an
 *     enum value
 */
EnumResult parseEnum(std::istream& is);

/**
 * Helper function that parses a map as RON.
 *
 * @tparam Map the map type
 * @param is the input stream
 * @param v the value to assign to
 * @return @p is
 */
template<typename Map>
std::istream&
parseMap(std::istream& is, Map& v) {
  skip(is, true);
  io::getChar(is, '{');

  v.clear();

  bool first = true;
  while (true) {
    skip(is, true);
    auto right = io::getOptionalChar(is, '}');
    if (right)
      return is;

    if (first)
      first = false;
    else
      io::getChar(is, ',');

    typename Map::key_type key;
    parseRon(is, key);

    skip(is, true);
    io::getChar(is, ':');

    typename Map::mapped_type value;
    parseRon(is, value);

    v.emplace(std::move(key), std::move(value));
  }
}

/**
 * Helper function that parses a set as RON.
 *
 * @tparam Set the set type
 * @param is the input stream
 * @param v the value to assign to
 * @return @p is
 */
template<typename Set>
std::istream&
parseSet(std::istream& is, Set& v) {
  skip(is, true);
  io::getChar(is, '{');

  v.clear();

  bool first = true;
  while (true) {
    skip(is, true);
    auto right = io::getOptionalChar(is, '}');
    if (right)
      return is;

    if (first)
      first = false;
    else
      io::getChar(is, ',');

    typename Set::value_type value;
    parseRon(is, value);

    v.insert(std::move(value));
  }
}

namespace internal {

template<typename T>
void
parseTupleImpl(std::istream& is, T& v, size_t index) {
  if (index > 0) {
    skip(is, true);
    io::getChar(is, ',');
  }
  parseRon(is, v);
}

} // namespace internal

/**
 * Helper function that parses a tuple as RON.
 *
 * @tparam Tuple the tuple type
 * @param is the input stream
 * @param v the value to assign to
 * @return @p is
 */
template<typename Tuple, size_t... Index>
std::istream&
parseTuple(std::istream& is, Tuple& v, std::index_sequence<Index...>) {
  skip(is, true);
  io::getChar(is, '(');

  (..., internal::parseTupleImpl(is, std::get<Index>(v), Index));

  skip(is, true);
  io::getChar(is, ')');
  return is;
}

namespace internal {

template<typename Variant, size_t Index = 0>
std::istream&
parseVariantImpl(std::istream& is, size_t first, size_t last, Variant& v, size_t index) {
  if constexpr (Index < std::variant_size_v<Variant>) {
    if (Index == index) {
      using Type = std::variant_alternative_t<Index, Variant>;
      Type value;
      parseRon(is, value);
      v = std::move(value);
      return is;
    } else {
      // Go on with next type using template recursion
      return parseVariantImpl<Variant, Index + 1>(is, first, last, v, index);
    }
  } else {
    throw io::ParseFailure(is, first, { first, last }, fmt::format("Invalid index: {}", index));
  }
}

} // namespace internal

/**
 * Helper function that parses a variant as RON.
 *
 * @tparam Variant the variant type
 * @param is the input stream
 * @param v the value to assign to
 * @return @p is
 */
template<typename Variant>
std::istream&
parseVariant(std::istream& is, Variant& v) {
  skip(is, true);

  size_t inputPos = io::tellg(is);
  size_t index = getInteger<size_t>(is);
  size_t indexFirst = inputPos, indexLast = io::tellg(is);

  skip(is, true);
  io::getChar(is, ':');

  return internal::parseVariantImpl(is, indexFirst, indexLast, v, index);
}

/**
 * Helper function that parses a vector as RON.
 *
 * @tparam Vector the vector t ype
 * @param is the input stream
 * @param v the value to assign to
 * @return @p is
 */
template<typename Vector>
std::istream&
parseVector(std::istream& is, Vector& v) {
  skip(is, true);
  io::getChar(is, '[');

  v.clear();

  bool first = true;
  while (true) {
    skip(is, true);
    auto right = io::getOptionalChar(is, ']');
    if (right)
      return is;

    if (first)
      first = false;
    else
      io::getChar(is, ',');

    typename Vector::value_type value;
    parseRon(is, value);

    v.push_back(std::move(value));
  }
}

/**
 * Skips whitespace and comments starting with `#`.
 *
 * Unless @p checkEof is `true`, the input stream's EOF bit may be set when the function returns.
 *
 * @param is the input stream
 * @param checkEof if `true`, a #rocket::io::ParseFailure is thrown if `is.eof()` returns `true`
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::io::ParseFailure if @p checkEof is `true` and `is.eof()` returns `true`
 */
void skip(std::istream& is, bool checkEof);

} // namespace parsing

// RON encoding/decoding ------------------------------------------------------------------------------------

/**
 * Parses the string @p s as a value of type @p T, using #parseRon.
 *
 * @tparam T the type to parse as
 * @param s the string to parse
 * @return a value of type @p T
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::io::ParseFailure if `is.eof()` returns `true` or if the string cannot be parsed as a
 *     value of type @p T
 */
template<typename T>
T
parse(std::string_view s) {
  auto is = io::is(s);
  T v;
  parseRon(is, v);

  parsing::skip(is, false);
  if (not is.eof()) {
    throw io::ParseFailure(is, 0, { 0, s.size() },
        message::cannotParseAs(s, Type::of<T>()));
  }

  return v;
}

/**
 * Tries to parse string @p s as a value of type @p T, using #parseRon.
 *
 * @tparam T the type to parse as
 * @param s the string to parse
 * @return a value of type @p T if the operation succeeds, otherwise null
 */
template<typename T>
std::optional<T>
tryParse(std::string_view s) {
  try {
    return parse<T>(s);
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

} // namespace ron

} // namespace rocket::codec

// EOF
