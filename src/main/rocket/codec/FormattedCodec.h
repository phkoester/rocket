/**
 * @file FormattedCodec.h
 */

#pragma once

#include "rocket/enum.h"
#include "rocket/Guard.h"
#include "rocket/InputFailure.h"
#include "rocket/codec/codec.h"
#include "rocket/nio/nio.h"
#include "rocket/str/escape/escape.h"
#include "rocket/unicode/ConvertTo.h"

#include <fmt/std.h>

#include <scn/scan.h>

namespace rocket::codec {

// #FormattedConsumerConfig ---------------------------------------------------------------------------------

/// Configuration for #rocket::codec::FormattedConsumer.
struct FormattedConsumerConfig {
  /// Whether to indent the output and format a tree.
  bool indent = false;
};

// #FormattedProducerConfig ---------------------------------------------------------------------------------

/// Configuration for #rocket::codec::FormattedProducer.
struct FormattedProducerConfig {
  /// Whether to allow C-style comments starting with @c // or @c /*.
  bool cComments = false;
  /// Whether to allow shell-style comments starting with @c #.
  bool shellComments = false;
};

namespace internal {

// Utilities for encoding -----------------------------------------------------------------------------------

// Takes care of indentation
void beginContainer(nio::Sink& out, const FormattedConsumerConfig& config, char c);

// Takes care of indentation
void endContainer(nio::Sink& out, const FormattedConsumerConfig& config, u64 size, char c);

// Takes care of indentation
void nextElem(nio::Sink& out, const FormattedConsumerConfig& config, u64 index);

// Resets the current level of indentation
void resetLevel();

// Utilities for decoding -----------------------------------------------------------------------------------

// Throws if there is no colon, advances the source only on success
void expectColon(nio::StringSource& in);

// Throws if there is no comma, advances the source only on success
void expectComma(nio::StringSource& in);

// Finds a character not preceded by an escaping backslash
[[nodiscard]] u64 findUnescaped(std::string_view str, char c);

// Reads a single expected character, advances the source only on success
[[nodiscard]] bool read(nio::StringSource& in, char c);

// Reads any of a set of expected strings, advances the source only on success
[[nodiscard]] std::optional<std::string_view> read(
  nio::StringSource& in,
  const std::set<std::string_view>& values,
  bool ignoreCase = false);

/// Skips whitespace and comments
void skip(nio::StringSource& in, const FormattedProducerConfig& config);

// #FormattedConsumerImpl -----------------------------------------------------------------------------------

/// @cond undocumented
#define CONFIG__ [[maybe_unused]] const FormattedConsumerConfig& config
/// @endcond

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
    std::basic_string<C> str = { val };
    std::string utf8(unicode::ConvertTo<char>::apply(str));
    std::string escaped = str::escape::escapeCString(utf8, { .quote='\'' });
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
    std::string utf8(unicode::ConvertTo<char>::apply(val));
    std::string escaped = str::escape::escapeCString(utf8, { .quote='"' });
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

    using Elem = T::value_type;
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;
    FormattedConsumerImpl<ElemEncode, Elem>().consume(*val, out, config);
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
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;
    FormattedConsumerImpl<ElemEncode, Elem>().consume(elem, out, config, std::forward<Args>(args)...);
  }
};

template<typename T>
struct FormattedConsumerImpl<ValueType::Array, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    using Elem = T::value_type;
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;

    beginContainer(out, config, '[');
    u64 index = 0;
    for (const auto& elem : val) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<ElemEncode, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), ']');
  }
};

template<typename T>
struct FormattedConsumerImpl<ValueType::Set, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    using Elem = T::value_type;
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;

    beginContainer(out, config, '{');
    u64 index = 0;
    for (const auto& elem : val) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<ElemEncode, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), '}');
  }
};

template<typename T>
struct FormattedConsumerImpl<ValueType::Map, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    using Key = T::key_type;
    constexpr auto KeyEncode = ValueTypes<Key>::Encode;
    using Elem = T::mapped_type;
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;

    beginContainer(out, config, '{');
    u64 index = 0;
    for (const auto& [key, elem] : val) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<KeyEncode, Key>().consume(key, out, config);
      out.write(": ");
      FormattedConsumerImpl<ElemEncode, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), '}');
  }
};

template<typename T>
struct FormattedConsumerImpl<ValueType::Bimap, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    using Key = PurgeType<typename T::left_value_type::first_type>;
    constexpr auto KeyEncode = ValueTypes<Key>::Encode;
    using Elem = PurgeType<typename T::left_value_type::second_type>;
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;

    beginContainer(out, config, '{');
    u64 index = 0;
    for (const auto& [key, elem] : val.left) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<KeyEncode, Key>().consume(key, out, config);
      out.write(": ");
      FormattedConsumerImpl<ElemEncode, Elem>().consume(elem, out, config);
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
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;
    static_assert(ElemEncode == ValueType::Tuple);

    // Here we have to pass an additional argument, the instance, to the tuple consumer. The tuple consumer
    // will pass it on to the member-reference consumer
    FormattedConsumerImpl<ElemEncode, Elem>().consume(refs, out, config, val);
  }
};

template<typename T>
struct FormattedConsumerImpl<ValueType::MemberRef, T> {
  template<typename C>
  void
  consume(const T& val, nio::Sink& out, CONFIG__, const C& instance) {
    using Elem = T::ValueType;
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;

    out.write(val.name());
    out.write('=');
    FormattedConsumerImpl<ElemEncode, Elem>().consume(val.get(instance), out, config);
  }
};

template<typename T>
struct FormattedConsumerImpl<ValueType::VarRef, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    using Elem = T::ValueType;
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;

    out.write(val.name());
    out.write('=');
    FormattedConsumerImpl<ElemEncode, Elem>().consume(val.get(), out, config);
  }
};

#undef CONFIG__

// #FormattedProducerImpl -----------------------------------------------------------------------------------

/// @cond undocumented
#define CONFIG__ [[maybe_unused]] const FormattedProducerConfig& config
/// @endcond

template<ValueType ValueType, typename T>
struct FormattedProducerImpl;

template<>
struct FormattedProducerImpl<ValueType::Bool, bool> {
  void
  produce(bool& val, nio::StringSource& in, CONFIG__) {
    skip(in, config);
    const auto pos = in.tell();

    if (read(in, { "0", "false" }, true)) {
      val = false;
      return;
    }
    if (read(in, { "1", "true" }, true)) {
      val = true;
      return;
    }
    throw InputFailure(pos, "Expected a boolean value");
  }
};

template<typename C>
struct FormattedProducerImpl<ValueType::Char, C> {
  void
  produce(C& val, nio::StringSource& in, CONFIG__) {
    skip(in, config);
    const auto pos = in.tell();

    if (not read(in, '\'')) {
      throw InputFailure(pos, "Expected a character");
    }

    auto available = in.available();
    auto closing = findUnescaped(available, '\'');
    if (closing == NPOS) {
      throw InputFailure(pos, "Unterminated character literal");
    }
    in.seek(closing + 1, nio::SeekMode::cur);

    std::string_view input = available.substr(0, closing);
    std::string unescaped = str::escape::unescapeCString(input);
    std::basic_string<C> str(unicode::ConvertTo<C>::apply(unescaped));
    if (str.size() != 1) {
      throw InputFailure(pos, "Invalid character literal");
    }
    val = str[0];
  }
};

template<typename E>
struct FormattedProducerImpl<ValueType::Enum, E> {
  void
  produce(E& val, nio::StringSource& in, CONFIG__) {
    skip(in, config);
    const auto pos = in.tell();

    if constexpr (rocket::Enum<E>::value) { // @todo Use `scan::is_scannable`
      try {
        auto available = in.available();
        const auto [size, enumVal] = rocket::Enum<E>::toType(available, false);
        in.seek(size, nio::SeekMode::cur);
        val = enumVal;
      } catch (const std::exception&) {
        throw InputFailure(pos, fmt::format("Invalid value for enumeration `{}`", typeid(E)));
      }
    } else {
      using Underlying = decltype(std::to_underlying(val));
      constexpr auto UnderlyingDecode = ValueTypes<Underlying>::Decode;
      Underlying underlying;
      FormattedProducerImpl<UnderlyingDecode, Underlying>().produce(underlying, in, config);
      val = static_cast<E>(underlying);
    }
  }
};

template<typename I>
struct FormattedProducerImpl<ValueType::Integer, I> {
  void
  produce(I& val, nio::StringSource& in, CONFIG__) {
    skip(in, config);
    const auto pos = in.tell();

    auto available = in.available();
    // Setting `base` to 0 detects the base from the input
    auto result = scn::scan_int<I>(available, 0);
    if (result) {
      in.seek(result->begin() - available.begin(), nio::SeekMode::cur);
      val = result->value();
      return;
    }

    throw InputFailure(pos, "Expected an integer value");
  }
};

template<typename F>
struct FormattedProducerImpl<ValueType::Float, F> {
  void
  produce(F& val, nio::StringSource& in, CONFIG__) {
    skip(in, config);
    const auto pos = in.tell();

    auto available = in.available();
    auto result = scn::scan<F>(available, "{}");
    if (result) {
      in.seek(result->begin() - available.begin(), nio::SeekMode::cur);
      val = result->value();
      return;
    }

    throw InputFailure(pos, "Expected a floating-point value");
  }
};

template<typename T>
struct FormattedProducerImpl<ValueType::String, T> {
  void
  produce(T& val, nio::StringSource& in, CONFIG__) {
    skip(in, config);
    const auto pos = in.tell();

    if (not read(in, '"')) {
      throw InputFailure(pos, "Expected a string");
    }

    auto available = in.available();
    auto closing = findUnescaped(available, '"');
    if (closing == NPOS) {
      throw InputFailure(pos, "Unterminated string literal");
    }
    in.seek(closing + 1, nio::SeekMode::cur);

    std::string_view input = available.substr(0, closing);
    std::string unescaped = str::escape::unescapeCString(input);
    using C = T::value_type;
    std::basic_string<C> str(unicode::ConvertTo<C>::apply(unescaped));
    val = std::move(str);
  }
};

template<typename T>
struct FormattedProducerImpl<ValueType::Optional, T> {
  void
  produce(T& val, nio::StringSource& in, CONFIG__) {
    skip(in, config);

    if (read(in, { "<none>" })) {
      val = std::nullopt;
      return;
    }

    using Elem = T::value_type;
    constexpr auto ElemDecode = ValueTypes<Elem>::Decode;
    val = Elem();
    FormattedProducerImpl<ElemDecode, Elem>().produce(*val, in, config);
  }
};

// For #MemberRef, the tuple producer must be able to pass additional arguments to the element producer
template<typename T>
struct FormattedProducerImpl<ValueType::Tuple, T> {
  template<typename... Args>
  void
  produce(T& val, nio::StringSource& in, CONFIG__, Args&&... args) {
    skip(in, config);
    const auto pos = in.tell();

    if (not read(in, '(')) {
      throw InputFailure(pos, "Expected a tuple");
    }

    u64 index = 0;
    std::apply([&](auto&&... arg) {
      (produceElem(std::forward<decltype(arg)>(arg), in, config, index++, std::forward<Args>(args)...), ...);
    }, val);

    skip(in, config);
    if (std::tuple_size<T>::value > 0 && read(in, ',')) { // Allow trailing comma if nonempty
      skip(in, config);
    }
    if (not read(in, ')')) {
      throw InputFailure(in.tell(), { pos, in.tell() }, "Unterminated tuple");
    }
  }

private:

  template<typename Elem, typename... Args>
  void
  produceElem(Elem& elem, nio::StringSource& in, CONFIG__, u64 index, Args&&... args) {
    skip(in, config);
    if (index > 0) {
      expectComma(in);
      skip(in, config);
    }
    constexpr auto ElemDecode = ValueTypes<Elem>::Decode;
    FormattedProducerImpl<ElemDecode, Elem>().produce(elem, in, config, std::forward<Args>(args)...);
  }
};

template<typename T>
struct FormattedProducerImpl<ValueType::Array, T> {
  void
  produce(T& val, nio::StringSource& in, CONFIG__) {
    skip(in, config);
    const auto pos = in.tell();

    if (not read(in, '[')) {
      throw InputFailure(pos, "Expected an array");
    }

    if constexpr (IsArray<T>) {
      // Fixed-size array
      produceArray(val, in, config, pos);
    } else {
      // Dynamic-size vector
      produceVector(val, in, config);
    }
  }

private:

  void
  produceArray(T& val, nio::StringSource& in, CONFIG__, u64 pos)  {
    using Elem = T::value_type;
    constexpr auto ElemDecode = ValueTypes<Elem>::Decode;

    const auto size = val.size();
    for (u64 index = 0; index < size; ++index) {
      skip(in, config);
      if (index > 0) {
        expectComma(in);
        skip(in, config);
      }
      FormattedProducerImpl<ElemDecode, Elem>().produce(val[index], in, config);
    }

    skip(in, config);
    if (size > 0 && read(in, ',')) { // Allow trailing comma if nonempty
      skip(in, config);
    }
    if (not read(in, ']')) {
      throw InputFailure(in.tell(), { pos, in.tell() }, "Unterminated array");
    }
  }

  void
  produceVector(T& val, nio::StringSource& in, CONFIG__)  {
    using Elem = T::value_type;
    constexpr auto ElemDecode = ValueTypes<Elem>::Decode;

    u64 index = 0;
    while (true) {
      skip(in, config);
      if (read(in, ']')) {
        return;
      }
      if (index++ > 0) {
        expectComma(in);
        skip(in, config);
        if (read(in, ']')) { // Allow trailing comma if nonempty
          return;
        }
      }
      val.push_back(Elem());
      FormattedProducerImpl<ElemDecode, Elem>().produce(val.back(), in, config);
    }
  }
};

template<typename T>
struct FormattedProducerImpl<ValueType::Set, T> {
  void
  produce(T& val, nio::StringSource& in, CONFIG__) {
    using Elem = T::value_type;
    constexpr auto ElemDecode = ValueTypes<Elem>::Decode;

    skip(in, config);
    const auto pos = in.tell();

    if (not read(in, '{')) {
      throw InputFailure(pos, "Expected a set");
    }

    u64 index = 0;
    while (true) {
      skip(in, config);
      if (read(in, '}')) {
        return;
      }
      if (index++ > 0) {
        expectComma(in);
        skip(in, config);
        if (read(in, '}')) { // Allow trailing comma if nonempty
          return;
        }
      }
      Elem elem;
      FormattedProducerImpl<ElemDecode, Elem>().produce(elem, in, config);
      val.insert(std::move(elem));
    }
  }
};

template<typename T>
struct FormattedProducerImpl<ValueType::Map, T> {
  void
  produce(T& val, nio::StringSource& in, CONFIG__) {
    using Key = T::key_type;
    constexpr auto KeyDecode = ValueTypes<Key>::Decode;
    using Elem = T::mapped_type;
    constexpr auto ElemDecode = ValueTypes<Elem>::Decode;

    skip(in, config);
    const auto pos = in.tell();

    if (not read(in, '{')) {
      throw InputFailure(pos, "Expected a map");
    }

    u64 index = 0;
    while (true) {
      skip(in, config);
      if (read(in, '}')) {
        return;
      }
      if (index++ > 0) {
        expectComma(in);
        if (read(in, '}')) { // Allow trailing comma if nonempty
          return;
        }
        skip(in, config);
      }

      Key key;
      FormattedProducerImpl<KeyDecode, Key>().produce(key, in, config);
      skip(in, config);

      expectColon(in);
      skip(in, config);

      Elem elem;
      FormattedProducerImpl<ElemDecode, Elem>().produce(elem, in, config);
      val.emplace(std::move(key), std::move(elem));
    }
  }
};

template<typename T>
struct FormattedProducerImpl<ValueType::Bimap, T> {
  void
  produce(T& val, nio::StringSource& in, CONFIG__) {
    using Key = PurgeType<typename T::left_value_type::first_type>;
    constexpr auto KeyDecode = ValueTypes<Key>::Decode;
    using Elem = PurgeType<typename T::left_value_type::second_type>;
    constexpr auto ElemDecode = ValueTypes<Elem>::Decode;

    skip(in, config);
    const auto pos = in.tell();

    if (not read(in, '{')) {
      throw InputFailure(pos, "Expected a map");
    }

    u64 index = 0;
    while (true) {
      skip(in, config);
      if (read(in, '}')) {
        return;
      }
      if (index++ > 0) {
        expectComma(in);
        skip(in, config);
        if (read(in, '}')) { // Allow trailing comma if nonempty
          return;
        }
      }

      Key key;
      FormattedProducerImpl<KeyDecode, Key>().produce(key, in, config);
      skip(in, config);

      expectColon(in);
      skip(in, config);

      Elem elem;
      FormattedProducerImpl<ElemDecode, Elem>().produce(elem, in, config);
      val.left.insert({ std::move(key), std::move(elem) });
    }
  }
};

template<typename T>
struct FormattedProducerImpl<ValueType::MemberRefProvider, T> {
  void
  produce(T& val, nio::StringSource& in, CONFIG__) {
    const auto& refs = rocket::reflect::MemberRefProvider<T>::refs;
    using Elem = PurgeType<decltype(refs)>;
    constexpr auto ElemDecode = ValueTypes<Elem>::Decode;
    static_assert(ElemDecode == ValueType::Tuple);

    // Here we have to pass an additional argument, the instance, to the tuple producer. The tuple producer
    // will pass it on to the member-reference producer
    FormattedProducerImpl<ElemDecode, Elem>().produce(const_cast<Elem&>(refs), in, config, val);
  }
};

template<typename T>
struct FormattedProducerImpl<ValueType::MemberRef, T> {
  template<typename C>
  void
  produce(T& val, nio::StringSource& in, CONFIG__, C& instance) {
    skip(in, config);
    const auto pos = in.tell();

    auto available = in.available();
    auto eq = available.find('=');
    if (eq == NPOS) {
      throw InputFailure(pos, "Expected a member reference");
    }
    in.seek(eq + 1, nio::SeekMode::cur);

    // For `MemberRef`, we demand the name to match
    std::string_view name = available.substr(0, eq);
    name = str::removeTrailing<char>(name, " "); // @todo Trim all trailing whitespace
    if (name != val.name()) {
      throw InputFailure(pos, fmt::format("Expected name `{}`, got `{}`", val.name(), name));
    }
    skip(in, config);

    using Elem = T::ValueType;
    constexpr auto ElemDecode = ValueTypes<Elem>::Decode;
    FormattedProducerImpl<ElemDecode, Elem>().produce(val.get(instance), in, config);
  }
};

template<typename T>
struct FormattedProducerImpl<ValueType::VarRef, T> {
  void
  produce(T& val, nio::StringSource& in, CONFIG__) {
    skip(in, config);
    const auto pos = in.tell();

    auto available = in.available();
    auto eq = available.find('=');
    if (eq == NPOS) {
      throw InputFailure(pos, "Expected a variable reference");
    }
    in.seek(eq + 1, nio::SeekMode::cur);

    // For `VarRef`, we ignore the name altogether and do not demand it to match
    skip(in, config);

    using Elem = T::ValueType;
    constexpr auto ElemDecode = ValueTypes<Elem>::Decode;
    FormattedProducerImpl<ElemDecode, Elem>().produce(val.get(), in, config);
  }
};

#undef CONFIG__

} // namespace internal

// #FormattedConsumer ---------------------------------------------------------------------------------------

/// The consumer for #rocket::codec::FormattedCodec.
struct FormattedConsumer {
  /// @type_alias
  template<ValueType ValueType, typename T>
  using Type = internal::FormattedConsumerImpl<ValueType, T>;
};

// #FormattedProducer ---------------------------------------------------------------------------------------

/// The producer for #rocket::codec::FormattedCodec.
struct FormattedProducer {
  /// @type_alias
  template<ValueType ValueType, typename T>
  using Type = internal::FormattedProducerImpl<ValueType, T>;
};

// #FormattedCodec ------------------------------------------------------------------------------------------

/// The codec for formatted string I/O.
struct FormattedCodec : Codec<FormattedConsumer, FormattedProducer> {
  using Base = Codec<FormattedConsumer, FormattedProducer>; ///< @type_base

  /**
   * Encodes a value.
   *
   * @tparam T the type to encode
   * @param val the value to encode
   * @param out the output sink
   * @return whatever the consumer returns
   */
  template<typename T>
  auto
  encode(const T& val, nio::Sink& out) const {
    ROCKET_GUARD([&] { internal::resetLevel(); });
    return Base::encode(val, out, FormattedConsumerConfig());
  }

  /**
   * Encodes a value.
   *
   * @tparam T the type to encode
   * @param val the value to encode
   * @param out the output sink
   * @param config the configuration
   * @return whatever the consumer returns
   */
  template<typename T>
  auto
  encode(const T& val, nio::Sink& out, const FormattedConsumerConfig& config) const {
    ROCKET_GUARD([&] { internal::resetLevel(); });
    return Base::encode(val, out, config);
  }

  /**
   * Decodes a value from a source.
   *
   * @tparam T the type to decode
   * @param in the input source
   * @return the decoded value
   * @throw #std::exception if the value cannot be decoded
   */
  template<typename T>
  T
  decode(nio::StringSource& in) const {
    return Base::decode<T>(in, FormattedProducerConfig());
  }

  /**
   * Decodes a value from a source.
   *
   * @tparam T the type to decode
   * @param in the input source
   * @param config the configuration
   * @return the decoded value
   * @throw #std::exception if the value cannot be decoded
   */
  template<typename T>
  T
  decode(nio::StringSource& in, const FormattedProducerConfig& config) const {
    return Base::decode<T>(in, config);
  }

  /**
   * Tries to decode a value from a source.
   *
   * @tparam T the type to decode
   * @param in the input source
   * @return the decoded value, or null if the value cannot be decoded
   */
  template<typename T>
  [[nodiscard]] std::optional<T>
  tryDecode(nio::StringSource& in) const {
    return Base::tryDecode<T>(in, FormattedProducerConfig());
  }

  /**
   * Tries to decode a value from a source.
   *
   * @tparam T the type to decode
   * @param in the input source
   * @param config the configuration
   * @return the decoded value, or null if the value cannot be decoded
   */
  template<typename T>
  [[nodiscard]] std::optional<T>
  tryDecode(nio::StringSource& in, const FormattedProducerConfig& config) const {
    return Base::tryDecode<T>(in, config);
  }
};

} // namespace rocket::codec

// EOF
