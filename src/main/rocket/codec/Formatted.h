/**
 * @file Formatted.h
 *
 * Formatted encoding/decoding.
 *
 * This is based on {fmt} and scnlib.
 */

#pragma once

#include "rocket/assert.h"
#include "rocket/InputFailure.h"
#include "rocket/rocket.h"
#include "rocket/format/std.h"
#include "rocket/scan/scan.h"

namespace rocket::codec {

// #FormattedCodec ------------------------------------------------------------------------------------------

// Default implementation ...................................................................................

template<typename T>
struct FormattedCodec {
  static std::pair<T, u64>
  decode(std::string_view in, u64 offset) {
    const std::string_view input = in.substr(offset);
    const auto result = scn::scan<T>(input, "{}");
    if (not result) {
      throw InputFailure(offset, fmt::format("Cannot scan as `{}`", typeid(T)));
    }
    const u64 len = result->begin() - input.begin();
    return { result->value(), len };
  }

  static void
  encode(std::string& out, const T& val) {
    out.append(fmt::format("{}", val));
  }
};

// #std::optional ...........................................................................................

template<typename T>
struct FormattedCodec<std::optional<T>> {
  using Type = std::optional<T>;

  static constexpr std::string_view NONE = "<none>";

  static std::pair<Type, u64>
  decode(std::string_view in, u64 offset) {
    const std::string_view input = in.substr(offset);
    if (input.starts_with(NONE)) {
      return { {}, NONE.size() };
    }
    return FormattedCodec<T>::decode(in, offset);
  }

  static void
  encode(std::string& out, const Type& val) {
    if (not val) {
      out.append(NONE);
    } else {
      FormattedCodec<T>::encode(out, *val);
    }
  }
};

// #Formatted -----------------------------------------------------------------------------------------------

struct Formatted {
  using EncodedType = std::string;
  using EncodedViewType = std::string_view;

  template<typename T>
  static std::pair<T, u64>
  decode(EncodedViewType in, u64 offset) {
    return FormattedCodec<T>::decode(in, offset);
  }

  template<typename T>
  static void
  encode(EncodedType& out, const T& val) {
    FormattedCodec<T>::encode(out, val);
  }
};

} // namespace rocket::codec

// EOF
