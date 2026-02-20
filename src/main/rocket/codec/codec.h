/**
 * @file codec.h
 *
 * Encoding and decoding of arbitrary C++ data structures for various purposes, such as comparisons, hashing,
 * formatting, scanning, serialization, deserialization, etc.
 *
 * @note For binary transmissions, the byte order in encoded form is always **little endian**. On big-endian
 *       systems, some byte swapping is necessary before encoding and after decoding.
 */

#pragma once

#include "rocket/type-traits.h"
#include "rocket/reflect/Declared.h"
#include "rocket/reflect/Instance.h"
#include "rocket/reflect/MemberRef.h"
#include "rocket/reflect/VarRef.h"

#include <boost/bimap/bimap.hpp>

#include <array>
#include <bit>
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

// Mixed/middle endian is not supported, although the name PDP-11 sounds interesting ...
static_assert(
  std::endian::native == std::endian::little || std::endian::native == std::endian::big,
  "Only little-endian and big-endian architectures are supported");

/// Whether the native architecture is little-endian.
constexpr bool HAS_LITTLE_ENDIAN = std::endian::native == std::endian::little;

// #DataType ------------------------------------------------------------------------------------------------

/**
 * Data types that consumers and producers may handle.
 *
 * Not all value types are supported by all consumers and producers.
 */
enum class DataType {
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
  /// Lists: #std::array, #std::forward_list, #std::list, #std::span, or #std::vector.
  List,
  /// Sets.
  Set,
  /// Maps.
  Map,
  /// Bimaps.
  Bimap,

  // Rocket reflection ......................................................................................

  // An instance with default member references
  Declared,
  // An instance with specified member references
  Instance,
  /// Member references.
  MemberRef,
  /// Variable references.
  VarRef,
};

// #DataTypes -----------------------------------------------------------------------------------------------

/**
 * The #rocket::codec::DataTypes template maps a C++ type to a #rocket::codec::DataType enum value, both for
 * encoding and decoding, at compile time.
 *
 * This is the central logic of the codec type system. The encoders and decoders all rely on the data types
 * provided here.
 */
template<typename T>
struct DataTypes;

/// @spec{#rocket::codec::DataTypes, bool}
template<>
struct DataTypes<bool> {
  static constexpr auto Value = DataType::Bool; ///< The value type.
};

/// @spec{#rocket::codec::DataTypes, #rocket::IsChar}
template<typename C> requires IsChar<C>
struct DataTypes<C> {
  static constexpr auto Value = DataType::Char; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #std::is_enum_v}
template<typename E> requires std::is_enum_v<E>
struct DataTypes<E> {
  static constexpr auto Value = DataType::Enum; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #rocket::IsInteger}
template<typename I> requires IsInteger<I>
struct DataTypes<I> {
  static constexpr auto Value = DataType::Integer; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #rocket::IsFloat}
template<typename F> requires IsFloat<F>
struct DataTypes<F> {
  static constexpr auto Value = DataType::Float; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #std::is_pointer_v}
template<typename P> requires std::is_pointer_v<P>
struct DataTypes<P> {
  static constexpr auto Value = DataType::Pointer; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #std::basic_string}
template<typename C> requires IsChar<C>
struct DataTypes<std::basic_string<C>> {
  static constexpr auto Value = DataType::String; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #std::basic_string_view}
template<typename C, typename Traits> requires IsChar<C>
struct DataTypes<std::basic_string_view<C, Traits>> {
  static constexpr auto Value = DataType::String; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #std::optional}
template<typename T>
struct DataTypes<std::optional<T>> {
  static constexpr auto Value = DataType::Optional; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #std::pair}
template<typename A, typename B>
struct DataTypes<std::pair<A, B>> {
  static constexpr auto Value = DataType::Tuple; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #std::tuple}
template<typename... T>
struct DataTypes<std::tuple<T...>> {
  static constexpr auto Value = DataType::Tuple; ///< The value type.
};

/// @spec{#rocket::codec::DataTypes, #std::array}
template<typename T, u64 N>
struct DataTypes<std::array<T, N>> {
  static constexpr auto Value = DataType::List; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #std::forward_list}
template<typename T, typename Alloc>
struct DataTypes<std::forward_list<T, Alloc>> {
  static constexpr auto Value = DataType::List; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #std::list}
template<typename T, typename Alloc>
struct DataTypes<std::list<T, Alloc>> {
  static constexpr auto Value = DataType::List; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #std::span}
template<typename T, u64 Extent>
struct DataTypes<std::span<T, Extent>> {
  static constexpr auto Value = DataType::List; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #std::vector}
template<typename T, typename Alloc>
struct DataTypes<std::vector<T, Alloc>> {
  static constexpr auto Value = DataType::List; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #std::set}
template<typename T, typename Compare, typename Alloc>
struct DataTypes<std::set<T, Compare, Alloc>> {
  static constexpr auto Value = DataType::Set; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #std::unordered_set}
template<typename T, typename Hash, typename Pred, typename Alloc>
struct DataTypes<std::unordered_set<T, Hash, Pred, Alloc>> {
  static constexpr auto Value = DataType::Set; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #std::map}
template<typename K, typename V, typename Compare, typename Alloc>
struct DataTypes<std::map<K, V, Compare, Alloc>> {
  static constexpr auto Value = DataType::Map; ///< The value type.
};

/// @spec{#rocket::codec::DataTypes, #std::unordered_map}
template<typename K, typename V, typename Hash, typename Pred, typename Alloc>
struct DataTypes<std::unordered_map<K, V, Hash, Pred, Alloc>> {
  static constexpr auto Value = DataType::Map; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, boost::bimaps::bimap}
template<typename A, typename B>
struct DataTypes<boost::bimaps::bimap<A, B>> {
  static constexpr auto Value = DataType::Bimap; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #rocket::reflect::Declared}
template< typename T> requires rocket::reflect::Declared<T>::value
struct DataTypes<T> {
  static constexpr auto Value = DataType::Declared; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #rocket::reflect::Instance}
template<typename T, typename Inner>
struct DataTypes<rocket::reflect::Instance<T, Inner>> {
  static constexpr auto Value = DataType::Instance; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #rocket::reflect::MemberRef}
template<typename C, typename T>
struct DataTypes<rocket::reflect::MemberRef<C, T>> {
  static constexpr auto Value = DataType::MemberRef; ///< The data type.
};

/// @spec{#rocket::codec::DataTypes, #rocket::reflect::VarRef}
template<typename T>
struct DataTypes<rocket::reflect::VarRef<T>> {
  static constexpr auto Value = DataType::VarRef; ///< The data type.
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
    constexpr auto Value = DataTypes<T>::Value;
    using ConsumerType = Consumer::template Type<Value, T>;
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
   * Decodes a value.
   *
   * @tparam T the type to decode
   * @tparam Args types of additional arguments to pass to the producer
   * @param args additional arguments to pass to the producer
   * @return the decoded value
   */
  template<typename T, typename... Args>
  T
  decode(Args&&... args) const {
    constexpr auto Value = DataTypes<T>::Value;
    using ProducerType = Producer::template Type<Value, T>;
    ProducerType producer;
    T val;
    producer.produce(val, std::forward<Args>(args)...);
    return val;
  }

  /**
   * Tries to decode a value.
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
struct Codec : Encoder<Consumer>, Decoder<Producer> {};

} // namespace rocket::codec

// EOF
