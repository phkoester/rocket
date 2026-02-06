/**
 * @file codec.h
 *
 * Rocket encoding/decoding.
 *
 * @note For binary transmissions, the byte order in encoded form is always **little endian**.
 */

#pragma once

#include <bit>
#include <optional>

namespace rocket::codec {

// Mixed/middle endian is not supported, even though PDP-11 sounds interesting ...
static_assert(
  std::endian::native == std::endian::little || std::endian::native == std::endian::big,
  "Only little-endian and big-endian architectures are supported");

constexpr bool HAS_LITTLE_ENDIAN = std::endian::native == std::endian::little;

// Functions ------------------------------------------------------------------------------------------------

/**
 * Decodes a value of type @p T, using a specific codec and reading from a view.
 *
 * @tparam Codec the codec to use
 * @tparam T the type to decode
 * @param val a view on the encoded value
 * @return the decoded value
 * @throw #std::exception if the decoding fails
 */
template<typename Codec, typename T>
T
decode(typename Codec::EncodedViewType val) {
  return Codec::template decode<T>(val, 0).first;
}

/**
 * Encodes a value of type @p T, using a specific codec.
 *
 * @tparam Codec the codec to use
 * @tparam T the type to encode
 * @param val the value to encode
 * @return the encoded value
 */
template<typename Codec, typename T>
Codec::EncodedType
encode(const T& val) {
  typename Codec::EncodedType out;
  Codec::encode(out, val);
  return out;
}

/**
 * Tries to decode a value of type @p T, using a specific codec and reading from a view.
 *
 * @tparam Codec the codec to use
 * @tparam T the type to decode
 * @param val a view on the encoded value
 * @return the decoded value, or null of the decoding fails
 */
template<typename Codec, typename T>
std::optional<T>
tryDecode(typename Codec::EncodedViewType val) {
  try {
    return decode<Codec, T>(val);
  } catch (const std::exception&) {
    return {};
  }
}

} // namespace rocket::codec

// EOF
