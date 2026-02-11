/**
 * @file codec.h
 *
 * Encoding/decoding of arbitrary C++ data structures for various purposes, such as comparisons, hashing,
 * formatting, scanning, serialization, and deserialization.
 *
 * @note For binary transmissions, the byte order in encoded form is always **little endian**. On big-endian
 *       systems, some byte swapping is necessary while encoding and decoding.
 */

#pragma once

#include "rocket/Bimap.h"
#include "rocket/type-traits.h"
#include "rocket/nio/nio-fwd.h"

#include <array>
#include <bit>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace rocket::codec {

// Mixed/middle endian is not supported, although the name PDP-11 sounds interesting ...
static_assert(
  std::endian::native == std::endian::little || std::endian::native == std::endian::big,
  "Only little-endian and big-endian architectures are supported");

constexpr bool HAS_LITTLE_ENDIAN = std::endian::native == std::endian::little;

// #ValueType -----------------------------------------------------------------------------------------------

/**
 * Value types that consumers and producers may handle.
 *
 * Not all value types are supported by all consumers and producers.
 */
enum class ValueType {
  /// `bool` values.
  boolean,
  /// Character values conforming to #rocket::IsChar.
  character,
  /// Enums.
  enumeration,
  /// Integer values conforming to #rocket::IsInteger.
  integer,
  /// Floating point-values conforming to #rocket::IsFloat.
  floatingPoint,
  /// Pointers.
  pointer,
  /// Strings.
  string,
  /// #std::optional values.
  optional,
  /// Tuples, either #std::pair or #std::tuple.
  tuple,
  /// Arrays, either #std::array or #std::vector.
  array,
  /// Sets.
  set,
  /// Maps.
  map,
  /// Bimaps.
  bimap
};

// #ValueTypes ----------------------------------------------------------------------------------------------

/**
 * The #rocket::ValueTypes template maps a C++ type to a #rocket::ValueType at compile time.
 *
 * This is the central logic for the codec system and for traversing C++ data structures of any kind. The
 * encoders and decoders all rely on the value types provided here.
 */
template<typename T>
struct ValueTypes;

template<>
struct ValueTypes<bool> {
  static constexpr auto value = ValueType::boolean;
};

template<typename C> requires IsChar<C>
struct ValueTypes<C> {
  static constexpr auto value = ValueType::character;
};

template<typename E> requires std::is_enum_v<E>
struct ValueTypes<E> {
  static constexpr auto value = ValueType::enumeration;
};

template<typename I> requires IsInteger<I>
struct ValueTypes<I> {
  static constexpr auto value = ValueType::integer;
};

template<typename F> requires IsFloat<F>
struct ValueTypes<F> {
  static constexpr auto value = ValueType::floatingPoint;
};

template<typename T> requires std::is_pointer_v<T>
struct ValueTypes<T> {
  static constexpr auto value = ValueType::pointer;
};

template<typename C> requires IsChar<C>
struct ValueTypes<std::basic_string<C>> {
  static constexpr auto value = ValueType::string;
};

template<typename C> requires IsChar<C>
struct ValueTypes<std::basic_string_view<C>> {
  static constexpr auto value = ValueType::string;
};

template<typename T>
struct ValueTypes<std::optional<T>> {
  static constexpr auto value = ValueType::optional;
};

template<typename A, typename B>
struct ValueTypes<std::pair<A, B>> {
  static constexpr auto value = ValueType::tuple;
};

template<typename... T>
struct ValueTypes<std::tuple<T...>> {
  static constexpr auto value = ValueType::tuple;
};

template<typename T, u64 N>
struct ValueTypes<std::array<T, N>> {
  static constexpr auto value = ValueType::array;
};

template<typename T>
struct ValueTypes<std::vector<T>> {
  static constexpr auto value = ValueType::array;
};

template<typename T>
struct ValueTypes<std::set<T>> {
  static constexpr auto value = ValueType::set;
};

template<typename T>
struct ValueTypes<std::unordered_set<T>> {
  static constexpr auto value = ValueType::set;
};

template<typename K, typename V>
struct ValueTypes<std::map<K, V>> {
  static constexpr auto value = ValueType::map;
};

template<typename K, typename V>
struct ValueTypes<std::unordered_map<K, V>> {
  static constexpr auto value = ValueType::map;
};

template<typename A, typename B>
struct ValueTypes<boost::bimaps::bimap<A, B>> {
  static constexpr auto value = ValueType::bimap;
};

// #Encoder -------------------------------------------------------------------------------------------------

/**
 * An encoder needs a consumer that supports a set of value types.
 *
 * @tparam Consumer the consumer to use
 */
template<typename Consumer>
struct Encoder {
  template<typename T, typename... Arg>
  auto encode(const T& val, auto&& args){
    constexpr auto valueType = ValueTypes<T>::value;
    using ConsumerType = Consumer::template Type<valueType, T>;
    ConsumerType consumer;
    return consumer.consume(val, std::forward<decltype(args)>(args));
  }
};

// #Decoder -------------------------------------------------------------------------------------------------

/**
 * A decoder needs a producer that supports a set of value types.
 *
 * @tparam Producer the producer to use
*/
template<typename Producer>
struct Decoder {
  template<typename T, typename... Arg>
  auto
  decode(nio::Source& in, Arg&&... args) {
    constexpr auto valueType = ValueTypes<T>::value;
    using ProducerType = Producer::template Type<valueType, T>;
    ProducerType producer;
    return producer.produce(in, std::forward<Arg>(args)...);
  }

  template<typename T, typename... Arg>
  auto
  tryDecode(nio::Source& in, Arg&&... args) ->
    std::optional<decltype(decode<T>(in, std::forward<Arg>(args)...))> {
    try {
      return decode<T>(in, std::forward<Arg>(args)...);
    } catch (const std::exception&) {
      return {};
    }
  }
};

// #Codec ---------------------------------------------------------------------------------------------------

/**
 * A codec is both an encoder and a decoder.
 *
 * @tparam Consumer the consumer to use
 * @tparam Producer the producer to use
 */
template<typename Consumer, typename Producer>
struct Codec : Encoder<Consumer>, Decoder<Producer> {};

} // namespace rocket::codec

// EOF
