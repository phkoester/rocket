/**
 * @file codec.h
 *
 * Rocket encoding/decoding.
 *
 * The byte order in encoded form is always little endian.
 */

#pragma once

#include "rocket/assert.h"
#include "rocket/rocket.h"
#include "rocket/format/format.h"
#include "rocket/scan/scan.h"
#include "rocket/str/message/message.h"

#include <bit>
#include <string>
#include <string_view>
#include <typeinfo>

namespace rocket::codec {

// Mixed or middle endian is not supported
static_assert(std::endian::native == std::endian::little || std::endian::native == std::endian::big);

// #FormattedCodec ------------------------------------------------------------------------------------------

template<typename T>
struct FormattedCodec {
  static T
  decode(std::string_view str) {
    auto result = scn::scan<T>(str, "{}");
    if (not result) {
      ROCKET_FAIL("{}", str::message::cannotScanAs(str, typeid(T)));
    }
    return result->value();
  }

  static std::string
  encode(const T& val) {
    return fmt::format("{}", val);
  }
};

// #Formatted -----------------------------------------------------------------------------------------------

struct Formatted {
  using EncodedType = std::string;
  using EncodedViewType = std::string_view;

  template<typename T>
  static T
  decode(EncodedViewType str) {
    return FormattedCodec<T>::decode(str);
  }

  template<typename T>
  static EncodedType
  encode(const T& val) {
    return FormattedCodec<T>::encode(val);
  }

};

// Functions ------------------------------------------------------------------------------------------------

template<typename Codec, typename T>
T
decode(const typename Codec::EncodedViewType val) {
  return Codec::template decode<T>(val);
}

template<typename Codec, typename T>
Codec::EncodedType
encode(const T& val) {
  return Codec::encode(val);
}

consteval bool
littleEndian() {
  return std::endian::native == std::endian::little;
}

} // namespace rocket::codec

// EOF
