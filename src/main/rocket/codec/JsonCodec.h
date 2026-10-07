/**
 * @file JsonCodec.h
 *
 * Reads and writes JSON (JavaScript Object Notation).
 *
 * # The JSON Codec
 *
 * This codec is generous when decoding, but strict when encoding. This means that when reading, it will
 * accept a variety of formats, but it will be canonical and consistent when writing.
 *
 * The encoder writes standard JSON (not JSON5), except for the floating-point values `-Infinity`,
 * `Infinity`, and `NaN`. The decoder accepts those same floating-point literals, C-style comments, and a
 * trailing comma after the last element of an array or object.
 *
 * For detailed information on the supported data types and how they map to C++, see @ref codec_type_system.
 *
 * ## Text-File Encoding, Line Breaks
 *
 * JSON is a text format. It must always be encoded in UTF-8 to work with this codec.
 *
 * When reading, both Unix-style and Windows-style line breaks are accepted. When writing, Unix-style line
 * breaks are used.
 *
 * ## Comments
 *
 * When reading, C-style single-line and multi-line comments are accepted. Single-line comments start with
 * `//` and continue to the end of the line. Multi-line comments start with <code>/</code><code>*</code> and
 * end with <code>*</code><code>/</code>. Comments are never written.
 *
 * ## Trailing Comma
 *
 * When reading, a trailing comma after the last element of an array or object is allowed and ignored. When
 * writing, no trailing comma is added.
 *
 * ## Boolean Values
 *
 * Boolean values are read and written as `true` or `false`.
 *
 * ## Characters {#json_char}
 *
 * Characters are JSON strings of length one. Example values: `"a"`, `"\t"`, `"€"` `"\u20AC".
 *
 * One-byte characters must be valid ASCII characters in the range [0,127]. Two-byte characters must be valid
 * Unicode code points in the ranges [U+0000,U+D7FF] and [U+E000,U+10FFFF].
 *
 * When reading, escape sequences as well as the prefixes `\x`, `\u`, and `\U` are accepted.
 *
 * When writing, the escape sequences `\a`, `\b`, `\t`, `\n`, `\v`, `\f`, `\r`, `\e`, `\'`, and `\\` are
 * used. If a character is classified as printable by the Unicode standard, it appears verbatim in the
 * output, e.g. as `"a"` or `"€"`. Otherwise, it is written in hexadecimal notation, using the prefix `\u`
 * (four hexadecimal digits).
 *
 * ## Enumerations
 *
 * Enumerations are JSON strings. Example values: `"Red"`, `"GREEN"`, `"powder_blue"`.
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
 * Example values: `.1`, `0.1`, `+2.`, `-2.0`, `1e3`, `-1e3`, `1.23e-4`, `1.23e+4`, `-Infinity`, `Infinity`,
 * `+Infinity`, `NaN`.
 *
 * Floating-point values are always written in their canonical form, without any leading `+`. For nonfinite
 * values, `-Infinity`, `Infinity`, and `NaN` are used.
 *
 * ## Pointers
 *
 * The null pointer is notated as `null`. Other pointers are hexadecimal JSON strings starting with `0x`.
 *
 * ## Strings
 *
 * Strings always appear surrounded by double quotes (`"`). Example values: `"Hello"`, `"Hello\nWorld"`,
 * `"3 \u20AC"`. Any well-formed UTF-8 string is a valid JSON string.
 *
 * When writing, the writing rules for @ref json_char "characters" apply. Grapheme clusters are recognized
 * and written verbatim.
 *
 * ## Optional Values
 *
 * Optional values appear just like normal values, with one exception: null options are notated as `null`.
 *
 * ## Tuples
 *
 * Tuples are JSON arrays. Example values: `[1, 2, 3]`, `["Answer", 42, true]`.
 *
 * ## Lists
 *
 * Lists are JSON arrays. Examples: `[1, 2, 3]`, `["one", "two", "three"]`.
 *
 * ## Sets
 *
 * Sets are JSON arrays. Example values: `[1, 2, 3]`, `["one", "two", "three"]`.
 *
 * Duplicate elements are discouraged but allowed. If an element appears multiple times, the first one wins.
 *
 * ## Maps
 *
 * Maps are JSON objects. Keys are always JSON strings surrounded by double quotes (`"`). Example:
 * `{ "alpha": 1, "beta": 2}`.
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
 * Durations are JSON strings of an integer value directly followed by a unit. Example values: `"5ns"`,
 * `"1700µs"`.
 *
 * Unit  | Meaning
 * :---- | :------
 * `ns`  | Nanoseconds
 * `µs`  | Microseconds
 * `us`  | Microseconds
 * `ms`  | Milliseconds
 * `s`   | Seconds
 * `min` | Minutes
 * `h`   | Hours
 * `d`   | Days
 * `w`   | Weeks
 * `m`   | Months
 * `y`   | Years
 *
 * ## Hours, Minutes, Seconds
 *
 * Example values: `"01:02:03"`, `"01:02:03.123"`, `"-111:02:03.123456789"`.
 *
 * ## Dates
 *
 * Example values: `"1970-01-02"`, `"-100-01-02"`.
 *
 * ## Time Zones
 *
 * Time zones are JSON strings. Example values: `"Europe/Berlin"`, `"UTC"`.
 *
 * ## Times
 *
 * Example value: `"2026-10-07T09:05:26.529594325Z"`.
 *
 * ## Times with Time Zone
 *
 * Example value: `"2026-10-07T11:21:00.561378057+02:00 (Europe/Berlin)"`.
 *
 * ## Intervals
 *
 * Empty intervals are notated as `null`. Non-empty intervals are JSON arrays of two bounds. A missing
 * (infinite) bound is notated as `null`.
 *
 * Example values: `"null"`, `"[-1.2, 3.4]"`, `"[null, -12]"`, or `"[12, null]"`.
 *
 * ## Declared Objects
 *
 * Members appear as a JSON object, where each entry is a name-value pair. Example:
 * `{"x_": 11, "y_": 12, "name_": "A"}`.
 *
 * Duplicate member names are discouraged but allowed. If a name appears multiple times, the last one wins.
 *
 * ## Instances
 *
 * Instances appear just like declared objects. They only display a different set of members, most probably
 * a subset.
 *
 * ## Members
 *
 * See declared objects.
 *
 * ## Variables
 *
 * Variables appear as a JSON object, where each entry is a name-value pair. Example:
 * `{"x": 11, "y": 12, "name": "A"}`.
 *
 * They cannot be decoded.
 *
 * ## Code Points
 *
 * Code points appear as JSON strings in the typical Unicode notation, e.g. `"U+0041"` or `"U+10FFFF"`.
 *
 * A JSON string of a single character is also accepted when reading.
 *
 * ## Grapheme Clusters
 *
 * Grapheme clusters appear just like strings.
 */

#pragma once

#include "rocket/InputFailure.h"
#include "rocket/scan.h"
#include "rocket/std.h"
#include "rocket/codec/FormattedCodec.h"
#include "rocket/codec/codec.h"
#include "rocket/io/io.h"
#include "rocket/nio/nio.h"
#include "rocket/nio/nio-utils.h"
#include "rocket/system/system.h"
#include "rocket/unicode/ConvertTo.h"

#include <fmt/std.h>

#include <scn/chrono.h>

#include <cctype>
#include <cmath>
#include <limits>

namespace rocket::codec {

// `JsonConsumerConfig` -------------------------------------------------------------------------------------

/// Configuration for #rocket::codec::JsonConsumer.
struct JsonConsumerConfig {
  /// Whether to indent the output and format a tree.
  bool indent = false;
  /// The current level of indentation.
  u64 level = 0;
};

namespace internal {

// XXX
// Functions ------------------------------------------------------------------------------------------------

inline void
beginContainer(nio::Sink& out, JsonConsumerConfig& config, char c) {
  rocket::nio::beginContainer(out, config.indent, config.level, c);
}

inline void
endContainer(nio::Sink& out, JsonConsumerConfig& config, u64 size, char c) {
  rocket::nio::endContainer(out, config.indent, config.level, size, c);
}

inline void
nextElem(nio::Sink& out, JsonConsumerConfig& config, u64 index) {
  rocket::nio::nextElem(out, config.indent, config.level, index);
}

inline void
skipJson(nio::Source& in) {
  rocket::nio::skip(in, true, false);
}

inline bool
isIdentChar(char c) {
  const auto u = static_cast<unsigned char>(c);
  return std::isalnum(u) || c == '_' || c == '$';
}

/**
 * Reads an expected keyword, advances the source only on success.
 *
 * Unlike #rocket::nio::readString, the keyword must not be followed by an identifier character.
 */
inline bool
readKeyword(nio::Source& in, std::string_view keyword) {
  const auto pos = in.tell();
  if (not readString(in, keyword)) {
    return false;
  }
  char c; // NOLINT
  if (in.read(c) != 1) {
    return true;
  }
  if (isIdentChar(c)) {
    in.seek(static_cast<i64>(pos), nio::SeekMode::beg);
    return false;
  }
  in.seek(-1, nio::SeekMode::cur);
  return true;
}

inline void
writeJsonString(nio::Sink& out, std::string_view utf8) {
  out.write('"');
  for (const unsigned char c : utf8) {
    switch (c) {
    case '"':
      out.write("\\\"");
      break;
    case '\\':
      out.write("\\\\");
      break;
    case '\b':
      out.write("\\b");
      break;
    case '\f':
      out.write("\\f");
      break;
    case '\n':
      out.write("\\n");
      break;
    case '\r':
      out.write("\\r");
      break;
    case '\t':
      out.write("\\t");
      break;
    default:
      if (c < 0x20 || c == 0x7F) {
        out.print("\\u{:04X}", static_cast<u32>(c));
      } else {
        out.write(static_cast<char>(c));
      }
      break;
    }
  }
  out.write('"');
}

inline u32
readHex4(nio::Source& in, u64 pos) {
  u32 ret = 0;
  for (u32 i = 0; i < 4; ++i) {
    char c; // NOLINT
    if (in.read(c) != 1) {
      throw InputFailure(pos, "Invalid Unicode escape");
    }
    const auto u = static_cast<unsigned char>(c);
    u32 nibble = 0;
    if (u >= '0' && u <= '9') {
      nibble = u - '0';
    } else if (u >= 'A' && u <= 'F') {
      nibble = u - 'A' + 10;
    } else if (u >= 'a' && u <= 'f') {
      nibble = u - 'a' + 10;
    } else {
      throw InputFailure(pos, "Invalid Unicode escape");
    }
    ret = (ret << 4) | nibble;
  }
  return ret;
}

inline std::string
readJsonString(nio::Source& in) {
  const auto pos = in.tell();
  if (not readChar(in, '"')) {
    throw InputFailure(pos, "Expected a string");
  }

  std::string ret;
  while (true) {
    char c; // NOLINT
    if (in.read(c) != 1) {
      throw InputFailure(pos, "Unterminated string literal");
    }
    if (c == '"') {
      return ret;
    }
    if (c == '\\') {
      char e; // NOLINT
      if (in.read(e) != 1) {
        throw InputFailure(pos, "Unterminated string literal");
      }
      switch (e) {
      case '"':
      case '\\':
      case '/':
        ret.push_back(e);
        break;
      case 'b':
        ret.push_back('\b');
        break;
      case 'f':
        ret.push_back('\f');
        break;
      case 'n':
        ret.push_back('\n');
        break;
      case 'r':
        ret.push_back('\r');
        break;
      case 't':
        ret.push_back('\t');
        break;
      case 'u': {
        const u32 u = readHex4(in, pos);
        if (u >= 0xD800 && u <= 0xDBFF) {
          if (not readString(in, "\\u")) {
            throw InputFailure(pos, "Invalid Unicode escape");
          }
          const u32 low = readHex4(in, pos);
          if (low < 0xDC00 || low > 0xDFFF) {
            throw InputFailure(pos, "Invalid Unicode escape");
          }
          const char32 cp = 0x10000 + (static_cast<char32>(u - 0xD800) << 10) + (low - 0xDC00);
          ret.append(static_cast<std::string>(unicode::CodePoint(cp)));
        } else if (u >= 0xDC00 && u <= 0xDFFF) {
          throw InputFailure(pos, "Invalid Unicode escape");
        } else {
          ret.append(static_cast<std::string>(unicode::CodePoint(static_cast<char32>(u))));
        }
        break;
      }
      default:
        throw InputFailure(in.tell(), "Invalid escape sequence");
      }
      continue;
    }
    if (static_cast<unsigned char>(c) < 0x20) {
      throw InputFailure(in.tell(), "Invalid control character in string");
    }
    ret.push_back(c);
  }
}

consteval bool
jsonEncodedAsString(DataType type) {
  switch (type) {
  case DataType::Char:
  case DataType::Enum:
  case DataType::String:
  case DataType::Duration:
  case DataType::Date:
  case DataType::HourMinuteSecond:
  case DataType::TimeZone:
  case DataType::Time:
  case DataType::ZonedTime:
  case DataType::CodePoint:
  case DataType::Character:
    return true;
  default:
    return false;
  }
}

// `JsonConsumerImpl` ---------------------------------------------------------------------------------------

/// @cond undocumented
#define CONFIG__ [[maybe_unused]] JsonConsumerConfig& config
/// @endcond

template<DataType DataType, typename T>
struct JsonConsumerImpl;

template<typename Key>
void
writeJsonKey(const Key& key, nio::Sink& out, JsonConsumerConfig&) {
  constexpr auto KeyDataType = DataTypes<Key>::Value;
  nio::StringSink tmp;
  JsonConsumerConfig compact {};
  JsonConsumerImpl<KeyDataType, Key>().consume(key, tmp, compact);
  const auto& encoded = tmp.str();
  if (encoded.size() >= 2 && encoded.front() == '"' && encoded.back() == '"') {
    out.write(encoded);
  } else {
    writeJsonString(out, encoded);
  }
}

template<>
struct JsonConsumerImpl<DataType::Bool, bool> {
  void
  consume(bool val, nio::Sink& out, CONFIG__) const {
    out.write(val ? "true" : "false");
  }
};

template<typename C>
struct JsonConsumerImpl<DataType::Char, C> {
  void
  consume(C val, nio::Sink& out, CONFIG__) const {
    const std::basic_string<C> str { val };
    const std::string utf8(unicode::ConvertTo<char>::apply(str));
    writeJsonString(out, utf8);
  }
};

template<typename E>
struct JsonConsumerImpl<DataType::Enum, E> {
  void
  consume(E val, nio::Sink& out, CONFIG__) const {
    if constexpr (fmt::is_formattable<E>::value) {
      writeJsonString(out, fmt::format("{}", val));
      return;
    }
    ROCKET_FAIL("Cannot format enum of type `{}`", typeid(E));
  }
};

template<typename I>
struct JsonConsumerImpl<DataType::Integer, I> {
  void
  consume(I val, nio::Sink& out, CONFIG__) const {
    out.print("{}", val);
  }
};

template<typename F>
struct JsonConsumerImpl<DataType::Float, F> {
  using Limits = std::numeric_limits<F>;

  void
  consume(F val, nio::Sink& out, CONFIG__) const {
    if (val == -Limits::infinity()) {
      out.write("-Infinity");
      return;
    }
    if (val == Limits::infinity()) {
      out.write("Infinity");
      return;
    }
    if (std::isnan(val)) {
      out.write("NaN");
      return;
    }
    out.print("{}", val);
  }
};

template<typename P>
struct JsonConsumerImpl<DataType::Pointer, P> {
  void
  consume(P val, nio::Sink& out, CONFIG__) const {
    if (val == nullptr) {
      out.write("null");
      return;
    }
    writeJsonString(out, fmt::format("{}", static_cast<const void*>(val)));
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::String, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    const std::string utf8(unicode::ConvertTo<char>::apply(val));
    writeJsonString(out, utf8);
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::Optional, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    if (not val) {
      out.write("null");
      return;
    }

    JsonConsumerImpl<ElemDataType, Elem>().consume(*val, out, config);
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::Tuple, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    beginContainer(out, config, '[');
    u64 index = 0;
    std::apply([&](auto&&... arg) {
      (consumeElem(std::forward<decltype(arg)>(arg), out, config, index++), ...);
    }, val);
    endContainer(out, config, std::tuple_size_v<T>, ']');
  }

private:

  template<typename Elem>
  void
  consumeElem(const Elem& elem, nio::Sink& out, CONFIG__, u64 index) const {
    nextElem(out, config, index);
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    JsonConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::List, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    beginContainer(out, config, '[');
    u64 index = 0;
    for (const auto& elem : val) {
      nextElem(out, config, index++);
      JsonConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, index, ']');
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::Set, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    beginContainer(out, config, '[');
    u64 index = 0;
    for (const auto& elem : val) {
      nextElem(out, config, index++);
      JsonConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), ']');
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::Map, T> {
  using Key = T::key_type;
  using Elem = T::mapped_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    beginContainer(out, config, '{');
    u64 index = 0;
    for (const auto& [key, elem] : val) {
      nextElem(out, config, index++);
      writeJsonKey(key, out, config);
      out.write(": ");
      JsonConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), '}');
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::Bimap, T> {
  using Key = Purge<typename T::left_value_type::first_type>;
  using Elem = Purge<typename T::left_value_type::second_type>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    beginContainer(out, config, '{');
    u64 index = 0;
    for (const auto& [key, elem] : val.left) {
      nextElem(out, config, index++);
      writeJsonKey(key, out, config);
      out.write(": ");
      JsonConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), '}');
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::Duration, T> {
  void
  consume(T val, nio::Sink& out, CONFIG__) const { // Take by value
    nio::StringSink tmp;
    FormattedConsumerConfig ron {};
    FormattedConsumerImpl<DataType::Duration, T>().consume(val, tmp, ron);
    writeJsonString(out, tmp.str());
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::Date, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    nio::StringSink tmp;
    FormattedConsumerConfig ron {};
    FormattedConsumerImpl<DataType::Date, T>().consume(val, tmp, ron);
    writeJsonString(out, tmp.str());
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::HourMinuteSecond, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    nio::StringSink tmp;
    FormattedConsumerConfig ron {};
    FormattedConsumerImpl<DataType::HourMinuteSecond, T>().consume(val, tmp, ron);
    writeJsonString(out, tmp.str());
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::TimeZone, T> {
  void
  consume(T val, nio::Sink& out, CONFIG__) const {
    writeJsonString(out, val->name());
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::Time, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    nio::StringSink tmp;
    FormattedConsumerConfig ron {};
    FormattedConsumerImpl<DataType::Time, T>().consume(val, tmp, ron);
    writeJsonString(out, tmp.str());
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::ZonedTime, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    nio::StringSink tmp;
    FormattedConsumerConfig ron {};
    FormattedConsumerImpl<DataType::ZonedTime, T>().consume(val, tmp, ron);
    writeJsonString(out, tmp.str());
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::Interval, T> {
  using A = T::A;
  static constexpr auto ADataType = DataTypes<A>::Value;
  using B = T::B;
  static constexpr auto BDataType = DataTypes<B>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    if (val.empty()) {
      out.write("null");
      return;
    }

    out.write('[');
    const auto optA = optionalOf(val.a);
    if (not optA) {
      out.write("null");
    } else {
      JsonConsumerImpl<ADataType, A>().consume(val.a, out, config);
    }
    out.write(", ");
    const auto optB = optionalOf(val.b);
    if (not optB) {
      out.write("null");
    } else {
      JsonConsumerImpl<BDataType, B>().consume(val.b, out, config);
    }
    out.write(']');
  }
};

template<typename Ref, typename C>
void
consumeObjectElem(const Ref& ref, const C& instance, nio::Sink& out, JsonConsumerConfig& config, u64 index) {
  nextElem(out, config, index);
  using RefType = Purge<Ref>;
  JsonConsumerImpl<DataType::MemberRef, RefType>().consume(ref, out, config, instance);
}

template<typename Refs, typename C>
void
consumeObject(const Refs& refs, const C& instance, nio::Sink& out, JsonConsumerConfig& config) {
  beginContainer(out, config, '{');
  u64 index = 0;
  std::apply([&](const auto&... ref) {
    (consumeObjectElem(ref, instance, out, config, index++), ...);
  }, refs);
  endContainer(out, config, std::tuple_size_v<Refs>, '}');
}

template<typename T>
struct JsonConsumerImpl<DataType::Declared, T> {
  static constexpr auto& refs = rocket::reflect::Declared<T>::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    consumeObject(refs, val, out, config);
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::Instance, T> {
  static constexpr auto& refs = T::InnerType::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    consumeObject(refs, val.get(), out, config);
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::MemberRef, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  template<typename C>
  void
  consume(const T& val, nio::Sink& out, CONFIG__, const C& instance) const {
    writeJsonString(out, val.name());
    out.write(": ");
    JsonConsumerImpl<ElemDataType, Elem>().consume(val.get(instance), out, config);
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::VarRef, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    beginContainer(out, config, '{');
    nextElem(out, config, 0);
    writeJsonString(out, val.name());
    out.write(": ");
    JsonConsumerImpl<ElemDataType, Elem>().consume(val.get(), out, config);
    endContainer(out, config, 1, '}');
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::CodePoint, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    writeJsonString(out, fmt::format("U+{:0>4X}", static_cast<u32>(val)));
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::Character, T> {
  using Elem = T::View;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    const auto elem = static_cast<Elem>(val);
    JsonConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
  }
};

// `JsonProducerImpl` ---------------------------------------------------------------------------------------

template<DataType DataType, typename T>
struct JsonProducerImpl;

template<typename Key>
void
readJsonKey(Key& key, nio::Source& in) {
  constexpr auto KeyDataType = DataTypes<Key>::Value;
  const std::string keyStr = readJsonString(in);
  nio::StringSink tmp;
  if constexpr (jsonEncodedAsString(KeyDataType)) {
    writeJsonString(tmp, keyStr);
  } else {
    static_cast<nio::Sink&>(tmp).write(keyStr);
  }
  nio::StringSource keyIn(tmp.str());
  JsonProducerImpl<KeyDataType, Key>().produce(key, keyIn);
}

/**
 * Produces a single member of @p instance if the name of the member reference @p ref matches @p name.
 * Returns whether the name matched, so callers can fold over a tuple of member references
 */
template<typename Ref, typename C>
bool
produceJsonMember(const Ref& ref, std::string_view name, C& instance, nio::Source& in) {
  if (ref.name() != name) {
    return false;
  }
  using Elem = Ref::Type;
  constexpr auto ElemDataType = DataTypes<Elem>::Value;
  JsonProducerImpl<ElemDataType, Elem>().produce(ref.get(instance), in);
  return true;
}

/**
 * Produces the members of @p instance from a JSON object of `"name": value` entries, using the member
 * references in @p refs to look up each name.
 *
 * Unlike the tuple producer, the entries may appear in any order, and entries may be missing altogether, in
 * which case the corresponding members of @p instance are left untouched. An entry whose name does not match
 * any member reference is an error.
 */
template<typename Refs, typename C>
void
produceJsonMembers(const Refs& refs, C& instance, nio::Source& in) {
  skipJson(in);
  const auto pos = in.tell();

  if (not readChar(in, '{')) {
    throw InputFailure(pos, "Expected an object");
  }

  u64 index = 0;
  while (true) {
    skipJson(in);
    if (readChar(in, '}')) {
      return;
    }
    if (index++ > 0) {
      expectComma(in);
      skipJson(in);
      if (readChar(in, '}')) { // Allow trailing comma if nonempty
        return;
      }
    }

    // Read the member name

    const auto namePos = in.tell();
    const std::string name = readJsonString(in);
    skipJson(in);

    expectColon(in);
    skipJson(in);

    // Look up the member reference by name and produce the member

    const bool found = std::apply([&](const auto&... ref) {
      return (produceJsonMember(ref, name, instance, in) || ...);
    }, refs);
    if (not found) {
      throw InputFailure(namePos, fmt::format("Unknown member `{}`", name));
    }
  }
}

template<>
struct JsonProducerImpl<DataType::Bool, bool> {
  void
  produce(bool& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    if (readKeyword(in, "false")) {
      val = false;
      return;
    }
    if (readKeyword(in, "true")) {
      val = true;
      return;
    }
    throw InputFailure(pos, "Expected a boolean value");
  }
};

template<typename C>
struct JsonProducerImpl<DataType::Char, C> {
  void
  produce(C& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    if (not readChar(in, '"')) {
      throw InputFailure(pos, "Expected a character");
    }
    in.seek(-1, nio::SeekMode::cur);

    const std::string unescaped = readJsonString(in);
    const std::basic_string<C> str(unicode::ConvertTo<C>::apply(unescaped));
    if (str.size() != 1) {
      throw InputFailure(pos, "Invalid character literal");
    }
    val = str[0];
  }
};

template<typename E>
struct JsonProducerImpl<DataType::Enum, E> {
  void
  produce(E& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    if constexpr (scn::detail::is_scannable<E, char>::value) {
      const std::string str = readJsonString(in);
      nio::StringSource inner(str);
      const auto result = scan<E>(inner);
      if (result) {
        val = *result;
        return;
      }
      throw InputFailure(pos, fmt::format("Invalid value for enum `{}`", typeid(E)));
    }
    throw InputFailure(pos, fmt::format("Cannot scan enum of type `{}`", typeid(E)));
  }
};

template<typename I>
struct JsonProducerImpl<DataType::Integer, I> {
  void
  produce(I& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    const auto result = scan<I>(in);
    if (result) {
      val = *result;
      return;
    }
    throw InputFailure(pos, "Expected an integer value");
  }
};

template<typename F>
struct JsonProducerImpl<DataType::Float, F> {
  using Limits = std::numeric_limits<F>;

  void
  produce(F& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    if (readKeyword(in, "-Infinity")) {
      val = -Limits::infinity();
      return;
    }
    if (readKeyword(in, "+Infinity") || readKeyword(in, "Infinity")) {
      val = Limits::infinity();
      return;
    }
    if (readKeyword(in, "NaN")) {
      val = Limits::quiet_NaN();
      return;
    }

    const auto result = scan<F>(in);
    if (result) {
      val = *result;
      return;
    }
    throw InputFailure(pos, "Expected a floating-point value");
  }
};

template<typename P>
struct JsonProducerImpl<DataType::Pointer, P> {
  void
  produce(P& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    if (readKeyword(in, "null")) {
      val = nullptr;
      return;
    }

    if (readChar(in, '"')) {
      in.seek(-1, nio::SeekMode::cur);
      const std::string str = readJsonString(in);
      nio::StringSource inner(str);
      const auto result = scan<P>(inner);
      if (result) {
        val = *result;
        return;
      }
      throw InputFailure(pos, "Expected a pointer value");
    }

    const auto result = scan<P>(in);
    if (result) {
      val = *result;
      return;
    }
    throw InputFailure(pos, "Expected a pointer value");
  }
};

template<typename T>
struct JsonProducerImpl<DataType::String, T> {
  void
  produce(T& val, nio::Source& in) const {
    skipJson(in);

    const std::string unescaped = readJsonString(in);
    using C = T::value_type;
    std::basic_string<C> str(unicode::ConvertTo<C>::apply(unescaped));
    if constexpr (std::same_as<T, std::basic_string_view<C>>) {
      // Because both unescaping and Unicode-converting produce intermediate strings local to this function,
      // decoding to `std::basic_string_view` is a bit more involved. To make this possible, we store the
      // intermediate string in the source so the decoded string view is valid for the lifetime of the source
      const auto& ref = in.store(std::move(str));
      val = ref;
    } else {
      // `std::basic_string`
      val = std::move(str);
    }
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Optional, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skipJson(in);

    if (readKeyword(in, "null")) {
      val = std::nullopt;
      return;
    }

    val = Elem();
    JsonProducerImpl<ElemDataType, Elem>().produce(*val, in);
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Tuple, T> {
  void
  produce(T& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    if (not readChar(in, '[')) {
      throw InputFailure(pos, "Expected an array");
    }

    u64 index = 0;
    std::apply([&](auto&&... arg) {
      (produceElem(std::forward<decltype(arg)>(arg), in, index++), ...);
    }, val);

    skipJson(in);
    if (std::tuple_size_v<T> > 0 && readChar(in, ',')) { // Allow trailing comma if nonempty
      skipJson(in);
    }
    if (not readChar(in, ']')) {
      throw InputFailure(in.tell(), { pos, in.tell() }, "Unterminated array");
    }
  }

private:

  template<typename Elem>
  void
  produceElem(Elem& elem, nio::Source& in, u64 index) const {
    skipJson(in);
    if (index > 0) {
      expectComma(in);
      skipJson(in);
    }
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    JsonProducerImpl<ElemDataType, Elem>().produce(elem, in);
  }
};

template<typename T>
struct JsonProducerImpl<DataType::List, T> {
  static_assert(not IsView<T>, "Cannot decode list view");
  static_assert(not IsForwardList<T>, "Cannot decode forward list");

  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    if constexpr (not IsArray<T>) {
      val.clear();
    }

    if (not readChar(in, '[')) {
      throw InputFailure(pos, "Expected an array");
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
  produceArray(T& val, nio::Source& in, u64 pos) const {
    const auto size = val.size();
    for (u64 index = 0; index < size; ++index) {
      skipJson(in);
      if (index > 0) {
        expectComma(in);
        skipJson(in);
      }
      JsonProducerImpl<ElemDataType, Elem>().produce(val[index], in);
    }

    skipJson(in);
    if (size > 0 && readChar(in, ',')) { // Allow trailing comma if nonempty
      skipJson(in);
    }
    if (not readChar(in, ']')) {
      throw InputFailure(in.tell(), { pos, in.tell() }, fmt::format("Unterminated array of size {}", size));
    }
  }

  void
  produceContainerWithPushBack(T& val, nio::Source& in) const {
    u64 index = 0;
    while (true) {
      skipJson(in);
      if (readChar(in, ']')) {
        return;
      }
      if (index++ > 0) {
        expectComma(in);
        skipJson(in);
        if (readChar(in, ']')) { // Allow trailing comma if nonempty
          return;
        }
      }
      val.push_back(Elem());
      JsonProducerImpl<ElemDataType, Elem>().produce(val.back(), in);
    }
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Set, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    val.clear();
    if (not readChar(in, '[')) {
      throw InputFailure(pos, "Expected an array");
    }

    u64 index = 0;
    while (true) {
      skipJson(in);
      if (readChar(in, ']')) {
        return;
      }
      if (index++ > 0) {
        expectComma(in);
        skipJson(in);
        if (readChar(in, ']')) { // Allow trailing comma if nonempty
          return;
        }
      }
      Elem elem;
      JsonProducerImpl<ElemDataType, Elem>().produce(elem, in);
      val.insert(std::move(elem));
    }
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Map, T> {
  using Key = T::key_type;
  using Elem = T::mapped_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    val.clear();

    if (not readChar(in, '{')) {
      throw InputFailure(pos, "Expected an object");
    }

    u64 index = 0;
    while (true) {
      skipJson(in);
      if (readChar(in, '}')) {
        return;
      }
      if (index++ > 0) {
        expectComma(in);
        skipJson(in);
        if (readChar(in, '}')) { // Allow trailing comma if nonempty
          return;
        }
      }

      Key key;
      readJsonKey(key, in);
      skipJson(in);

      expectColon(in);
      skipJson(in);

      Elem elem;
      JsonProducerImpl<ElemDataType, Elem>().produce(elem, in);
      // Make the last one win
      val.insert_or_assign(std::move(key), std::move(elem));
    }
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Bimap, T> {
  using Key = Purge<typename T::left_value_type::first_type>;
  using Elem = Purge<typename T::left_value_type::second_type>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    val.clear();

    if (not readChar(in, '{')) {
      throw InputFailure(pos, "Expected an object");
    }

    u64 index = 0;
    while (true) {
      skipJson(in);
      if (readChar(in, '}')) {
        return;
      }
      if (index++ > 0) {
        expectComma(in);
        skipJson(in);
        if (readChar(in, '}')) { // Allow trailing comma if nonempty
          return;
        }
      }

      Key key;
      readJsonKey(key, in);
      skipJson(in);

      expectColon(in);
      skipJson(in);

      Elem elem;
      JsonProducerImpl<ElemDataType, Elem>().produce(elem, in);
      // Make the last one win
      insertOrReplace(val, key, elem);
    }
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Duration, T> {
  void
  produce(T& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    if (not readChar(in, '"')) {
      throw InputFailure(pos, "Expected a duration");
    }
    in.seek(-1, nio::SeekMode::cur);

    try {
      const std::string str = readJsonString(in);
      nio::StringSource inner(str);
      FormattedProducerImpl<DataType::Duration, T>().produce(val, inner);
    } catch (const InputFailure& ex) {
      throw InputFailure(pos, ex.message());
    }
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Date, T> {
  void
  produce(T& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    if (not readChar(in, '"')) {
      throw InputFailure(pos, "Expected a date");
    }
    in.seek(-1, nio::SeekMode::cur);

    try {
      const std::string str = readJsonString(in);
      nio::StringSource inner(str);
      FormattedProducerImpl<DataType::Date, T>().produce(val, inner);
    } catch (const InputFailure&) {
      throw InputFailure(pos, "Expected a date");
    }
  }
};

template<typename T>
struct JsonProducerImpl<DataType::HourMinuteSecond, T> {
  void
  produce(T& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    if (not readChar(in, '"')) {
      throw InputFailure(pos, "Expected an hour, minute, and second");
    }
    in.seek(-1, nio::SeekMode::cur);

    try {
      const std::string str = readJsonString(in);
      nio::StringSource inner(str);
      FormattedProducerImpl<DataType::HourMinuteSecond, T>().produce(val, inner);
    } catch (const InputFailure& ex) {
      throw InputFailure(pos, ex.message());
    }
  }
};

template<typename T>
struct JsonProducerImpl<DataType::TimeZone, T> {
  void
  produce(T& val, nio::Source& in) const {
    using namespace std::chrono;

    skipJson(in);
    const auto pos = in.tell();

    if (not readChar(in, '"')) {
      throw InputFailure(pos, "Expected a time zone");
    }
    in.seek(-1, nio::SeekMode::cur);

    try {
      const std::string name = readJsonString(in);
      // This throws if the time zone is not found
      val = locate_zone(name);
    } catch (const InputFailure&) {
      throw InputFailure(pos, "Expected a time zone");
    } catch (const std::exception& ex) {
      throw InputFailure(pos, ex.what());
    }
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Time, T> {
  void
  produce(T& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    if (not readChar(in, '"')) {
      throw InputFailure(pos, "Expected a time point");
    }
    in.seek(-1, nio::SeekMode::cur);

    try {
      const std::string str = readJsonString(in);
      nio::StringSource inner(str);
      FormattedProducerImpl<DataType::Time, T>().produce(val, inner);
    } catch (const InputFailure&) {
      throw InputFailure(pos, "Expected a time point");
    }
  }
};

template<typename T>
struct JsonProducerImpl<DataType::ZonedTime, T> {
  void
  produce(T& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    if (not readChar(in, '"')) {
      throw InputFailure(pos, "Expected a time point");
    }
    in.seek(-1, nio::SeekMode::cur);

    try {
      const std::string str = readJsonString(in);
      nio::StringSource inner(str);
      FormattedProducerImpl<DataType::ZonedTime, T>().produce(val, inner);
    } catch (const InputFailure&) {
      throw InputFailure(pos, "Expected a time point");
    }
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Interval, T> {
  using A = T::A;
  static constexpr auto ADataType = DataTypes<A>::Value;
  using B = T::B;
  static constexpr auto BDataType = DataTypes<B>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    if (readKeyword(in, "null")) {
      val = T();
      return;
    }

    if (not readChar(in, '[')) {
      throw InputFailure(pos, "Expected an interval");
    }
    skipJson(in);

    A a = A();
    if constexpr (IsOptional<A>) {
      if (not readKeyword(in, "null")) {
        JsonProducerImpl<ADataType, A>().produce(a, in);
      }
    } else {
      JsonProducerImpl<ADataType, A>().produce(a, in);
    }
    val.a = a;

    skipJson(in);
    expectComma(in);
    skipJson(in);

    B b = B();
    if constexpr (IsOptional<B>) {
      if (not readKeyword(in, "null")) {
        JsonProducerImpl<BDataType, B>().produce(b, in);
      }
    } else {
      JsonProducerImpl<BDataType, B>().produce(b, in);
    }
    val.b = b;

    skipJson(in);
    if (readChar(in, ',')) { // Allow trailing comma
      skipJson(in);
    }
    if (not readChar(in, ']')) {
      throw InputFailure(in.tell(), { pos, in.tell() }, "Unterminated interval");
    }
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Declared, T> {
  static constexpr auto& refs = rocket::reflect::Declared<T>::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  produce(T& val, nio::Source& in) const {
    produceJsonMembers(refs, val, in);
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Instance, T> {
  static constexpr auto& refs = T::InnerType::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  produce(T& val, nio::Source& in) const {
    produceJsonMembers(refs, val.get(), in);
  }
};

// `MemberRef` production is implemented in `produceJsonMembers` and `produceJsonMember`

template<typename T>
struct JsonProducerImpl<DataType::VarRef, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    if (not readChar(in, '{')) {
      throw InputFailure(pos, "Expected an object");
    }
    skipJson(in);

    if (readChar(in, '}')) {
      return;
    }

    (void) readJsonString(in); // Ignore the name
    skipJson(in);
    expectColon(in);
    skipJson(in);

    JsonProducerImpl<ElemDataType, Elem>().produce(val.get(), in);

    skipJson(in);
    if (readChar(in, ',')) { // Allow trailing comma
      skipJson(in);
    }
    if (not readChar(in, '}')) {
      throw InputFailure(in.tell(), { pos, in.tell() }, "Unterminated object");
    }
  }
};

template<typename T>
struct JsonProducerImpl<DataType::CodePoint, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skipJson(in);
    const auto pos = in.tell();

    const std::string str = readJsonString(in);
    if (str.size() >= 2 && str[0] == 'U' && str[1] == '+') {
      nio::StringSource inner(str);
      auto result = scanCodePoint<u32>(inner);
      if (result) {
        val = static_cast<Elem>(*result);
        return;
      }
      throw InputFailure(pos, "Expected a code point");
    }

    const std::u32string u32(unicode::ConvertTo<char32>::apply(str));
    if (u32.size() != 1) {
      throw InputFailure(pos, "Expected a code point");
    }
    val = static_cast<Elem>(u32[0]);
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Character, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    Elem elem;
    JsonProducerImpl<ElemDataType, Elem>().produce(elem, in);
    val = T(std::move(elem));
  }
};

#undef CONFIG__

} // namespace internal

// `JsonConsumer` -------------------------------------------------------------------------------------------

/// The consumer for #rocket::codec::JsonCodec.
struct JsonConsumer {
  /// @type_alias
  template<DataType DataType, typename T>
  using Type = internal::JsonConsumerImpl<DataType, T>;
};

// `JsonProducer` -------------------------------------------------------------------------------------------

/// The producer for #rocket::codec::JsonCodec.
struct JsonProducer {
  /// @type_alias
  template<DataType DataType, typename T>
  using Type = internal::JsonProducerImpl<DataType, T>;
};

// `JsonCodec` ----------------------------------------------------------------------------------------------

/**
 * A codec for JSON I/O.
 *
 * The encoder can serialize an arbitrary C++ data structure to a sink. The output is standard JSON, except
 * for the floating-point values `Infinity`, `-Infinity`, and `NaN`. Tuples, lists, and sets are JSON arrays.
 * Maps, declared objects, and instances are JSON objects. If a configuration is provided, the output may be
 * indented and formatted as a tree.
 *
 * The decoder can scan such a JSON input from a source and construct an arbitrary C++ data structure from
 * it. While scanning, any irrelevant whitespace, including line breaks, is ignored. The decoder skips
 * single-line C-style comments starting with <code>//</code> and multi-line C-style comments starting with
 * <code>/</code><code>*</code>. A trailing comma after the last element of an array or object is allowed.
 *
 * When decoding a declared type or a #rocket::reflect::Instance, the `"name": value` entries may appear in
 * any order, and entries may be missing altogether; missing members keep their default values. An entry
 * whose name does not match any member is an error.
 *
 * Decoding to list views and forward lists is not supported. String views and character views, however, are
 * allowed. This is made possible by storing intermediate strings in the source. Hence, decoded string views
 * and character views are valid, and valid only, for the lifetime of the source.
 *
 * There are various optimizations for decoding from contiguous sources.
 *
 * @see #rocket::codec::JsonConsumerConfig
 */
struct JsonCodec : Codec<JsonConsumer, JsonProducer> {
  using Base = Codec<JsonConsumer, JsonProducer>; ///< @type_base

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
  encode(const T& val, nio::Sink& out, const JsonConsumerConfig& config = {}) const {
    JsonConsumerConfig localConfig = config;
    localConfig.level = 0;
    return Base::encode(val, out, localConfig);
  }

  /**
   * Decodes a value from a source.
   *
   * @tparam T the type to decode
   * @param in the input source
   * @return the decoded value
   * @throw #std::exception if the value cannot be decoded
   */
  template<typename T>
  [[nodiscard]] DecodeResult<T>
  decode(nio::Source& in) const {
    return Base::decode<T>(in);
  }
};

} // namespace rocket::codec

// EOF
