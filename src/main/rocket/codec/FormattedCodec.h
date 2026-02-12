/**
 * @file FormattedCodec.h
 */

#pragma once

#include "rocket/Guard.h"
#include "rocket/codec/codec.h"
#include "rocket/nio/nio.h"
#include "rocket/str/escape/escape.h"
#include "rocket/unicode/ConvertTo.h"

#include <fmt/format.h>

#include <scn/scan.h>

namespace rocket::codec {

// #FormattedConsumerConfig ---------------------------------------------------------------------------------

/// Configuration for the #FormattedConsumer.
struct FormattedConsumerConfig {
  /// Whether to indent the output and format a tree.
  bool indent = false;
};

namespace internal {

// Container  management (encoding) -------------------------------------------------------------------------

extern thread_local u64 level;

void beginContainer(nio::Sink& out, const FormattedConsumerConfig& config, char c);

void endContainer(nio::Sink& out, const FormattedConsumerConfig& config, u64 size, char c);

void nextElem(nio::Sink& out, const FormattedConsumerConfig& config, u64 index);

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

template<typename C>
struct FormattedConsumerImpl<ValueType::Char, C> {
  void
  consume(C val, nio::Sink& out, CONFIG__) {
    using namespace rocket::str::escape;

    std::basic_string<C> str = { val };
    std::string utf8(unicode::ConvertTo<char>::apply(str));
    std::string escaped = escapeCString(utf8, { .quote='\'' });
    out.print("{}", escaped);
  }
};

template<typename E>
struct FormattedConsumerImpl<ValueType::Enum, E> {
  void
  consume(E val, nio::Sink& out, CONFIG__) {
    if constexpr (fmt::is_formattable<E>::value) {
      out.print("{}", val);
    } else {
      out.print("{}", std::to_underlying(val));
    }
  }
};

template<typename I>
struct FormattedConsumerImpl<ValueType::Integer, I> {
  void
  consume(I val, nio::Sink& out, CONFIG__) {
    out.print("{}", val);
  }
};

template<typename F>
struct FormattedConsumerImpl<ValueType::Float, F> {
  void
  consume(F val, nio::Sink& out, CONFIG__) {
    out.print("{}", val);
  }
};

template<typename T>
struct FormattedConsumerImpl<ValueType::Pointer, T> {
  void
  consume(T val, nio::Sink& out, CONFIG__) {
    if (val == nullptr) {
      out.write("<null>");
      return;
    }
    out.print("{}", static_cast<const void*>(val));
  }
};

template<typename T>
struct FormattedConsumerImpl<ValueType::String, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    using namespace rocket::str::escape;

    std::string utf8(unicode::ConvertTo<char>::apply(val));
    std::string escaped = escapeCString(utf8, { .quote='"' });
    out.print("{}", escaped);
  }
};

template<typename T>
struct FormattedConsumerImpl<ValueType::Optional, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    if (not val) {
      out.write("<none>");
      return;
    }

    using Elem = typename T::value_type;
    constexpr auto elemValueType = ValueTypes<Elem>::value;
    FormattedConsumerImpl<elemValueType, Elem>().consume(*val, out, config);
  }
};

// For #MemberRef, the tuple consumer must be able to pass additional arguments to the element consumer
template<typename T>
struct FormattedConsumerImpl<ValueType::Tuple, T> {
  template<typename... Args>
  void
  consume(const T& val, nio::Sink& out, CONFIG__, Args&&... args) {
    beginContainer(out, config, '(');
    u64 index = 0;
    std::apply([&](auto&&... arg) {
      (consumeElem(std::forward<decltype(arg)>(arg), out, config, index++, std::forward<Args>(args)...), ...);
    }, val);
    endContainer(out, config, std::tuple_size<T>::value, ')');
  }

private:

  template<typename Elem, typename... Args>
  void
  consumeElem(const Elem& elem, nio::Sink& out, CONFIG__, u64 index, Args&&... args) {
    nextElem(out, config, index);
    constexpr auto elemValueType = ValueTypes<Elem>::value;
    FormattedConsumerImpl<elemValueType, Elem>().consume(elem, out, config, std::forward<Args>(args)...);
  }
};

template<typename T>
struct FormattedConsumerImpl<ValueType::Array, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    using Elem = typename T::value_type;
    constexpr auto elemValueType = ValueTypes<Elem>::value;

    beginContainer(out, config, '[');
    u64 index = 0;
    for (const auto& elem : val) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<elemValueType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), ']');
  }
};

template<typename T>
struct FormattedConsumerImpl<ValueType::Set, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    using Elem = typename T::value_type;
    constexpr auto elemValueType = ValueTypes<Elem>::value;

    beginContainer(out, config, '{');
    u64 index = 0;
    for (const auto& elem : val) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<elemValueType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), '}');
  }
};

template<typename T>
struct FormattedConsumerImpl<ValueType::Map, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    using Key = typename T::key_type;
    constexpr auto keyValueType = ValueTypes<Key>::value;
    using Elem = typename T::mapped_type;
    constexpr auto elemValueType = ValueTypes<Elem>::value;

    beginContainer(out, config, '{');
    u64 index = 0;
    for (const auto& [key, elem] : val) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<keyValueType, Key>().consume(key, out, config);
      out.write(": ");
      FormattedConsumerImpl<elemValueType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), '}');
  }
};

template<typename T>
struct FormattedConsumerImpl<ValueType::Bimap, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    using Key = PurgeType<typename T::left_value_type::first_type>;
    constexpr auto keyValueType = ValueTypes<Key>::value;
    using Elem = PurgeType<typename T::left_value_type::second_type>;
    constexpr auto elemValueType = ValueTypes<Elem>::value;

    beginContainer(out, config, '{');
    u64 index = 0;
    for (const auto& [key, elem] : val.left) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<keyValueType, Key>().consume(key, out, config);
      out.write(": ");
      FormattedConsumerImpl<elemValueType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), '}');
  }
};

template<typename T>
struct FormattedConsumerImpl<ValueType::MemberRefProvider, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    constexpr auto& refs = rocket::reflect::MemberRefProvider<T>::refs;
    using Elem = PurgeType<decltype(refs)>;
    constexpr auto elemValueType = ValueTypes<Elem>::value;
    static_assert(elemValueType == ValueType::Tuple);

    // Here we have to pass an additional argument, the instance, to the tuple consumer. The tuple consumer
    // will pass it on to the member-reference consumer
    FormattedConsumerImpl<elemValueType, Elem>().consume(refs, out, config, val);
  }
};

template<typename T>
struct FormattedConsumerImpl<ValueType::MemberRef, T> {
  template<typename C>
  void
  consume(const T& val, nio::Sink& out, CONFIG__, const C& instance) {
    using Elem = T::ValueType;
    constexpr auto elemValueType = ValueTypes<Elem>::value;

    out.write(val.name());
    out.write('=');
    FormattedConsumerImpl<elemValueType, Elem>().consume(val.get(instance), out, config);
  }
};

template<typename T>
struct FormattedConsumerImpl<ValueType::VarRef, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    using Elem = T::ValueType;
    constexpr auto elemValueType = ValueTypes<Elem>::value;

    out.write(val.name());
    out.write('=');
    FormattedConsumerImpl<elemValueType, Elem>().consume(val.get(), out, config);
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
    ROCKET_GUARD([&] { internal::level = 0; });
    return Base::encode(val, out, FormattedConsumerConfig());
  }

  template<typename T>
  auto
  encode(const T& val, nio::Sink& out, const FormattedConsumerConfig& config) const {
    ROCKET_GUARD([&] { internal::level = 0; });
    return Base::encode(val, out, config);
  }
};

} // namespace rocket::codec

// EOF
