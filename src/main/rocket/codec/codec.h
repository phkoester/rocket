/**
 * @file codec.h
 *
 * Encoding and decoding of arbitrary C++ data structures for various purposes, such as comparisons, hashing,
 * formatting, scanning, serialization, deserialization, etc.
 *
 * @note For binary transmissions, the byte order in encoded form is always **little endian**. On big-endian
 *       systems, some byte swapping is necessary while encoding and decoding.
 */

#pragma once

#include "rocket/type-traits.h"
#include "rocket/reflect/MemberRef.h"
#include "rocket/reflect/VarRef.h"

#include <boost/bimap/bimap.hpp>

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

/// Whether the native architecture is little-endian.
constexpr bool HAS_LITTLE_ENDIAN = std::endian::native == std::endian::little;

// #ValueType -----------------------------------------------------------------------------------------------

/**
 * Value types that consumers and producers may handle.
 *
 * Not all value types are supported by all consumers and producers.
 */
enum class ValueType {
  // Basic types ............................................................................................

  /// `bool` values.
  Bool,
  /// Character values conforming to #rocket::IsChar.
  Char,
  /// Enums.
  Enum,
  /// Integer values conforming to #rocket::IsInteger.
  Integer,
  /// Floating point-values conforming to #rocket::IsFloat.
  Float,
  /// Pointers.
  Pointer,

  // Container types ........................................................................................

  /// Strings, either #std::string or #std::string_view.
  String,
  /// #std::optional values.
  Optional,
  /// Tuples, either #std::pair or #std::tuple.
  Tuple,
  /// Arrays, either #std::array or #std::vector.
  Array,
  /// Sets.
  Set,
  /// Maps.
  Map,
  /// Bimaps.
  Bimap,

  // Rocket reflection ......................................................................................

  /// Classes holding member references.
  MemberRefProvider,
  /// Member references.
  MemberRef,
  /// Variable references.
  VarRef,
};

// #ValueTypes ----------------------------------------------------------------------------------------------

/**
 * The #rocket::codec::ValueTypes template maps a C++ type to a #rocket::codec::ValueType at compile time.
 *
 * This is the central logic for the codec system and for traversing C++ data structures of any kind. The
 * encoders and decoders all rely on the value types provided here.
 */
template<typename T>
struct ValueTypes;

/// @spec{#rocket::codec::ValueTypes, bool}
template<>
struct ValueTypes<bool> {
  static constexpr auto value = ValueType::Bool; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, C}
template<typename C> requires IsChar<C>
struct ValueTypes<C> {
  static constexpr auto value = ValueType::Char; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, E}
template<typename E> requires std::is_enum_v<E>
struct ValueTypes<E> {
  static constexpr auto value = ValueType::Enum; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, I}
template<typename I> requires IsInteger<I>
struct ValueTypes<I> {
  static constexpr auto value = ValueType::Integer; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, F}
template<typename F> requires IsFloat<F>
struct ValueTypes<F> {
  static constexpr auto value = ValueType::Float; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, T}
template<typename T> requires std::is_pointer_v<T>
struct ValueTypes<T> {
  static constexpr auto value = ValueType::Pointer; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, #std::basic_string}
template<typename C> requires IsChar<C>
struct ValueTypes<std::basic_string<C>> {
  static constexpr auto value = ValueType::String; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, #std::basic_string_view}
template<typename C> requires IsChar<C>
struct ValueTypes<std::basic_string_view<C>> {
  static constexpr auto value = ValueType::String; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, #std::optional}
template<typename T>
struct ValueTypes<std::optional<T>> {
  static constexpr auto value = ValueType::Optional; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, #std::pair}
template<typename A, typename B>
struct ValueTypes<std::pair<A, B>> {
  static constexpr auto value = ValueType::Tuple; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, #std::tuple}
template<typename... T>
struct ValueTypes<std::tuple<T...>> {
  static constexpr auto value = ValueType::Tuple; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, #std::array}
template<typename T, u64 N>
struct ValueTypes<std::array<T, N>> {
  static constexpr auto value = ValueType::Array; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, #std::vector}
template<typename T>
struct ValueTypes<std::vector<T>> {
  static constexpr auto value = ValueType::Array; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, #std::set}
template<typename T>
struct ValueTypes<std::set<T>> {
  static constexpr auto value = ValueType::Set; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, #std::unordered_set}
template<typename T>
struct ValueTypes<std::unordered_set<T>> {
  static constexpr auto value = ValueType::Set; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, #std::map}
template<typename K, typename V>
struct ValueTypes<std::map<K, V>> {
  static constexpr auto value = ValueType::Map; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, #std::unordered_map}
template<typename K, typename V>
struct ValueTypes<std::unordered_map<K, V>> {
  static constexpr auto value = ValueType::Map; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, boost::bimaps::bimap}
template<typename A, typename B>
struct ValueTypes<boost::bimaps::bimap<A, B>> {
  static constexpr auto value = ValueType::Bimap; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, #rocket::reflect::MemberRefProvider}
template< typename T> requires rocket::reflect::MemberRefProvider<T>::value
struct ValueTypes<T> {
  static constexpr auto value = ValueType::MemberRefProvider; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, #rocket::reflect::MemberRef}
template<typename C, typename T>
struct ValueTypes<rocket::reflect::MemberRef<C, T>> {
  static constexpr auto value = ValueType::MemberRef; ///< The value type.
};

/// @spec{#rocket::codec::ValueTypes, #rocket::reflect::VarRef}
template<typename T>
struct ValueTypes<rocket::reflect::VarRef<T>> {
  static constexpr auto value = ValueType::VarRef; ///< The value type.
};

// #Encoder -------------------------------------------------------------------------------------------------

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
   * @tparam Args types of additional arguments to pass to the consumer
   * @param val the value to encode
   * @param args additional arguments to pass to the consumer
   * @return whatever the consumer returns
   */
  template<typename T, typename... Args>
  auto
  encode(const T& val, Args&&... args) const {
    constexpr auto valueType = ValueTypes<T>::value;
    using ConsumerType = Consumer::template Type<valueType, T>;
    ConsumerType consumer;
    return consumer.consume(val, std::forward<Args>(args)...);
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
  /**
   * Decodes a value from a source.
   *
   * @tparam T the type to decode
   * @tparam Args types of additional arguments to pass to the producer
   * @param args additional arguments to pass to the producer
   * @return the decoded value
   */
  template<typename T, typename... Args>
  T
  decode(Args&&... args) const {
    constexpr auto valueType = ValueTypes<T>::value;
    using ProducerType = Producer::template Type<valueType, T>;
    ProducerType producer;
    T val;
    producer.produce(val, std::forward<Args>(args)...);
    return val;
  }

  /**
   * Tries to decode a value from a source.
   *
   * @tparam T the type to decode
   * @tparam Args types of additional arguments to pass to the producer
   * @param args additional arguments to pass to the producer
   * @return the decoded value, or null if the value cannot be decoded
   */
  template<typename T, typename... Args>
  [[nodiscard]] std::optional<T>
  tryDecode(Args&&... args) const {
    try {
      return decode(std::forward<Args>(args)...);
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
struct Codec : Encoder<Consumer>, Decoder<Producer> {
  using ConsumerType = Consumer; ///< @type_alias
  using ProducerType = Producer; ///< @type_alias
};

} // namespace rocket::codec

// EOF
