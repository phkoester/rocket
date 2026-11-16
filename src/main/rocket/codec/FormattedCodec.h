/**
 * @file FormattedCodec.h
 *
 * Reads and writes RON (Rocket Object Notation). The recommended file-name extension is `.ron`.
 *
 * # The RON File Format
 *
 * RON is a human-readable format that is similar to, but not quite the same as, JSON. It is designed to be
 * easy to read and write by hand. It is also designed to be easy to parse and generate programmatically.
 *
 * This codec is generous when decoding, but strict when encoding. This means that when reading, it will
 * accept a variety of formats, but it will be canonical and consistent when writing.
 *
 * For detailed information on the supported data types and how they map to C++, see @ref codec_type_system.
 *
 * ## Text-File Encoding, Line Breaks
 *
 * RON is a text format. It must always be encoded in UTF-8 to work with this codec.
 *
 * When reading, both Unix-style and Windows-style line breaks are accepted. When writing, Unix-style line
 * breaks are used.
 *
 * ## Comments
 *
 * RON supports C-style single-line and multi-line comments. Single-line comments start with `//` and
 * continue to the end of the line. Multi-line comments start with <code>/</code><code>*</code> and end with
 * <code>*</code><code>/</code>.
 *
 * Shell-style comments are also accepted. They start with `#` and continue to the end of the line.
 *
 * Comments are never written.
 *
 * ## Trailing Comma
 *
 * When reading, a trailing comma after the last element of a container is allowed and ignored. When writing,
 * no trailing comma is added.
 *
 * ## Boolean Values
 *
 * When reading, the values `0`, `1`, `false`, and `true` are valid. Case is ignored, so `False` and
 * `truE` are also accepted.
 *
 * Boolean values are always written as `true` or `false`.
 *
 * ## Characters {#formatted_char}
 *
 * Characters start and end with a single quote (`'`). Example values: `'a'`, `'\x20'`, `'€'`, `'\u20AC'`,
 * `'\U0010FFFF'`.
 *
 * One-byte characters must be valid ASCII characters in the range [0,127]. Four-byte characters must be
 * valid Unicode code points in the ranges [U+0000,U+D7FF] and [U+E000,U+10FFFF].
 *
 * When reading, escape sequences as well as the prefixes `\x`, `\u`, and `\U` are accepted.
 *
 * When writing, the escape sequences `\a`, `\b`, `\t`, `\n`, `\v`, `\f`, `\r`, `\e`, `\'`, and `\\` are
 * used. If a character is classified as printable by the Unicode standard, it appears verbatim in the
 * output, e.g., as `'a'` or `'€'`. Otherwise, it is written in hexadecimal notation, using the prefixes
 * `\x` (two hexadecimal digits), `\u` (four hexadecimal digits), or `\U` (eight hexadecimal digits), as
 * needed.
 *
 * ## Enumerations
 *
 * Enumerations appear exactly as in code, with no surrounding quotes. Example values: `Red`, `GREEN`,
 * `powder_blue`.
 *
 * ## Integer Values
 *
 * Example values: `+42`, `-42`, `0xABCD` (hexadecimal), `077` (octal), `0o123` (octal), `0b1010101`
 * (binary).
 *
 * Integer values are always written in decimal notation, without any leading `+`.
 *
 * ## Floating-Point Values
 *
 * Example values: `.1`, `0.1`, `+2.`, `-2.0`, `1e3`, `-1e3`, `1.23e-4`, `1.23e+4`, `-inf`, `inf`, `+inf`,
 * `-∞`, `∞`, `nan`.
 *
 * Floating-point values are always written in their canonical form, without any leading `+`. For infinite
 * values, `-∞` and `∞` are used.
 *
 * ## Pointers
 *
 * Pointers are always hexadecimal values, starting with `0x`. The null pointer is notated as `null`.
 *
 * ## Strings
 *
 * Strings always appear surrounded by double quotes (`"`). Example values: `"Hello"`, `"Hello\nWorld"`,
 * `"3 \u20AC"`. Any well-formed UTF-8 string is a valid RON string.
 *
 * When writing, the writing rules for @ref formatted_char "characters" apply. Grapheme clusters are
 * recognized and written verbatim, except that a quote or backslash inside a cluster is escaped.
 *
 * ## Optional Values
 *
 * Optional values appear just like normal values, with one exception: null options are notated as `null`.
 *
 * ## Tuples
 *
 * Tuples appear surrounded by parentheses (`(` and `)`). Example values: `(1, 2, 3)`,
 * `("Answer", 42, true)`.
 *
 * ## Lists
 *
 * Lists appear surrounded by square brackets (`[` and `]`). Examples: `[1, 2, 3]`,
 * `["one", "two", "three"]`.
 *
 * ## Sets
 *
 * Sets appear surrounded by curly braces (`{` and `}`). Example values: `{1, 2, 3}`,
 * `{"one", "two", "three"}`.
 *
 * Duplicate elements are discouraged but allowed. If an element appears multiple times, the first one wins.
 *
 * ## Maps
 *
 * Maps appear surrounded by curly braces (`{` and `}`). Key-value pairs are separated by a colon (`:`).
 * Example: `{1: "one", 2: "two", 3: "three"}`.
 *
 * Duplicate keys are discouraged but allowed. If a key appears multiple times, the last one wins.
 *
 * ## Bidirectional Maps
 *
 * Bidirectional maps appear just like normal maps. Only the left side is notated. The right side is the
 * implicit opposite.
 *
 * Duplicate keys are discouraged but allowed. If a key appears multiple times, the last one wins.
 *
 * ## Durations
 *
 * Durations are notated as {fmt} writes them: a count, directly followed by a unit. Example values: `5ns`,
 * `1700µs`, `90min`, `2[604800]s` (two weeks).
 *
 * Unit       | Meaning
 * :--------- | :------
 * `ns`       | Nanoseconds
 * `µs`       | Microseconds
 * `us`       | Microseconds (reading only)
 * `ms`       | Milliseconds
 * `s`        | Seconds
 * `min`      | Minutes
 * `h`        | Hours
 * `d`        | Days
 * `[n]s`     | Units of `n` seconds, e.g., `[604800]s` for weeks
 * `[n/d]s`   | Units of `n/d` seconds
 *
 * Other SI prefixes that {fmt} writes, such as `ps` or `ks`, are accepted, too. When reading, the value is
 * converted to the target type. A value that is not exactly representable in the target type, or that does
 * not fit into it, is an error.
 *
 * ## Hours, Minutes, Seconds
 *
 * Example values: `01:02:03`, `01:02:03.123`, `01:02:03.123456`, `-111:02:03.123456789`.
 *
 * ## Dates
 *
 * Example values: `1970-01-02`, `-100-01-02`.
 *
 * Dates are written by {fmt} and read with the scnlib format `%F`. Years from -32767 through 9999 can be
 * read; a nonnegative year has exactly four digits.
 *
 * ## Time Zones
 *
 * Time zones are notated by their IANA names, without quotes. Example values: `Europe/Berlin`, `UTC`,
 * `Etc/GMT+5`.
 *
 * When reading, a name consists of ASCII letters, digits, and the characters `_`, `/`, `+`, and `-`.
 *
 * ## Times
 *
 * Example value: `2026-10-07T09:05:26.529594325Z`.
 *
 * ## Times with Time Zone
 *
 * Example value: `2026-10-07T11:21:00.561378057+02:00 Europe/Berlin`.
 *
 * The UTC offset is `Z` for +00:00. When reading, the offset must be one that the time zone has at that
 * local time; it selects between the two meanings of an ambiguous local time.
 *
 * ## Intervals
 *
 * Intervals are read and written in the notation of `rocket/math/Interval-format.h` and
 * `rocket/math/Interval-scan.h`. Example values: `∅` (empty), `[-1.2,3.4]` (closed), `(-1.2,3.4)` (open),
 * `(-∞,-12]` (left-open), `[12,∞)` (right-open). A missing bound is notated as `-∞` or `∞`.
 *
 * Whitespace is allowed between the tokens of an interval, comments are not.
 *
 * ## Declared Objects
 *
 * Members appear like a tuple surrounded by parentheses (`(` and `)`), where each entry is a name-value pair
 * separated by an equal sign (`=`). Example: `(x_=11, y_=12, name_="A")`.
 *
 * Duplicate member names are discouraged but allowed. If a name appears multiple times, the last one wins.
 *
 * ## Instances
 *
 * Instances appear just like declared objects. They only display a different set of members, most probably
 * a subset.
 *
 * Duplicate member names are discouraged but allowed. If a name appears multiple times, the last one wins.
 *
 * ## Members
 *
 * See declared objects.
 *
 * ## Variables
 *
 * Variables appear like a tuple surrounded by parentheses (`(` and `)`), where each entry is a name-value
 * pair separated by an equal sign (`=`). Example: `(x=11, y=12, name="A")`.
 *
 * They cannot be decoded.
 *
 * ## Code Points
 *
 * Code points appear in the typical Unicode notation, e.g., `U+0041` or `U+10FFFF`.
 *
 * A single character is also accepted when reading.
 *
 * ## Grapheme Clusters
 *
 * Grapheme clusters appear just like strings.
 */

#pragma once

#include "rocket/InputFailure.h"
#include "rocket/TypeId-format.h"
#include "rocket/format.h"
#include "rocket/scan.h"
#include "rocket/std.h"
#include "rocket/codec/codec.h"
#include "rocket/math/Interval-format.h"
#include "rocket/math/Interval-scan.h"
#include "rocket/nio/nio.h"
#include "rocket/nio/nio-utils.h"
#include "rocket/str/escape/escape.h"
#include "rocket/unicode/ConvertTo.h"
#include "rocket/unicode/unicode-format.h"
#include "rocket/unicode/unicode-scan.h"

namespace rocket::codec {

// `FormattedConsumerConfig` --------------------------------------------------------------------------------

/// Configuration for #rocket::codec::FormattedConsumer.
struct FormattedConsumerConfig {
  /// Whether to indent the output and format a tree.
  bool indent = false;
  /// The current level of indentation.
  u64 level = 0;
};

namespace internal::formatted {

// Functions ------------------------------------------------------------------------------------------------

inline void
beginContainer(nio::Sink& out, FormattedConsumerConfig& config, char c) {
  rocket::codec::internal::beginContainer(out, config.indent, config.level, c);
}

inline void
endContainer(nio::Sink& out, FormattedConsumerConfig& config, u64 size, char c) {
  rocket::codec::internal::endContainer(out, config.indent, config.level, size, c);
}

inline void
nextElem(nio::Sink& out, FormattedConsumerConfig& config, u64 index) {
  rocket::codec::internal::nextElem(out, config.indent, config.level, index);
}

inline void
skip(nio::Source& in) {
  rocket::nio::skip(in, true, true);
}

// `FormattedConsumerImpl` ----------------------------------------------------------------------------------

/// @cond undocumented
#define CONFIG__ [[maybe_unused]] FormattedConsumerConfig& config
/// @endcond

template<DataType DataType, typename T>
struct FormattedConsumerImpl;

template<>
struct FormattedConsumerImpl<DataType::Bool, bool> {
  void
  consume(bool val, nio::Sink& out, CONFIG__) const {
    out.write(val ? "true" : "false");
  }
};

template<typename C>
struct FormattedConsumerImpl<DataType::Char, C> {
  void
  consume(C val, nio::Sink& out, CONFIG__) const {
    const std::basic_string<C> str { val };
    const std::string utf8(unicode::ConvertTo<char>::apply(str));
    const std::string escaped = str::escape::escapeCString(utf8, { .quote='\'' });
    out.print("{}", escaped);
  }
};

template<typename E>
struct FormattedConsumerImpl<DataType::Enum, E> {
  void
  consume(E val, nio::Sink& out, CONFIG__) const {
    if constexpr (fmt::is_formattable<E>::value) {
      out.print("{}", val);
      return;
    }
    ROCKET_FAIL("cannot format enum of type {}", TypeId::of<E>());
  }
};

template<typename I>
struct FormattedConsumerImpl<DataType::Integer, I> {
  void
  consume(I val, nio::Sink& out, CONFIG__) const {
    out.print("{}", val);
  }
};

template<typename F>
struct FormattedConsumerImpl<DataType::Float, F> {
  using Limits = std::numeric_limits<F>;

  void
  consume(F val, nio::Sink& out, CONFIG__) const {
    if (val == -Limits::infinity()) {
      out.write("-∞");
      return;
    }
    if (val == Limits::infinity()) {
      out.write("∞");
      return;
    }
    out.print("{}", val);
  }
};

template<typename P>
struct FormattedConsumerImpl<DataType::Pointer, P> {
  void
  consume(P val, nio::Sink& out, CONFIG__) const {
    if (val == nullptr) {
      out.write("null");
      return;
    }
    out.print("0x{:X}", reinterpret_cast<std::uintptr_t>(static_cast<const void*>(val)));
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::String, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    const std::string utf8(unicode::ConvertTo<char>::apply(val));
    const std::string escaped = str::escape::escapeCString(utf8, { .quote='"' });
    out.print("{}", escaped);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Optional, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    if (not val) {
      out.write("null");
      return;
    }
    FormattedConsumerImpl<ElemDataType, Elem>().consume(*val, out, config);
  }
};

// For `MemberRef`, the tuple consumer must be able to pass additional arguments to the element consumer
template<typename T>
struct FormattedConsumerImpl<DataType::Tuple, T> {
  template<typename... Args>
  void
  consume(const T& val, nio::Sink& out, CONFIG__, Args&&... args) const {
    beginContainer(out, config, '(');
    u64 index = 0;
    std::apply([&](auto&&... arg) {
      (consumeElem(std::forward<decltype(arg)>(arg),
        out, config, index++, std::forward<Args>(args)...), ...);
    }, val);
    endContainer(out, config, std::tuple_size_v<T>, ')');
  }

private:

  template<typename Elem, typename... Args>
  void
  consumeElem(const Elem& elem, nio::Sink& out, CONFIG__, u64 index, Args&&... args) const {
    nextElem(out, config, index);
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    FormattedConsumerImpl<ElemDataType, Elem>().consume(elem, out, config, std::forward<Args>(args)...);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::List, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    beginContainer(out, config, '[');
    u64 index = 0;
    for (const auto& elem : val) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, index, ']');
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Set, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    beginContainer(out, config, '{');
    u64 index = 0;
    for (const auto& elem : val) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), '}');
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Map, T> {
  using Key = T::key_type;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = T::mapped_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    beginContainer(out, config, '{');
    u64 index = 0;
    for (const auto& [key, elem] : val) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<KeyDataType, Key>().consume(key, out, config);
      out.write(": ");
      FormattedConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), '}');
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Bimap, T> {
  using Key = Purge<typename T::left_value_type::first_type>;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = Purge<typename T::left_value_type::second_type>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    beginContainer(out, config, '{');
    u64 index = 0;
    for (const auto& [key, elem] : val.left) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<KeyDataType, Key>().consume(key, out, config);
      out.write(": ");
      FormattedConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), '}');
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Duration, T> {
  void
  consume(T val, nio::Sink& out, CONFIG__) const { // Take by value
    out.print("{}", val);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Date, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.write(fmt::format("{}", val));
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::HourMinuteSecond, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.print("{}", val);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::TimeZone, T> {
  void
  consume(T val, nio::Sink& out, CONFIG__) const {
    out.write(val->name());
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Time, T> {
  using Clock = T::clock;
  static_assert(std::same_as<Clock, std::chrono::system_clock>, "clock must be `system_clock`");

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.write(fmt::format("{:%FT%TZ}", val)); // Zulu time
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::ZonedTime, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    const auto* tz = val.get_time_zone();
    if (val.get_info().offset == std::chrono::seconds(0)) {
      out.print("{:%FT%TZ} {}", val, tz->name()); // Zulu time
    } else {
      out.print("{:%FT%T%Ez} {}", val, tz->name()); // Time with UTC offset
    }
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Interval, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.print("{}", val);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Declared, T> {
  static constexpr auto& refs = rocket::reflect::Declared<T>::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    // Here we have to pass an additional argument, the instance, to the tuple consumer. The tuple consumer
    // will pass it on to the member-reference consumer
    FormattedConsumerImpl<ElemDataType, Elem>().consume(refs, out, config, val);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Instance, T> {
  static constexpr auto& refs = T::InnerType::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    // Here we have to pass an additional argument, the instance, to the tuple consumer. The tuple consumer
    // will pass it on to the member-reference consumer
    FormattedConsumerImpl<ElemDataType, Elem>().consume(refs, out, config, val.get());
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::MemberRef, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  template<typename C>
  void
  consume(const T& val, nio::Sink& out, CONFIG__, const C& instance) const {
    out.write(val.name());
    out.write('=');
    FormattedConsumerImpl<ElemDataType, Elem>().consume(val.get(instance), out, config);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::VarRef, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.write(val.name());
    out.write('=');
    FormattedConsumerImpl<ElemDataType, Elem>().consume(val.get(), out, config);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::CodePoint, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.print("{}", val);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Character, T> {
  using Elem = T::View;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    const auto elem = static_cast<Elem>(val);
    FormattedConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
  }
};

#undef CONFIG__

// `FormattedProducerImpl` ----------------------------------------------------------------------------------

template<DataType DataType, typename T>
struct FormattedProducerImpl;

/**
 * Produces a single member of @p instance if the name of the member reference @p ref matches @p name.
 * Returns whether the name matched, so callers can fold over a tuple of member references.
 */
template<typename Ref, typename C>
bool
produceMember(const Ref& ref, std::string_view name, C& instance, nio::Source& in) {
  if (ref.name() != name) {
    return false;
  }
  using Elem = Ref::Type;
  constexpr auto ElemDataType = DataTypes<Elem>::Value;
  FormattedProducerImpl<ElemDataType, Elem>().produce(ref.get(instance), in);
  return true;
}

/**
 * Produces the members of @p instance from a parenthesized list of `name=value` entries, using the member
 * references in @p refs to look up each name.
 *
 * Unlike the tuple producer, the entries may appear in any order, and entries may be missing altogether, in
 * which case the corresponding members of @p instance are left untouched. An entry whose name does not match
 * any member reference is an error.
 */
template<typename Refs, typename C>
void
produceMembers(const Refs& refs, C& instance, nio::Source& in) {
  skip(in);
  const auto pos = in.tell();

  if (not readChar(in, '(')) {
    throw InputFailure(pos, "expected a tuple");
  }

  u64 index = 0;
  while (true) {
    skip(in);
    if (readChar(in, ')')) {
      return;
    }
    if (index++ > 0) {
      expectComma(in);
      skip(in);
      if (readChar(in, ')')) { // Allow trailing comma if nonempty
        return;
      }
    }

    // Read the member name

    const auto namePos = in.tell();
    const auto name = readIdentifier(in);
    if (not name) {
      throw InputFailure(namePos, "expected a member reference");
    }
    skip(in);
    expectChar(in, '=');

    // Look up the member reference by name and produce the member

    const bool found = std::apply([&](const auto&... ref) {
      return (produceMember(ref, *name, instance, in) || ...);
    }, refs);
    if (not found) {
      throw InputFailure(namePos, fmt::format("unknown member `{}`", *name));
    }
  }
}

template<>
struct FormattedProducerImpl<DataType::Bool, bool> {
  void
  produce(bool& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (readChoice(in, { "0", "false" }, true)) {
      val = false;
      return;
    }
    if (readChoice(in, { "1", "true" }, true)) {
      val = true;
      return;
    }
    throw InputFailure(pos, "expected a boolean value");
  }
};

template<typename C>
struct FormattedProducerImpl<DataType::Char, C> {
  void
  produce(C& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (not readChar(in, '\'')) {
      throw InputFailure(pos, "expected a character");
    }

    auto input = readUntilUnescapedChar(in, '\'');
    if (not input) {
      throw InputFailure(pos, "unterminated character literal");
    }

    const std::string unescaped = unescapeLiteral(*input, pos);
    const std::basic_string<C> str(unicode::ConvertTo<C>::apply(unescaped));
    if (str.size() != 1) {
      throw InputFailure(pos, "invalid character literal");
    }
    val = str[0];
  }
};

template<typename E>
struct FormattedProducerImpl<DataType::Enum, E> {
  void
  produce(E& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if constexpr (scn::detail::is_scannable<E, char>::value) {
      const auto result = in.scan<E>("{}");
      if (result) {
        val = result->value();
        return;
      }
      throw InputFailure(pos, fmt::format("invalid value for enum {}", TypeId::of<E>()));
    }
    throw InputFailure(pos, fmt::format("cannot scan enum of type {}", TypeId::of<E>()));
  }
};

template<typename I>
struct FormattedProducerImpl<DataType::Integer, I> {
  void
  produce(I& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    // `{:i}` detects the base from the input, so a prefix such as `0x` is accepted
    const auto result = in.scan<I>("{:i}");
    if (result) {
      val = result->value();
      return;
    }
    throw InputFailure(pos, "expected an integer value");
  }
};

template<typename F>
struct FormattedProducerImpl<DataType::Float, F> {
  using Limits = std::numeric_limits<F>;

  void
  produce(F& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (readString(in, "-∞")) {
      val = -Limits::infinity();
      return;
    }
    if (readString(in, "∞")) {
      val = Limits::infinity();
      return;
    }

    const auto result = in.scan<F>("{}");
    if (result) {
      val = result->value();
      return;
    }
    throw InputFailure(pos, "expected a floating-point value");
  }
};

template<typename P>
struct FormattedProducerImpl<DataType::Pointer, P> {
  void
  produce(P& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (readString(in, "null")) {
      val = nullptr;
      return;
    }

    const auto result = in.scan<P>("{}");
    if (result) {
      val = result->value();
      return;
    }
    throw InputFailure(pos, "expected a pointer value");
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::String, T> {
  using C = T::value_type;

  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (not readChar(in, '"')) {
      throw InputFailure(pos, "expected a string");
    }

    auto input = readUntilUnescapedChar(in, '"');
    if (not input) {
      throw InputFailure(pos, "unterminated string literal");
    }

    const std::string unescaped = unescapeLiteral(*input, pos);
    std::basic_string<C> str(unicode::ConvertTo<C>::apply(unescaped));
    if constexpr (std::same_as<T, std::basic_string_view<C>>) {
      // Decoding a string view: make it point to a string bound to the decoder
      const auto& ref = internal::DecoderStore::get()->store(std::move(str));
      val = ref;
    } else {
      // `std::basic_string`
      val = std::move(str);
    }
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Optional, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skip(in);

    if (readString(in, "null")) {
      val = std::nullopt;
      return;
    }

    val = Elem();
    FormattedProducerImpl<ElemDataType, Elem>().produce(*val, in);
  }
};

// For `MemberRef`, the tuple producer must be able to pass additional arguments to the element producer
template<typename T>
struct FormattedProducerImpl<DataType::Tuple, T> {
  template<typename... Args>
  void
  produce(T& val, nio::Source& in, Args&&... args) const {
    skip(in);
    const auto pos = in.tell();

    if (not readChar(in, '(')) {
      throw InputFailure(pos, "expected a tuple");
    }

    u64 index = 0;
    std::apply([&](auto&&... arg) {
      (produceElem(std::forward<decltype(arg)>(arg), in, index++, std::forward<Args>(args)...), ...);
    }, val);

    skip(in);
    if (std::tuple_size_v<T> > 0 && readChar(in, ',')) { // Allow trailing comma if nonempty
      skip(in);
    }
    if (not readChar(in, ')')) {
      const auto end = in.tell();
      throw InputFailure::withRange(end, pos, end, "unterminated tuple");
    }
  }

private:

  template<typename Elem, typename... Args>
  void
  produceElem(Elem& elem, nio::Source& in, u64 index, Args&&... args) const {
    skip(in);
    if (index > 0) {
      expectComma(in);
      skip(in);
    }
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    FormattedProducerImpl<ElemDataType, Elem>().produce(elem, in, std::forward<Args>(args)...);
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::List, T> {
  static_assert(not IsView<T>, "cannot decode list view");
  static_assert(not IsForwardList<T>, "cannot decode forward list");

  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if constexpr (not IsArray<T>) {
      val.clear();
    }

    if (not readChar(in, '[')) {
      throw InputFailure(pos, "expected a list");
    }

    if constexpr (IsArray<T>) {
      // Fixed-size array
      produceArray(val, in, pos);
    } else {
      // Container with `push_back`
      produceContainerWithPushBack(val, in);
    }
  }

private:

  void
  produceArray(T& val, nio::Source& in, std::optional<u64> pos) const {
    const auto size = val.size();
    for (u64 index = 0; index < size; ++index) {
      skip(in);
      if (index > 0) {
        expectComma(in);
        skip(in);
      }
      FormattedProducerImpl<ElemDataType, Elem>().produce(val[index], in);
    }

    skip(in);
    if (size > 0 && readChar(in, ',')) { // Allow trailing comma if nonempty
      skip(in);
    }
    if (not readChar(in, ']')) {
      const auto end = in.tell();
      throw InputFailure::withRange(end, pos, end, fmt::format("unterminated array of size {}", size));
    }
  }

  void
  produceContainerWithPushBack(T& val, nio::Source& in) const {
    u64 index = 0;
    while (true) {
      skip(in);
      if (readChar(in, ']')) {
        return;
      }
      if (index++ > 0) {
        expectComma(in);
        skip(in);
        if (readChar(in, ']')) { // Allow trailing comma if nonempty
          return;
        }
      }
      val.push_back(Elem());
      FormattedProducerImpl<ElemDataType, Elem>().produce(val.back(), in);
    }
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Set, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    val.clear();
    if (not readChar(in, '{')) {
      throw InputFailure(pos, "expected a set");
    }

    u64 index = 0;
    while (true) {
      skip(in);
      if (readChar(in, '}')) {
        return;
      }
      if (index++ > 0) {
        expectComma(in);
        skip(in);
        if (readChar(in, '}')) { // Allow trailing comma if nonempty
          return;
        }
      }
      Elem elem;
      FormattedProducerImpl<ElemDataType, Elem>().produce(elem, in);
      val.insert(std::move(elem));
    }
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Map, T> {
  using Key = T::key_type;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = T::mapped_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    val.clear();

    if (not readChar(in, '{')) {
      throw InputFailure(pos, "expected a map");
    }

    u64 index = 0;
    while (true) {
      skip(in);
      if (readChar(in, '}')) {
        return;
      }
      if (index++ > 0) {
        expectComma(in);
        skip(in);
        if (readChar(in, '}')) { // Allow trailing comma if nonempty
          return;
        }
        skip(in);
      }

      Key key;
      FormattedProducerImpl<KeyDataType, Key>().produce(key, in);
      skip(in);

      expectColon(in);
      skip(in);

      Elem elem;
      FormattedProducerImpl<ElemDataType, Elem>().produce(elem, in);
      // Make the last one win
      val.insert_or_assign(std::move(key), std::move(elem));
    }
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Bimap, T> {
  using Key = Purge<typename T::left_value_type::first_type>;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = Purge<typename T::left_value_type::second_type>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    val.clear();

    if (not readChar(in, '{')) {
      throw InputFailure(pos, "expected a map");
    }

    u64 index = 0;
    while (true) {
      skip(in);
      if (readChar(in, '}')) {
        return;
      }
      if (index++ > 0) {
        expectComma(in);
        skip(in);
        if (readChar(in, '}')) { // Allow trailing comma if nonempty
          return;
        }
      }

      Key key;
      FormattedProducerImpl<KeyDataType, Key>().produce(key, in);
      skip(in);

      expectColon(in);
      skip(in);

      Elem elem;
      FormattedProducerImpl<ElemDataType, Elem>().produce(elem, in);
      // Make the last one win. Both the key and the element must be unique, so remove any pair with either
      val.left.erase(key);
      val.right.erase(elem);
      val.insert({ std::move(key), std::move(elem) });
    }
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Duration, T> {
  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    auto result = in.scan<T>("{}");
    if (not result) {
      throw InputFailure(pos, durationError(result.error()));
    }
    val = result->value();
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Date, T> {
  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    auto result = in.scan<T>("{:%F}");
    if (not result) {
      throw InputFailure(pos, dateError(result.error()));
    }
    // A digit after the date means that a field has too many digits, e.g., `2026-01-257`
    const auto next = in.peek();
    if (not result->value().ok() || (next && *next >= '0' && *next <= '9')) {
      throw InputFailure(pos, "invalid date");
    }
    val = result->value();
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::HourMinuteSecond, T> {
  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    auto result = in.scan<T>("{}");
    if (not result) {
      throw InputFailure(pos, hourMinuteSecondError(result.error()));
    }
    val = result->value();
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::TimeZone, T> {
  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    // The characters of IANA time-zone names, e.g., `America/Port-au-Prince` or `Etc/GMT+5`
    const std::string name = readWhilePredicate(in, [](char c) {
      return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' ||
        c == '/' || c == '+' || c == '-';
    });
    if (name.empty()) {
      throw InputFailure(pos, "expected a time zone");
    }
    try {
      // This throws if the time zone is not found
      val = rocket::chrono::locateUnambiguousZone(name);
    } catch (const std::exception& ex) {
      throw InputFailure(pos, ex.what());
    }
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Time, T> {
  using Clock = T::clock;
  static_assert(std::same_as<Clock, std::chrono::system_clock>, "clock must be `system_clock`");

  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    // The patched scnlib reads optional subseconds with `%.S`
    auto result = in.scan<T>("{:%FT%H:%M:%.SZ}");
    if (not result) {
      throw InputFailure(pos, "expected a time point");
    }
    val = result->value();
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::ZonedTime, T> {
  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    // The patched scnlib reads optional subseconds with `%.S` and `Z` as a UTC offset with `%Ez`
    auto result = in.scan<T>("{:%FT%H:%M:%.S%Ez %Z}");
    if (not result) {
      throw InputFailure(pos, zonedTimeError(result.error()));
    }
    val = result->value();
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Interval, T> {
  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    auto result = in.scan<T>("{}");
    if (not result) {
      throw InputFailure(pos, "expected an interval");
    }
    val = result->value();
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Declared, T> {
  static constexpr auto& refs = rocket::reflect::Declared<T>::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  produce(T& val, nio::Source& in) const {
    produceMembers(refs, val, in);
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Instance, T> {
  static constexpr auto& refs = T::InnerType::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  produce(T& val, nio::Source& in) const {
    produceMembers(refs, val.get(), in);
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::MemberRef, T> {
  static_assert(false, "cannot decode member reference");
};

template<typename T>
struct FormattedProducerImpl<DataType::VarRef, T> {
  static_assert(false, "cannot decode variable reference");
};

template<typename T>
struct FormattedProducerImpl<DataType::CodePoint, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (readChar(in, '\'')) {
      // Read `char32`
      in.seek(-1, nio::SeekMode::cur);
      Elem elem = Elem();
      FormattedProducerImpl<ElemDataType, Elem>().produce(elem, in);
      val = T(elem);
      return;
    }

    // Read `U+...`
    auto result = in.scan<T>("{}");
    if (not result) {
      throw InputFailure(pos, result.error().msg());
    }
    val = result->value();
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Character, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    Elem elem;
    FormattedProducerImpl<ElemDataType, Elem>().produce(elem, in);
    if (elem.empty()) {
      throw InputFailure(pos, "expected a character");
    }
    val = T(std::move(elem));
  }
};

} // namespace internal::formatted

// `FormattedConsumer` --------------------------------------------------------------------------------------

/// The consumer for #rocket::codec::FormattedCodec.
struct FormattedConsumer {
  /// @type_alias
  template<DataType DataType, typename T>
  using Type = internal::formatted::FormattedConsumerImpl<DataType, T>;
};

// `FormattedProducer` --------------------------------------------------------------------------------------

/// The producer for #rocket::codec::FormattedCodec.
struct FormattedProducer {
  /// @type_alias
  template<DataType DataType, typename T>
  using Type = internal::formatted::FormattedProducerImpl<DataType, T>;
};

// `FormattedCodec` -----------------------------------------------------------------------------------------

/**
 * A codec for formatted (RON) I/O.
 *
 * The encoder can serialize an arbitrary C++ data structure to a sink. The output is in a format called RON
 * (Rocket Object Notation), which is similar to, but not quite the same as, JSON. Tuples are enclosed in
 * parentheses. Lists are enclosed in square brackets. Sets and maps are enclosed in curly braces. If a
 * configuration is provided, the output may be indented and formatted as a tree.
 *
 * The decoder can scan such a formatted input from a source and construct an arbitrary C++ data structure
 * from it. While scanning, any irrelevant whitespace, including line breaks, is ignored. The decoder skips
 * single-line C-style comments starting with <code>//</code>, multi-line C-style comments starting with
 * <code>/</code><code>*</code>, as well as single-line shell-style comments starting with `#`.
 *
 * When decoding a declared type or a #rocket::reflect::Instance, the `name=value` entries may appear in any
 * order, and entries may be missing altogether; missing members keep their default values. An entry whose
 * name does not match any member is an error.
 *
 * Decoding to list views and forward lists is not supported. String views and character views, however, are
 * allowed. This is made possible by keeping real strings in a store bound to the decoder. Hence, decoded
 * string views and character views are valid, and valid only, for the lifetime of the decoder.
 *
 * There are various optimizations for decoding from contiguous sources.
 *
 * @see #rocket::codec::FormattedConsumerConfig
 */
struct FormattedCodec : Codec<FormattedConsumer, FormattedProducer> {
  using Base = Codec<FormattedConsumer, FormattedProducer>; ///< @type_base

  /**
   * Encodes a value.
   *
   * @tparam T the type to encode
   * @param val the value to encode
   * @param out the output sink
   * @param config the configuration
   * @return whatever the consumer returns
   */
  template<typename T>
  auto
  encode(const T& val, nio::Sink& out, const FormattedConsumerConfig& config = {}) const {
    FormattedConsumerConfig localConfig = config;
    localConfig.level = 0;
    return Base::encode(val, out, localConfig);
  }

  /**
   * Decodes a value from a source.
   *
   * @tparam T the type to decode
   * @param in the input source
   * @return the decoded value
   */
  template<typename T>
  [[nodiscard]] DecodeResult<T>
  decode(nio::Source& in) const {
    T val {};
    auto result = Base::decode<T>(val, in);
    if (not result) {
      internal::snapToClusterStarts(result.error(), in);
    }
    return result;
  }
};

} // namespace rocket::codec

// EOF
