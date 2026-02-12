/**
 * @file FormattedCodec.h
 */

#pragma once

// XXX #include "rocket/InputFailure.h"
#include "rocket/codec/codec.h"
#include "rocket/nio/nio.h"

#include <fmt/std.h>

#include <scn/ranges.h>

namespace rocket::codec {

// #FormattedConsumerConfig ---------------------------------------------------------------------------------

/// Configuration for the #FormattedConsumer.
struct FormattedConsumerConfig {
  /// Whether to indent the output and format a tree.
  bool indent = false;
};

namespace internal {

#if 0
// #FormattedCodec ------------------------------------------------------------------------------------------

// Default implementation ...................................................................................

/**
 * A codec implementation for formatted string representations.
 *
 * This default implementation uses {fmt} to encode and scnlib to decode.
 *
 * @tparam T the type to encode/decode
 */
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

/// @spec{#rocket::codec::FormattedCodec, #std::optional}
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
#endif

// #FormattedConsumerImpl -----------------------------------------------------------------------------------

#define CONFIG__ [[maybe_unused]] const FormattedConsumerConfig& config

template<ValueType ValueType, typename T>
struct FormattedConsumerImpl;

template<>
struct FormattedConsumerImpl<ValueType::Bool, bool> {
  void
  consume(bool val, nio::Sink& out, CONFIG__) {
    out.print("{}", val);
  }
};

#undef CONFIG__

// #FormattedProducerImpl -----------------------------------------------------------------------------------

template<ValueType ValueType, typename T>
struct FormattedProducerImpl;

} // namespace internal

// #FormattedConsumer ---------------------------------------------------------------------------------------

/// The consumer for the #FormmatedCodec.
struct FormattedConsumer {
  /// @type_alias
  template<ValueType ValueType, typename T>
  using Type = internal::FormattedConsumerImpl<ValueType, T>;
};

// #FormattedProducer ---------------------------------------------------------------------------------------

/**
 * The producer for the #FormmatedCodec.
 */
struct FormattedProducer {
  /// @type_alias
  template<ValueType ValueType, typename T>
  using Type = internal::FormattedProducerImpl<ValueType, T>;
};

// #FormattedCodec ------------------------------------------------------------------------------------------

/// The codec for formatted string I/O.
struct FormattedCodec : Codec<FormattedConsumer, FormattedProducer> {
  using Base = Codec<FormattedConsumer, FormattedProducer>; ///< @type_base

  template<typename T>
  auto
  encode(const T& val, nio::Sink& out) const {
    return Base::encode(val, out, FormattedConsumerConfig());
  }

  template<typename T>
  auto
  encode(const T& val, nio::Sink& out, const FormattedConsumerConfig& config) const {
    return Base::encode(val, out, config);
  }
};

} // namespace rocket::codec

// EOF
