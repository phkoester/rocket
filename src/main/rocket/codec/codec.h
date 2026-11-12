/**
 * @file codec.h
 *
 * Encoding and decoding of arbitrary C++ data structures for various purposes, such as comparisons, hashing,
 * formatting, scanning, serialization, deserialization, etc.
 *
 * # The Codec Type System {#codec_type_system}
 *
 * Value Class             | `DataType`         | C++ Mapping
 * :---------------------- | :----------------- | :----------
 * Boolean values          | `Bool`             | `bool`
 * Characters              | `Char`             | `char`, `char32`
 * Enumerations            | `Enum`             | `enum` values
 * Integer values          | `Integer`          | `i8`, `i16`, `i32`, `i64`, `i128`, `u8`, `u16`, `u32`, `u64`, `u128`
 * Floating-point values   | `Float`            | `f32`, `f64`
 * Pointers                | `Pointer`          | C++ pointers
 * Strings                 | `String`           | #std::string, #std::string_view, #std::u32string, #std::u32string_view
 * Optional values         | `Optional`         | #std::optional
 * Tuples                  | `Tuple`            | #std::pair, #std::tuple
 * Lists                   | `List`             | #std::array, #std::forward_list, #std::list, #std::span, #std::vector
 * Sets                    | `Set`              | #std::set, #std::unordered_set
 * Maps                    | `Map`              | #std::map, #std::unordered_map
 * Bidirectional maps      | `Bimap`            | #rocket::Bimap (more specifically `boost::bimaps::bimap`)
 * Durations               | `Duration`         | #std::chrono::duration
 * Hours, minutes, seconds | `HourMinuteSecond` | #std::chrono::hh_mm_ss
 * Dates                   | `Date`             | #std::chrono::year_month_day
 * Time zones              | `TimeZone`         | #rocket::chrono::time_zone (#std::chrono::time_zone if available)
 * Times                   | `Time`             | #std::chrono::time_point
 * Times with time zone    | `ZonedTime`        | #rocket::chrono::zoned_time (#std::chrono::zoned_time if available)
 * Intervals               | `Interval`         | #rocket::math::Interval
 * Declared objects        | `Declared`         | #rocket::reflect::Declared
 * Instances               | `Instance`         | #rocket::reflect::Instance
 * Members                 | `MemberRef`        | #rocket::reflect::MemberRef
 * Variables               | `VarRef`           | #rocket::reflect::VarRef
 * Code points             | `CodePoint`        | #rocket::unicode::CodePoint
 * Grapheme clusters       | `Character`        | #rocket::unicode::Character, #rocket::unicode::CharacterView
 */

#pragma once

#include "rocket/Bimap.h"
#include "rocket/InputFailure.h"
#include "rocket/assert.h"
#include "rocket/type-traits.h"
#include "rocket/chrono/chrono.h"
#include "rocket/math/Interval.h"
#include "rocket/nio/nio-utils.h"
#include "rocket/reflect/Declared.h"
#include "rocket/reflect/Instance.h"
#include "rocket/reflect/MemberRef.h"
#include "rocket/reflect/VarRef.h"
#include "rocket/str/Range.h"
#include "rocket/str/escape/escape.h"
#include "rocket/unicode/Character.h"

#include <array>
#include <chrono>
#include <forward_list>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace rocket::codec {

// `DataType` -----------------------------------------------------------------------------------------------

/**
 * Data types that consumers and producers may handle.
 *
 * Not all value types are supported by all consumers and producers.
 */
enum class DataType : u8 {
  // Basic types ............................................................................................

  /// `bool` values.
  Bool,
  /// Character values conforming to #rocket::IsChar.
  Char,
  /// Enumerations.
  Enum,
  /// Integer values conforming to #rocket::IsInteger.
  Integer,
  /// Floating-point values conforming to #rocket::IsFloat.
  Float,
  /// Pointers.
  Pointer,

  // Container types ........................................................................................

  /// Strings, either #std::string, #std::u32string, #std::string_view, or #std::u32string_view.
  String,
  /// #std::optional values.
  Optional,
  /// Tuples, either #std::pair or #std::tuple.
  Tuple,
  /// Lists: #std::array, #std::forward_list, #std::list, #std::span, or #std::vector.
  List,
  /// Sets: either #std::set or #std::unordered_set.
  Set,
  /// Maps: either #std::map or #std::unordered_map.
  Map,
  /// Bidirectional maps.
  Bimap,

  // Chronological types ....................................................................................

  /// #std::chrono::duration values.
  Duration,
  /// #std::chrono::hh_mm_ss values.
  HourMinuteSecond,
  /// #std::chrono::year_month_day values.
  Date,
  /// #rocket::chrono::time_zone values.
  TimeZone,
  /// #std::chrono::time_point values.
  Time,
  /// #rocket::chrono::zoned_time values.
  ZonedTime,

  // Mathematical types .....................................................................................

  Interval,

  // Reflection types .......................................................................................

  // An instance with default member references
  Declared,
  // An instance with specified member references
  Instance,
  /// Member references: #rocket::reflect::MemberRef.
  MemberRef,
  /// Variable references: #rocket::reflect::VarRef.
  VarRef,

  // Unicode types ..........................................................................................

  /// Code points: #rocket::unicode::CodePoint.
  CodePoint,
  /// Grapheme clusters: either #rocket::unicode::Character or #rocket::unicode::CharacterView.
  Character,
};

// `DataTypes` ----------------------------------------------------------------------------------------------

/**
 * The #rocket::codec::DataTypes template maps a C++ type to a #rocket::codec::DataType enum value, both for
 * encoding and decoding, at compile time.
 *
 * This is the central logic of the #rocket::codec type system. The encoders and decoders all rely on the
 * data types provided here.
 */
template<typename T>
struct DataTypes;

/// @spec{rocket::codec::DataTypes, bool}
template<>
struct DataTypes<bool> {
  static constexpr auto Value = DataType::Bool; ///< The value type.
};

/// @spec{rocket::codec::DataTypes, rocket::IsChar}
template<typename C> requires IsChar<C>
struct DataTypes<C> {
  static constexpr auto Value = DataType::Char; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::is_enum_v}
template<typename E> requires std::is_enum_v<E>
struct DataTypes<E> {
  static constexpr auto Value = DataType::Enum; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, rocket::IsInteger}
template<typename I> requires IsInteger<I>
struct DataTypes<I> {
  static constexpr auto Value = DataType::Integer; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, rocket::IsFloat}
template<typename F> requires IsFloat<F>
struct DataTypes<F> {
  static constexpr auto Value = DataType::Float; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::is_pointer_v}
template<typename P> requires std::is_pointer_v<P>
struct DataTypes<P> {
  static constexpr auto Value = DataType::Pointer; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::basic_string}
template<typename C> requires IsChar<C>
struct DataTypes<std::basic_string<C>> {
  static constexpr auto Value = DataType::String; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::basic_string_view}
template<typename C, typename Traits> requires IsChar<C>
struct DataTypes<std::basic_string_view<C, Traits>> {
  static constexpr auto Value = DataType::String; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::optional}
template<typename T>
struct DataTypes<std::optional<T>> {
  static constexpr auto Value = DataType::Optional; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::pair}
template<typename A, typename B>
struct DataTypes<std::pair<A, B>> {
  static constexpr auto Value = DataType::Tuple; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::tuple}
template<typename... T>
struct DataTypes<std::tuple<T...>> {
  static constexpr auto Value = DataType::Tuple; ///< The value type.
};

/// @spec{rocket::codec::DataTypes, std::array}
template<typename T, std_size_t N>
struct DataTypes<std::array<T, N>> {
  static constexpr auto Value = DataType::List; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::forward_list}
template<typename T, typename Alloc>
struct DataTypes<std::forward_list<T, Alloc>> {
  static constexpr auto Value = DataType::List; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::list}
template<typename T, typename Alloc>
struct DataTypes<std::list<T, Alloc>> {
  static constexpr auto Value = DataType::List; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::span}
template<typename T, std_size_t Extent>
struct DataTypes<std::span<T, Extent>> {
  static constexpr auto Value = DataType::List; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::vector}
template<typename T, typename Alloc>
struct DataTypes<std::vector<T, Alloc>> {
  static constexpr auto Value = DataType::List; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::set}
template<typename T, typename Compare, typename Alloc>
struct DataTypes<std::set<T, Compare, Alloc>> {
  static constexpr auto Value = DataType::Set; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::unordered_set}
template<typename T, typename Hash, typename Pred, typename Alloc>
struct DataTypes<std::unordered_set<T, Hash, Pred, Alloc>> {
  static constexpr auto Value = DataType::Set; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::map}
template<typename K, typename V, typename Compare, typename Alloc>
struct DataTypes<std::map<K, V, Compare, Alloc>> {
  static constexpr auto Value = DataType::Map; ///< The value type.
};

/// @spec{rocket::codec::DataTypes, std::unordered_map}
template<typename K, typename V, typename Hash, typename Pred, typename Alloc>
struct DataTypes<std::unordered_map<K, V, Hash, Pred, Alloc>> {
  static constexpr auto Value = DataType::Map; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, boost::bimaps::bimap}
template<typename A, typename B>
struct DataTypes<boost::bimaps::bimap<A, B>> {
  static constexpr auto Value = DataType::Bimap; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::chrono::duration}
template<typename Rep, typename Period>
struct DataTypes<std::chrono::duration<Rep, Period>> {
  static constexpr auto Value = DataType::Duration; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::chrono::hh_mm_ss}
template<typename Duration>
struct DataTypes<std::chrono::hh_mm_ss<Duration>> {
  static constexpr auto Value = DataType::HourMinuteSecond; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::chrono::year_month_day}
template<>
struct DataTypes<std::chrono::year_month_day> {
  static constexpr auto Value = DataType::Date; ///< The data type.
};

/**
 * The specific time-zone type that can be encoded and decoded.
 *
 * This is a pointer because #rocket::chrono::time_zone can’t be constructed. Its instances are completely
 * managed by #rocket::chrono::locate_zone and #rocket::chrono::current_zone.
 */
using TimeZone = const rocket::chrono::time_zone*;

/// @spec{rocket::codec::DataTypes, rocket::chrono::time_zone}
template<>
struct DataTypes<TimeZone> {
  static constexpr auto Value = DataType::TimeZone; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, std::chrono::time_point}
template<typename Clock, typename Duration>
struct DataTypes<std::chrono::time_point<Clock, Duration>> {
  static constexpr auto Value = DataType::Time; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, rocket::chrono::zoned_time}
template<typename Duration>
struct DataTypes<rocket::chrono::zoned_time<Duration>> {
  static constexpr auto Value = DataType::ZonedTime; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, rocket::math::Interval}
template<typename Left, typename Right>
struct DataTypes<rocket::math::Interval<Left, Right>> {
  static constexpr auto Value = DataType::Interval; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, rocket::reflect::Declared}
template< typename T> requires rocket::reflect::Declared<T>::value
struct DataTypes<T> {
  static constexpr auto Value = DataType::Declared; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, rocket::reflect::Instance}
template<typename T, typename Inner>
struct DataTypes<rocket::reflect::Instance<T, Inner>> {
  static constexpr auto Value = DataType::Instance; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, rocket::reflect::MemberRef}
template<typename C, typename T>
struct DataTypes<rocket::reflect::MemberRef<C, T>> {
  static constexpr auto Value = DataType::MemberRef; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, rocket::reflect::VarRef}
template<typename T>
struct DataTypes<rocket::reflect::VarRef<T>> {
  static constexpr auto Value = DataType::VarRef; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, rocket::unicode::CodePoint}
template<>
struct DataTypes<rocket::unicode::CodePoint> {
  static constexpr auto Value = DataType::CodePoint; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, rocket::unicode::Character}
template<typename C> requires IsChar<C>
struct DataTypes<rocket::unicode::Character<C>> {
  static constexpr auto Value = DataType::Character; ///< The data type.
};

/// @spec{rocket::codec::DataTypes, rocket::unicode::CharacterView}
template<typename C> requires IsChar<C>
struct DataTypes<rocket::unicode::CharacterView<C>> {
  static constexpr auto Value = DataType::Character; ///< The data type.
};

namespace internal {

// `DecoderStore` -------------------------------------------------------------------------------------------

/**
 * A store for intermediate decoder values, such as decoded string views. These are valid for the lifetime
 * of the decoder.
 */
struct DecoderStore {
  static DecoderStore* get();

  static void set(DecoderStore* val);

  const std::string&
  store(std::string&& val) {
    return *strings_.insert(std::move(val)).first;
  }

  const std::u32string&
  store(std::u32string&& val) {
    return *u32strings_.insert(std::move(val)).first;
  }

private:

   /// We need pointer stability, which #std::unordered_set guarantees.
  std::unordered_set<std::string> strings_;
  /// We need pointer stability, which #std::unordered_set guarantees.
  std::unordered_set<std::u32string> u32strings_;
};

DecoderStore* store();

void store(DecoderStore* val);

// Functions ------------------------------------------------------------------------------------------------

/**
 * Converts a count of @p Unit to the duration type @p To, checking the range.
 *
 * Like #std::chrono::duration_cast, this truncates toward zero. Unlike #std::chrono::duration_cast, an
 * integer result that does not fit into `To::rep` is reported instead of overflowing.
 *
 * @tparam To the target duration type
 * @tparam Unit the duration type that @p count is given in
 * @param count the number of units
 * @return the converted duration, or null if it is out of range
 */
template<typename To, typename Unit>
std::optional<To>
checkedDurationCast(typename To::rep count) {
  using Rep = To::rep;
  if constexpr (std::floating_point<Rep>) {
    // No undefined behavior here: a value out of range becomes infinity
    return std::chrono::duration_cast<To>(std::chrono::duration<Rep, typename Unit::period>(count));
  } else {
    static_assert(sizeof(Rep) <= 8, "128-bit duration representations are not supported");
    using Ratio = std::ratio_divide<typename Unit::period, typename To::period>;
    // `count` has at most 64 bits and `Ratio::num` at most 55 (years to nanoseconds), so this cannot
    // overflow
    const i128 val = static_cast<i128>(count) * Ratio::num / Ratio::den;
    if (val < std::numeric_limits<Rep>::min() || val > std::numeric_limits<Rep>::max()) {
      return {};
    }
    return To(static_cast<Rep>(val));
  }
}

/**
 * Makes a date from scanned values.
 *
 * @param y the year
 * @param m the month
 * @param d the day
 * @return the date, or null if the values do not form a valid date
 */
inline std::optional<std::chrono::year_month_day>
makeDate(std_int y, std_unsigned m, std_unsigned d) {
  using namespace std::chrono;

  // Check the ranges first, because `year`, `month`, and `day` truncate their arguments
  if (y < static_cast<std_int>(year::min()) || y > static_cast<std_int>(year::max()) || m > 12 || d > 31) {
    return {};
  }
  const year_month_day ret { year(y), month(m), day(d) };
  if (not ret.ok()) {
    return {};
  }
  return ret;
}

/**
 * Scans a time point in UTC in the format `%FT%T`, advancing the source past it on success.
 *
 * This deliberately avoids the scnlib scanner for #std::chrono::time_point, which has a bug (seen in commit
 * 4b8a7400): it converts the scanned fields with `std::mktime`, which takes them for local time in the zone
 * selected by the environment variable `TZ` or the system configuration. A `system_clock` time point,
 * however, is UTC, so the result is off by the local UTC offset. Adding that offset back afterward does not
 * work in general: where the fields fall into a DST change, they name a local time that does not exist or
 * exists twice, and `std::mktime` has already moved them or picked one of the two.
 *
 * Scanning `scn::datetime_components` yields the plain fields, and converting these through
 * #std::chrono::sys_days involves no time zone at all. Once scnlib is fixed (the test `scanChronoTimePoint`
 * in `test-scnlib.cc` tracks this), this function can scan the time point directly.
 *
 * @tparam Duration the duration of the time point
 * @param in the source to read from
 * @return the time point, or null if the input is not a valid date and time
 */
template<typename Duration>
std::optional<std::chrono::sys_time<Duration>>
scanSysTime(nio::Source& in) {
  using namespace std::chrono;

  auto& is = in.istream();
  const auto result = scn::scan<scn::datetime_components>(is, "{:%FT%T}");
  if (not result) {
    return {};
  }
  in.seek(io::tellg(is), nio::SeekMode::beg);

  // The format sets all of these fields, and scnlib has checked their ranges, but not the day of the month
  const auto& fields = result->value();
  const auto date = makeDate(
    *fields.year, static_cast<std_unsigned>(*fields.mon), static_cast<std_unsigned>(*fields.mday));
  if (not date) {
    return {};
  }
  const seconds time = hours(*fields.hour) + minutes(*fields.min) + seconds(*fields.sec);
  return time_point_cast<Duration>(sys_seconds(sys_days(*date)) + time);
}

/**
 * Unescapes the content of a quoted literal, reporting failures relative to the enclosing input.
 *
 * @param input the content of the literal, without its quotes
 * @param contentPos the position of @p input in the enclosing input
 * @param config the configuration
 * @param positions if nonnull, receives the positions, which are relative to @p input
 * @return the unescaped string
 * @throw #rocket::InputFailure if @p input is malformed
 */
inline std::string
unescapeLiteral(
  std::string_view input,
  u64 contentPos,
  const str::escape::CStringConfig& config = {},
  Positions* positions = nullptr) {
  try {
    return str::escape::unescapeCString(input, config, positions);
  } catch (const InputFailure& ex) {
    throw ex.shifted(contentPos);
  }
}

/**
 * Converts a count of the time unit @p unit to the duration type @p To, checking the range.
 *
 * @tparam To the target duration type
 * @param unit the time unit: `ns`, `µs`, `us`, `ms`, `s`, `min`, `h`, `d`, `w`, `m`, or `y`
 * @param count the number of units
 * @return the converted duration, or null if it is out of range
 */
template<typename To>
std::optional<To>
unitDurationCast(std::string_view unit, typename To::rep count) {
  using namespace std::chrono;

  if (unit == "ns") {
    return checkedDurationCast<To, nanoseconds>(count);
  }
  if (unit == "µs" || unit == "us") {
    return checkedDurationCast<To, microseconds>(count);
  }
  if (unit == "ms") {
    return checkedDurationCast<To, milliseconds>(count);
  }
  if (unit == "s") {
    return checkedDurationCast<To, seconds>(count);
  }
  if (unit == "min") {
    return checkedDurationCast<To, minutes>(count);
  }
  if (unit == "h") {
    return checkedDurationCast<To, hours>(count);
  }
  if (unit == "d") {
    return checkedDurationCast<To, days>(count);
  }
  if (unit == "w") {
    return checkedDurationCast<To, weeks>(count);
  }
  if (unit == "m") {
    return checkedDurationCast<To, months>(count);
  }
  if (unit == "y") {
    return checkedDurationCast<To, years>(count);
  }
  ROCKET_TERMINATE_UNREACHABLE_CODE();
}

/**
 * Checks if scanned values form a valid hour, minute, and second.
 *
 * The hour may be negative and greater than 23, but must be small enough for the sum of all parts to fit
 * into nanoseconds.
 *
 * @param h the hour
 * @param m the minute
 * @param s the second
 * @return if the values are valid
 */
inline bool
validHourMinuteSecond(std::chrono::hours::rep h, std::chrono::minutes::rep m, std::chrono::seconds::rep s) {
  using namespace std::chrono;

  const auto maxHours = duration_cast<hours>(nanoseconds::max()).count();
  return h > -maxHours && h < maxHours && m >= 0 && m < 60 && s >= 0 && s < 60;
}

/**
 * Checks if scanned values form a valid UTC offset, which is at most 23:59 in magnitude, as in RFC 3339.
 *
 * @param h the hour
 * @param m the minute
 * @return if the values are valid
 */
inline bool
validUtcOffset(u32 h, u32 m) {
  return h < 24 && m < 60;
}

} // namespace internal

// `Encoder` ------------------------------------------------------------------------------------------------

/**
 * An encoder needs a consumer that supports a set of value types.
 *
 * @tparam Consumer the consumer to use
 */
template<typename Consumer>
struct Encoder {
  /**
   * Encodes a value.
   *
   * @tparam T the type to encode
   * @tparam U the type the consumer actually consumes for `T`
   * @tparam Args types of additional arguments to pass to the consumer
   * @param val the value to encode
   * @param args additional arguments to pass to the consumer
   * @return whatever the consumer returns
   */
  template<typename T, typename U = T, typename... Args>
  auto // NOLINT
  encode(const T& val, Args&&... args) const {
    constexpr auto Value = DataTypes<Purge<T>>::Value;
    using ConsumerType = Consumer::template Type<Value, T>;
    ConsumerType consumer; // NOLINT
    return consumer.consume(val, std::forward<Args>(args)...);
  }
};

// `DecodeError`, `DecodeResult` ............................................................................

/// The error type for #rocket::codec::Decoder::decode.
struct DecodeError {
  u64 position; ///< The position in the input where the error occurred.
  str::Ranges ranges; ///< The ranges of interest in the input.
  std::string message; ///< The error message.
};

/// The result type with #rocket::codec::DecodeError.
template<typename T>
using DecodeResult = std::expected<T, DecodeError>;

namespace internal {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Moves the position and the ranges of a decode error to the starts of their grapheme clusters.
 *
 * Producers that read a source byte by byte can fail inside a cluster, for example between a digit and a
 * combining mark. Such a position is not a character boundary, which #rocket::str::location::locations
 * requires.
 *
 * This reads the entire source again, so it is meant for the error path only. The source is left where it
 * was. The error is left as it is if the source cannot seek or does not hold valid UTF-8.
 *
 * @param error the error to adjust
 * @param in the source the error refers to
 */
void snapToClusterStarts(DecodeError& error, nio::Source& in);

} // namespace internal

// `Decoder` ------------------------------------------------------------------------------------------------

/**
 * A decoder needs a producer that supports a set of value types.
 *
 * @tparam Producer the producer to use
*/
template<typename Producer>
struct Decoder {
  /**
   * Decodes a value.
   *
   * @tparam T the type to decode
   * @tparam U the type the producer actually produces for `T`
   * @tparam Args types of additional arguments to pass to the producer
   * @param val the initial value to decode into
   * @param args additional arguments to pass to the producer
   * @return the decoded value
   */
  template<typename T, typename U = T, typename... Args>
  [[nodiscard]] DecodeResult<U>
  decode(U& val, Args&&... args) const {
    internal::DecoderStore::set(const_cast<internal::DecoderStore*>(&store_));

    constexpr auto Value = DataTypes<Purge<T>>::Value;
    using ProducerType = Producer::template Type<Value, T>;
    ProducerType producer; // NOLINT
    try {
      producer.produce(val, std::forward<Args>(args)...);
    } catch (const InputFailure& ex) {
      return std::unexpected(DecodeError { ex.position(), ex.ranges(), ex.message() });
    }
    return std::move(val);
  }

private:

  internal::DecoderStore store_; ///< The store for intermediate values.
};

// `Codec` --------------------------------------------------------------------------------------------------

/**
 * A codec is both an encoder and a decoder.
 *
 * @tparam Consumer the consumer type to use
 * @tparam Producer the producer type to use
 */
template<typename Consumer, typename Producer>
struct Codec : Encoder<Consumer>, Decoder<Producer> {};

} // namespace rocket::codec

// EOF
