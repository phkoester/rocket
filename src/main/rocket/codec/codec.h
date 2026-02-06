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

// Mixed or middle endian is not supported
static_assert(std::endian::native == std::endian::little || std::endian::native == std::endian::big);

constexpr bool HAS_LITTLE_ENDIAN = std::endian::native == std::endian::little;

// Functions ------------------------------------------------------------------------------------------------

template<typename Codec, typename T>
T
decode(typename Codec::EncodedViewType val) {
  return Codec::template decode<T>(val, 0).first;
}

template<typename Codec, typename T>
Codec::EncodedType
encode(const T& val) {
  typename Codec::EncodedType out;
  Codec::encode(out, val);
  return out;
}

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
