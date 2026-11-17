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
 * The encoder writes standard JSON, except for the floating-point values `-Infinity`, `Infinity`, and `NaN`.
 * The decoder accepts those same floating-point literals, C-style comments, and a trailing comma after the
 * last element of an array or object.
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
 * end with <code>*</code><code>/</code>.
 *
 * Comments are never written.
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
 * Characters are JSON strings of length one. Example values: `"a"`, `"\t"`, `"€"`, `"\u20AC"`.
 *
 * One-byte characters must be valid ASCII characters in the range [0,127]. Four-byte characters must be
 * valid Unicode code points in the ranges [U+0000,U+D7FF] and [U+E000,U+10FFFF].
 *
 * When reading, escape sequences as well as the prefixes `\x` (two hexadecimal digits), `\u` (four
 * hexadecimal digits), and `\U` (eight hexadecimal digits) are accepted. `\/` is accepted as well. A `\u`
 * high surrogate must be followed by a `\u` low surrogate; together, they yield a single code point.
 *
 * When writing, the escape sequences `\b`, `\t`, `\n`, `\f`, `\r`, <code>\\\"</code>, and `\\` are used. If
 * a character is classified as printable by the Unicode standard, it appears verbatim in the output, e.g.,
 * as `"a"` or `"€"`. Otherwise, it is written in hexadecimal notation, using the prefix `\u` (four
 * hexadecimal digits). A character greater than U+FFFF is then written as a `\u` high surrogate followed by
 * a `\u` low surrogate.
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
 * and written verbatim, except that a quote or backslash inside a cluster is escaped.
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
 * Lists are JSON arrays. Example values: `[1, 2, 3]`, `["one", "two", "three"]`.
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
 * `{ "alpha": 1, "beta": 2 }`.
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
 * Durations are JSON strings in the notation of {fmt}: a count, directly followed by a unit. Example values:
 * `"5ns"`, `"1700µs"`, `"90min"`, `"2[604800]s"` (two weeks).
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
 * Example values: `"01:02:03"`, `"01:02:03.123"`, `"-111:02:03.123456789"`.
 *
 * ## Dates
 *
 * Example values: `"1970-01-02"`, `"-100-01-02"`.
 *
 * Dates are written by {fmt} and read with the scnlib format `%F`. Years from -32767 through 9999 can be
 * read; a nonnegative year has exactly four digits.
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
 * The UTC offset is `Z` for +00:00. When reading, the offset must be one that the time zone has at that
 * local time; it selects between the two meanings of an ambiguous local time.
 *
 * ## Intervals
 *
 * Intervals are JSON strings: `∅` for an empty interval, otherwise the two bounds separated by a comma,
 * enclosed in `[` or `]` for a closed bound and `(` or `)` for an open bound. A missing bound is notated as
 * `-∞` or `∞`.
 *
 * Example values: `"∅"`, `"[-1.2,3.4]"`, `"(-1.2,3.4)"`, `"(-∞,-12]"`, `"[12,∞)"`.
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
 * Duplicate member names are discouraged but allowed. If a name appears multiple times, the last one wins.
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
 * Code points appear as JSON strings in the typical Unicode notation, e.g., `"U+0041"` or `"U+10FFFF"`.
 *
 * A JSON string of a single character is also accepted when reading.
 *
 * ## Grapheme Clusters
 *
 * Grapheme clusters appear just like strings.
 */

#pragma once

#include "rocket/InputFailure.h"
#include "rocket/TypeId-format.h"
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

// `JsonConsumerConfig` -------------------------------------------------------------------------------------

/// Configuration for #rocket::codec::JsonConsumer.
struct JsonConsumerConfig {
  /// Whether to indent the output and format a tree.
  bool indent = false;
  /// The current level of indentation.
  u64 level = 0;
};

namespace internal::json {

// Functions ------------------------------------------------------------------------------------------------

inline void
beginContainer(nio::Sink& out, JsonConsumerConfig& config, char c) {
  rocket::codec::internal::beginContainer(out, config.indent, config.level, c);
}

inline void
endContainer(nio::Sink& out, JsonConsumerConfig& config, u64 size, char c) {
  rocket::codec::internal::endContainer(out, config.indent, config.level, size, c);
}

/**
 * Reports whether @p T is a tuple of member or variable references.
 *
 * Such a tuple is a JSON object. Every other tuple is a JSON array.
 */
template<typename T, std_size_t... Index>
constexpr bool
isObjectTuple([[maybe_unused]] std::index_sequence<Index...> seq) {
  return ((DataTypes<std::tuple_element_t<Index, T>>::Value == DataType::MemberRef ||
           DataTypes<std::tuple_element_t<Index, T>>::Value == DataType::VarRef) && ...);
}

/**
 * Translates an offset in the unescaped content of a JSON string to a position in the enclosing input.
 *
 * @param pos the position of the opening quote of the string, or an empty optional if it is unknown
 * @param unescapedOffset the offset in the unescaped content
 * @param positions the positions recorded while unescaping
 * @return the position in the enclosing input, or an empty optional if @p pos is empty
 */
std::optional<u64> jsonStringOffset(std::optional<u64> pos, u64 unescapedOffset, const Positions& positions);

inline void
nextElem(nio::Sink& out, JsonConsumerConfig& config, u64 index) {
  rocket::codec::internal::nextElem(out, config.indent, config.level, index);
}

std::string readJsonString(nio::Source& in, Positions& positions);

/**
 * Checks that an inner source over the content of a JSON string has been consumed, apart from whitespace.
 *
 * @tparam InPos the type of @p inPos
 * @param inner the inner source
 * @param size the size of the content of the inner source
 * @param inPos a function that translates a position in @p inner to a position in the outer source
 * @throw #rocket::InputFailure if unconsumed input remains
 */
template<typename InPos>
void
expectEndOfString(nio::ContiguousSource& inner, u64 size, const InPos& inPos) {
  rocket::nio::skip(inner, false, false);
  const u64 innerPos = inner.position();
  if (innerPos != size) {
    throw InputFailure::withRange(
      inPos(innerPos), inPos(innerPos), inPos(size), "unexpected input after value");
  }
}

inline void
skip(nio::Source& in) {
  rocket::nio::skip(in, true, false);
}

// `JsonConsumerImpl` ---------------------------------------------------------------------------------------

/// @cond undocumented
#define CONFIG__ [[maybe_unused]] JsonConsumerConfig& config
/// @endcond

template<DataType DataType, typename T>
struct JsonConsumerImpl;

template<typename T>
void
writeJsonString(const T& val, nio::Sink& out) {
  using Type = Purge<T>;
  constexpr auto DataType = DataTypes<Type>::Value;

  nio::StringSink inner;
  JsonConsumerConfig config;
  JsonConsumerImpl<DataType, Type>().consume(val, inner, config);
  if constexpr (DataType == DataType::String) {
    out.write(inner.str());
  } else {
    const std::string escaped = str::escape::escapeCString(inner.str(), { .json=true, .quote='"' });
    out.write(escaped);
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
    const std::string escaped = str::escape::escapeCString(utf8, { .json=true, .quote='"' });
    out.write(escaped);
  }
};

template<typename E>
struct JsonConsumerImpl<DataType::Enum, E> {
  void
  consume(E val, nio::Sink& out, CONFIG__) const {
    if constexpr (fmt::is_formattable<E>::value) {
      out.print("\"{}\"", val);
      return;
    }
    ROCKET_FAIL("cannot format enum of type {}", TypeId::of<E>());
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
    out.print("\"0x{:X}\"", reinterpret_cast<std::uintptr_t>(static_cast<const void*>(val)));
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::String, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    const std::string utf8(unicode::ConvertTo<char>::apply(val));
    const std::string escaped = str::escape::escapeCString(utf8, { .json=true, .quote='"' });
    out.print("{}", escaped);
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

// For `MemberRef`, the tuple consumer must be able to pass additional arguments to the element consumer
template<typename T>
struct JsonConsumerImpl<DataType::Tuple, T> {
  static constexpr bool Object = isObjectTuple<T>(std::make_index_sequence<std::tuple_size_v<T>>());

  template<typename... Args>
  void
  consume(const T& val, nio::Sink& out, CONFIG__, Args&&... args) const {
    beginContainer(out, config, Object ? '{' : '[');
    u64 index = 0;
    std::apply([&](auto&&... arg) {
      (consumeElem(std::forward<decltype(arg)>(arg),
        out, config, index++, std::forward<Args>(args)...), ...);
    }, val);
    endContainer(out, config, std::tuple_size_v<T>, Object ? '}' : ']');
  }

private:

  template<typename Elem, typename... Args>
  void
  consumeElem(const Elem& elem, nio::Sink& out, CONFIG__, u64 index, Args&&... args) const {
    nextElem(out, config, index);
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    JsonConsumerImpl<ElemDataType, Elem>().consume(elem, out, config, std::forward<Args>(args)...);
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
      writeJsonString(key, out);
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
      writeJsonString(key, out);
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
    // The notation contains neither quotes nor backslashes, so it needs no escaping
    out.print("\"{}\"", val);
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::Date, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.write(fmt::format("\"{}\"", val));
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::HourMinuteSecond, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.print("\"{}\"", val);
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::TimeZone, T> {
  void
  consume(T val, nio::Sink& out, CONFIG__) const {
    out.print("\"{}\"", val->name());
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::Time, T> {
  using Clock = T::clock;
  static_assert(std::same_as<Clock, std::chrono::system_clock>, "clock must be `system_clock`");

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.write(fmt::format("\"{:%FT%TZ}\"", val)); // Zulu time
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::ZonedTime, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    const auto* tz = val.get_time_zone();
    if (val.get_info().offset == std::chrono::seconds(0)) {
      out.print("\"{:%FT%TZ} ({})\"", val, tz->name()); // Zulu time
    } else {
      out.print("\"{:%FT%T%Ez} ({})\"", val, tz->name()); // Time with UTC offset
    }
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::Interval, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    // The notation contains neither quotes nor backslashes, so it needs no escaping
    out.write(fmt::format("\"{}\"", val));
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::Declared, T> {
  static constexpr auto& refs = rocket::reflect::Declared<T>::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    // An empty member list has no element type to recognize as an object tuple
    if constexpr (std::tuple_size_v<Elem> == 0) {
      beginContainer(out, config, '{');
      endContainer(out, config, 0, '}');
      return;
    }
    // Here we have to pass an additional argument, the instance, to the tuple consumer. The tuple consumer
    // will pass it on to the member-reference consumer
    JsonConsumerImpl<ElemDataType, Elem>().consume(refs, out, config, val);
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
    // An empty member list has no element type to recognize as an object tuple
    if constexpr (std::tuple_size_v<Elem> == 0) {
      beginContainer(out, config, '{');
      endContainer(out, config, 0, '}');
      return;
    }
    // Here we have to pass an additional argument, the instance, to the tuple consumer. The tuple consumer
    // will pass it on to the member-reference consumer
    JsonConsumerImpl<ElemDataType, Elem>().consume(refs, out, config, val.get());
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::MemberRef, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  template<typename C>
  void
  consume(const T& val, nio::Sink& out, CONFIG__, const C& instance) const {
    out.print("\"{}\": ", val.name());
    JsonConsumerImpl<ElemDataType, Elem>().consume(val.get(instance), out, config);
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::VarRef, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.print("\"{}\": ", val.name());
    JsonConsumerImpl<ElemDataType, Elem>().consume(val.get(), out, config);
  }
};

template<typename T>
struct JsonConsumerImpl<DataType::CodePoint, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.print("\"{}\"", val);
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
  if constexpr (KeyDataType == DataType::String) {
    JsonProducerImpl<KeyDataType, Key>().produce(key, in);
  } else {
    const auto pos = in.tell();

    // Read JSON string and create an inner source for it
    Positions positions;
    const auto input = readJsonString(in, positions);
    nio::StringSource inner(input);
    const auto inPos = [pos, &positions](u64 innerPos) -> std::optional<u64> {
      return jsonStringOffset(pos, innerPos, positions);
    };

    try {
      JsonProducerImpl<KeyDataType, Key>().produce(key, inner);
    } catch (const InputFailure& ex) {
      throw ex.mapped(inPos);
    }
    expectEndOfString(inner, input.size(), inPos);
  }
}

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
produceMembers(const Refs& refs, C& instance, nio::Source& in) {
  skip(in);
  const auto pos = in.tell();

  if (not readChar(in, '{')) {
    throw InputFailure(pos, "expected an object");
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

    // Read the member name

    const auto namePos = in.tell();
    std::string name;
    readJsonKey(name, in);
    skip(in);

    expectColon(in);
    skip(in);

    // Look up the member reference by name and produce the member

    const bool found = std::apply([&](const auto&... ref) {
      return (produceMember(ref, name, instance, in) || ...);
    }, refs);
    if (not found) {
      throw InputFailure(namePos, fmt::format("unknown member `{}`", name));
    }
  }
}

template<>
struct JsonProducerImpl<DataType::Bool, bool> {
  void
  produce(bool& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (readString(in, "false")) {
      val = false;
      return;
    }
    if (readString(in, "true")) {
      val = true;
      return;
    }
    throw InputFailure(pos, "expected a boolean value");
  }
};

template<typename C>
struct JsonProducerImpl<DataType::Char, C> {
  void
  produce(C& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (not readChar(in, '"')) {
      throw InputFailure(pos, "expected a character");
    }
    auto input = readUntilUnescapedChar(in, '"');
    if (not input) {
      throw InputFailure(pos, "unterminated character literal");
    }

    const std::string unescaped = unescapeLiteral(*input, pos, { .json=true });
    const std::basic_string<C> str(unicode::ConvertTo<C>::apply(unescaped));
    if (str.size() != 1) {
      throw InputFailure(pos, "invalid character literal");
    }
    val = str[0];
  }
};

template<typename E>
struct JsonProducerImpl<DataType::Enum, E> {
  void
  produce(E& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if constexpr (scn::detail::is_scannable<E, char>::value) {
      // Read JSON string and create an inner source for it
      Positions positions;
      const auto input = readJsonString(in, positions);
      nio::StringSource inner(input);
      const auto inPos = [pos, &positions](u64 innerPos) -> std::optional<u64> {
        return jsonStringOffset(pos, innerPos, positions);
      };

      const auto result = inner.scan<E>("{}");
      if (result) {
        expectEndOfString(inner, input.size(), inPos);
        val = result->value();
        return;
      }
      throw InputFailure(inPos(0), fmt::format("invalid value for enum {}", TypeId::of<E>()));
    }
    throw InputFailure(pos, fmt::format("cannot scan enum of type {}", TypeId::of<E>()));
  }
};

template<typename I>
struct JsonProducerImpl<DataType::Integer, I> {
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
struct JsonProducerImpl<DataType::Float, F> {
  using Limits = std::numeric_limits<F>;

  void
  produce(F& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (readString(in, "-Infinity")) {
      val = -Limits::infinity();
      return;
    }
    if (readChoice(in, { "Infinity", "+Infinity" })) {
      val = Limits::infinity();
      return;
    }
    if (readString(in, "NaN")) {
      val = Limits::quiet_NaN();
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
struct JsonProducerImpl<DataType::Pointer, P> {
  void
  produce(P& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (readString(in, "null")) {
      val = nullptr;
      return;
    }

    // Read JSON string and create an inner source for it
    Positions positions;
    const auto input = readJsonString(in, positions);
    nio::StringSource inner(input);
    const auto inPos = [pos, &positions](u64 innerPos) -> std::optional<u64> {
      return jsonStringOffset(pos, innerPos, positions);
    };

    const auto result = inner.scan<P>("{}");
    if (result) {
      expectEndOfString(inner, input.size(), inPos);
      val = result->value();
      return;
    }
    throw InputFailure(inPos(0), "expected a pointer value");
  }
};

template<typename T>
struct JsonProducerImpl<DataType::String, T> {
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

    const std::string unescaped = unescapeLiteral(*input, pos, { .json=true });
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
struct JsonProducerImpl<DataType::Optional, T> {
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
    JsonProducerImpl<ElemDataType, Elem>().produce(*val, in);
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Tuple, T> {
  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (not readChar(in, '[')) {
      throw InputFailure(pos, "expected an array");
    }

    u64 index = 0;
    std::apply([&](auto&&... arg) {
      (produceElem(std::forward<decltype(arg)>(arg), in, index++), ...);
    }, val);

    skip(in);
    if (std::tuple_size_v<T> > 0 && readChar(in, ',')) { // Allow trailing comma if nonempty
      skip(in);
    }
    if (not readChar(in, ']')) {
      const auto end = in.tell();
      throw InputFailure::withRange(end, pos, end, "unterminated array");
    }
  }

private:

  template<typename Elem>
  void
  produceElem(Elem& elem, nio::Source& in, u64 index) const {
    skip(in);
    if (index > 0) {
      expectComma(in);
      skip(in);
    }
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    JsonProducerImpl<ElemDataType, Elem>().produce(elem, in);
  }
};

template<typename T>
struct JsonProducerImpl<DataType::List, T> {
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
      throw InputFailure(pos, "expected an array");
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
      JsonProducerImpl<ElemDataType, Elem>().produce(val[index], in);
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
    skip(in);
    const auto pos = in.tell();

    val.clear();
    if (not readChar(in, '[')) {
      throw InputFailure(pos, "expected an array");
    }

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
    skip(in);
    const auto pos = in.tell();

    val.clear();

    if (not readChar(in, '{')) {
      throw InputFailure(pos, "expected an object");
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
      readJsonKey(key, in);
      skip(in);

      expectColon(in);
      skip(in);

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
    skip(in);
    const auto pos = in.tell();

    val.clear();

    if (not readChar(in, '{')) {
      throw InputFailure(pos, "expected an object");
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
      readJsonKey(key, in);
      skip(in);

      expectColon(in);
      skip(in);

      Elem elem;
      JsonProducerImpl<ElemDataType, Elem>().produce(elem, in);
      // Make the last one win. Both the key and the element must be unique, so remove any pair with either
      val.left.erase(key);
      val.right.erase(elem);
      val.insert({ std::move(key), std::move(elem) });
    }
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Duration, T> {
  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    // Read JSON string and create an inner source for it
    Positions positions;
    const auto input = readJsonString(in, positions);
    nio::StringSource inner(input);
    const auto inPos = [pos, &positions](u64 innerPos) -> std::optional<u64> {
      return jsonStringOffset(pos, innerPos, positions);
    };

    auto result = inner.scan<T>("{}");
    if (not result) {
      throw InputFailure(inPos(0), durationError(result.error()));
    }
    val = result->value();
    expectEndOfString(inner, input.size(), inPos);
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Date, T> {
  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    // Read JSON string and create an inner source for it
    Positions positions;
    const auto input = readJsonString(in, positions);
    nio::StringSource inner(input);
    const auto inPos = [pos, &positions](u64 innerPos) -> std::optional<u64> {
      return jsonStringOffset(pos, innerPos, positions);
    };

    auto result = inner.scan<T>("{:%F}");
    if (not result) {
      throw InputFailure(inPos(0), dateError(result.error()));
    }
    // A digit after the date means that a field has too many digits, e.g., `2026-01-257`
    const auto next = inner.peek();
    if (not result->value().ok() || (next && *next >= '0' && *next <= '9')) {
      throw InputFailure(inPos(0), "invalid date");
    }
    val = result->value();
    expectEndOfString(inner, input.size(), inPos);
  }
};

template<typename T>
struct JsonProducerImpl<DataType::HourMinuteSecond, T> {
  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    // Read JSON string and create an inner source for it
    Positions positions;
    const auto input = readJsonString(in, positions);
    nio::StringSource inner(input);
    const auto inPos = [pos, &positions](u64 innerPos) -> std::optional<u64> {
      return jsonStringOffset(pos, innerPos, positions);
    };

    auto result = inner.scan<T>("{}");
    if (not result) {
      throw InputFailure(inPos(0), hourMinuteSecondError(result.error()));
    }
    val = result->value();
    expectEndOfString(inner, input.size(), inPos);
  }
};

template<typename T>
struct JsonProducerImpl<DataType::TimeZone, T> {
  void
  produce(T& val, nio::Source& in) const {
    using namespace std::chrono;

    skip(in);
    const auto pos = in.tell();

    // Read JSON string (no inner source needed here because we use `input` directly)
    Positions positions;
    const auto input = readJsonString(in, positions);
    const auto inPos = [pos, &positions](u64 innerPos) -> std::optional<u64> {
      return jsonStringOffset(pos, innerPos, positions);
    };

    try {
      // This throws if the time zone is not found
      val = rocket::chrono::locateUnambiguousZone(input);
    } catch (const std::exception& ex) {
      throw InputFailure(inPos(0), ex.what());
    }
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Time, T> {
  using Clock = T::clock;
  static_assert(std::same_as<Clock, std::chrono::system_clock>, "clock must be `system_clock`");

  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    // Read JSON string and create an inner source for it
    Positions positions;
    const auto input = readJsonString(in, positions);
    nio::StringSource inner(input);
    const auto inPos = [pos, &positions](u64 innerPos) -> std::optional<u64> {
      return jsonStringOffset(pos, innerPos, positions);
    };

    // The patched scnlib reads optional subseconds with `%.S`
    auto result = inner.scan<T>("{:%FT%H:%M:%.SZ}");
    if (not result) {
      throw InputFailure(inPos(0), "expected a time point");
    }
    val = result->value();
    expectEndOfString(inner, input.size(), inPos);
  }
};

template<typename T>
struct JsonProducerImpl<DataType::ZonedTime, T> {
  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    // Read JSON string and create an inner source for it
    Positions positions;
    const auto input = readJsonString(in, positions);
    nio::StringSource inner(input);
    const auto inPos = [pos, &positions](u64 innerPos) -> std::optional<u64> {
      return jsonStringOffset(pos, innerPos, positions);
    };

    // The patched scnlib reads optional subseconds with `%.S` and `Z` as a UTC offset with `%Ez`
    auto result = inner.scan<T>("{:%FT%H:%M:%.S%Ez (%Z)}");
    if (not result) {
      throw InputFailure(inPos(0), zonedTimeError(result.error()));
    }
    val = result->value();
    expectEndOfString(inner, input.size(), inPos);
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Interval, T> {
  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    // Read JSON string and create an inner source for it
    Positions positions;
    const auto input = readJsonString(in, positions);
    nio::StringSource inner(input);
    const auto inPos = [pos, &positions](u64 innerPos) -> std::optional<u64> {
      return jsonStringOffset(pos, innerPos, positions);
    };

    auto result = inner.scan<T>("{}");
    if (not result) {
      throw InputFailure(inPos(0), "expected an interval");
    }
    val = result->value();
    expectEndOfString(inner, input.size(), inPos);
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
    produceMembers(refs, val, in);
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
    produceMembers(refs, val.get(), in);
  }
};

template<typename T>
struct JsonProducerImpl<DataType::MemberRef, T> {
  static_assert(false, "cannot decode member reference");
};

template<typename T>
struct JsonProducerImpl<DataType::VarRef, T> {
  static_assert(false, "cannot decode variable reference");
};

template<typename T>
struct JsonProducerImpl<DataType::CodePoint, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (not readChar(in, '"')) {
      throw InputFailure(pos, "expected a code point");
    }

    if (not readString(in, "U+", false, false)) {
      // Read `char32`
      in.seek(-1, nio::SeekMode::cur);
      Elem elem = Elem();
      JsonProducerImpl<ElemDataType, Elem>().produce(elem, in);
      val = T(elem);
      return;
    }

    // Read `U+...`
    in.seek(-2, nio::SeekMode::cur);
    auto result = in.scan<T>("{}");
    if (not result) {
      throw InputFailure(pos, result.error().msg());
    }
    if (not readChar(in, '"')) {
      const auto end = in.tell();
      throw InputFailure::withRange(end, pos, end, "unterminated code point");
    }
    val = result->value();
  }
};

template<typename T>
struct JsonProducerImpl<DataType::Character, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    Elem elem;
    JsonProducerImpl<ElemDataType, Elem>().produce(elem, in);
    if (elem.empty()) {
      throw InputFailure(pos, "expected a character");
    }
    val = T(std::move(elem));
  }
};

#undef CONFIG__

} // namespace internal::json

// `JsonConsumer` -------------------------------------------------------------------------------------------

/// The consumer for #rocket::codec::JsonCodec.
struct JsonConsumer {
  /// @type_alias
  template<DataType DataType, typename T>
  using Type = internal::json::JsonConsumerImpl<DataType, T>;
};

// `JsonProducer` -------------------------------------------------------------------------------------------

/// The producer for #rocket::codec::JsonCodec.
struct JsonProducer {
  /// @type_alias
  template<DataType DataType, typename T>
  using Type = internal::json::JsonProducerImpl<DataType, T>;
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
 * allowed. This is made possible by keeping real strings in a store bound to the decoder. Hence, decoded
 * string views and character views are valid, and valid only, for the lifetime of the decoder.
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
