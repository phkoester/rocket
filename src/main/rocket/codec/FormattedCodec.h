/**
 * @file FormattedCodec.h
 */

#pragma once

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

template<DataType DataType, typename T>
struct FormattedConsumerImpl;

template<>
struct FormattedConsumerImpl<DataType::Bool, bool> {
  void
  consume(bool val, nio::Sink& out, CONFIG__) {
    out.print("{}", val);
  }
};

template<typename C>
struct FormattedConsumerImpl<DataType::Char, C> {
  void
  consume(C val, nio::Sink& out, CONFIG__) {
    std::basic_string<C> str = { val };
    std::string utf8(unicode::ConvertTo<char>::apply(str));
    std::string escaped = str::escape::escapeCString(utf8, { .quote='\'' });
    out.print("{}", escaped);
  }
};

template<typename E>
struct FormattedConsumerImpl<DataType::Enum, E> {
  using Underlying = std::underlying_type_t<E>;
  static constexpr auto UnderlyingDataType = DataTypes<Underlying>::Value;

  void
  consume(E val, nio::Sink& out, CONFIG__) {
    if constexpr (fmt::is_formattable<E>::value) {
      out.print("{}", val);
    } else {
      const auto underlying = std::to_underlying(val);
      FormattedConsumerImpl<UnderlyingDataType, Underlying>().consume(underlying, out, config);
    }
  }
};

template<typename I>
struct FormattedConsumerImpl<DataType::Integer, I> {
  void
  consume(I val, nio::Sink& out, CONFIG__) {
    out.print("{}", val);
  }
};

template<typename F>
struct FormattedConsumerImpl<DataType::Float, F> {
  void
  consume(F val, nio::Sink& out, CONFIG__) {
    out.print("{}", val);
  }
};

template<typename P>
struct FormattedConsumerImpl<DataType::Pointer, P> {
  void
  consume(P val, nio::Sink& out, CONFIG__) {
    if (val == nullptr) {
      out.write("<null>");
      return;
    }
    out.print("{}", static_cast<const void*>(val));
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::String, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    std::string utf8(unicode::ConvertTo<char>::apply(val));
    std::string escaped = str::escape::escapeCString(utf8, { .quote='"' });
    out.print("{}", escaped);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Optional, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    if (not val) {
      out.write("<none>");
      return;
    }

    using Elem = T::value_type;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    FormattedConsumerImpl<ElemDataType, Elem>().consume(*val, out, config);
  }
};

// For #MemberRef, the tuple consumer must be able to pass additional arguments to the element consumer
template<typename T>
struct FormattedConsumerImpl<DataType::Tuple, T> {
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
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
      FormattedConsumerImpl<ElemDataType, Elem>().consume(elem, out, config, std::forward<Args>(args)...);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Array, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    using Elem = T::value_type;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;

    beginContainer(out, config, '[');
    u64 index = 0;
    for (const auto& elem : val) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), ']');
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Set, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    using Elem = T::value_type;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;

    beginContainer(out, config, '{');
    u64 index = 0;
    for (const auto& elem : val) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), '}');
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Map, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    using Key = T::key_type;
    constexpr auto KeyDataType = DataTypes<Key>::Value;
    using Elem = T::mapped_type;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;

    beginContainer(out, config, '{');
    u64 index = 0;
    for (const auto& [key, elem] : val) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<KeyDataType, Key>().consume(key, out, config);
      out.write(": ");
      FormattedConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), '}');
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Bimap, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    using Key = Purge<typename T::left_value_type::first_type>;
    constexpr auto KeyDataType = DataTypes<Key>::Value;
    using Elem = Purge<typename T::left_value_type::second_type>;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;

    beginContainer(out, config, '{');
    u64 index = 0;
    for (const auto& [key, elem] : val.left) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<KeyDataType, Key>().consume(key, out, config);
      out.write(": ");
      FormattedConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), '}');
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Declared, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    constexpr auto& refs = rocket::reflect::Declared<T>::refs;
    using Elem = Purge<decltype(refs)>;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    static_assert(ElemDataType == DataType::Tuple);

    // Here we have to pass an additional argument, the instance, to the tuple consumer. The tuple consumer
    // will pass it on to the member-reference consumer
    FormattedConsumerImpl<ElemDataType, Elem>().consume(refs, out, config, val);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Instance, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    constexpr auto& refs = T::InnerType::refs;
    using Elem = Purge<decltype(refs)>;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    static_assert(ElemDataType == DataType::Tuple);

    // Here we have to pass an additional argument, the instance, to the tuple consumer. The tuple consumer
    // will pass it on to the member-reference consumer
    FormattedConsumerImpl<ElemDataType, Elem>().consume(refs, out, config, val.get());
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::MemberRef, T> {
  template<typename C>
  void
  consume(const T& val, nio::Sink& out, CONFIG__, const C& instance) {
    using Elem = T::ValueType;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;

    out.write(val.name());
    out.write('=');
    FormattedConsumerImpl<ElemDataType, Elem>().consume(val.get(instance), out, config);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::VarRef, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) {
    using Elem = T::ValueType;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;

    out.write(val.name());
    out.write('=');
    FormattedConsumerImpl<ElemDataType, Elem>().consume(val.get(), out, config);
  }
};

#undef CONFIG__

// #FormattedProducerImpl -----------------------------------------------------------------------------------

/// @cond undocumented
#define CONFIG__ [[maybe_unused]] const FormattedProducerConfig& config
/// @endcond

template<DataType DataType, typename T>
struct FormattedProducerImpl;

template<>
struct FormattedProducerImpl<DataType::Bool, bool> {
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
struct FormattedProducerImpl<DataType::Char, C> {
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
struct FormattedProducerImpl<DataType::Enum, E> {
  using Underlying = std::underlying_type_t<E>;
  static constexpr auto UnderlyingDataType = DataTypes<Underlying>::Value;

  void
  produce(E& val, nio::StringSource& in, CONFIG__) {
    skip(in, config);
    const auto pos = in.tell();

    if constexpr (scn::detail::is_scannable<E, char>::value) {
      const auto available = in.available();
      auto result = scn::scan<E>(available, "{}");
      if (result) {
        in.seek(result->begin() - available.begin(), nio::SeekMode::cur);
        val = result->value();
        return;
      } else {
        throw InputFailure(pos, fmt::format("Invalid value for enumeration `{}`", typeid(E)));
      }
    } else {
      Underlying underlying;
      FormattedProducerImpl<UnderlyingDataType, Underlying>().produce(underlying, in, config);
      val = static_cast<E>(underlying);
    }
  }
};

template<typename I>
struct FormattedProducerImpl<DataType::Integer, I> {
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
struct FormattedProducerImpl<DataType::Float, F> {
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

template<typename P>
struct FormattedProducerImpl<DataType::Pointer, P> {
  void
  produce(P& val, nio::StringSource& in, CONFIG__) {
    skip(in, config);
    const auto pos = in.tell();

    if (read(in, { "<null>" })) {
      val = nullptr;
      return;
    }

    auto available = in.available();
    auto result = scn::scan<P>(available, "{}");
    if (result) {
      in.seek(result->begin() - available.begin(), nio::SeekMode::cur);
      val = result->value();
      return;
    }

    throw InputFailure(pos, "Expected a pointer value");
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::String, T> {
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
    if constexpr (std::is_same_v<T, std::basic_string_view<C>>) {
      // Because both unescaping and Unicode-converting produce intermediate strings local to this function,
      // decoding to #std::basic_string_view is a bit more involved. To make this possible, we store the
      // intermediate string in the source so the decoded string view is valid for the lifetime of the source
      const auto& ref = in.store(std::move(str));
      val = ref;
    } else {
      // #std::basic_string
      val = std::move(str);
    }
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Optional, T> {
  void
  produce(T& val, nio::StringSource& in, CONFIG__) {
    skip(in, config);

    if (read(in, { "<none>" })) {
      val = std::nullopt;
      return;
    }

    using Elem = T::value_type;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    val = Elem();
    FormattedProducerImpl<ElemDataType, Elem>().produce(*val, in, config);
  }
};

// For #MemberRef, the tuple producer must be able to pass additional arguments to the element producer
template<typename T>
struct FormattedProducerImpl<DataType::Tuple, T> {
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
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    FormattedProducerImpl<ElemDataType, Elem>().produce(elem, in, config, std::forward<Args>(args)...);
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Array, T> {
  void
  produce(T& val, nio::StringSource& in, CONFIG__) {
    static_assert(not IsView<T>, "Cannot decode array view");

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
    constexpr auto ElemDataType = DataTypes<Elem>::Value;

    const auto size = val.size();
    for (u64 index = 0; index < size; ++index) {
      skip(in, config);
      if (index > 0) {
        expectComma(in);
        skip(in, config);
      }
      FormattedProducerImpl<ElemDataType, Elem>().produce(val[index], in, config);
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
    constexpr auto ElemDataType = DataTypes<Elem>::Value;

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
      FormattedProducerImpl<ElemDataType, Elem>().produce(val.back(), in, config);
    }
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Set, T> {
  void
  produce(T& val, nio::StringSource& in, CONFIG__) {
    using Elem = T::value_type;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;

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
      FormattedProducerImpl<ElemDataType, Elem>().produce(elem, in, config);
      val.insert(std::move(elem));
    }
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Map, T> {
  void
  produce(T& val, nio::StringSource& in, CONFIG__) {
    using Key = T::key_type;
    constexpr auto KeyDataType = DataTypes<Key>::Value;
    using Elem = T::mapped_type;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;

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
      FormattedProducerImpl<KeyDataType, Key>().produce(key, in, config);
      skip(in, config);

      expectColon(in);
      skip(in, config);

      Elem elem;
      FormattedProducerImpl<ElemDataType, Elem>().produce(elem, in, config);
      val.emplace(std::move(key), std::move(elem));
    }
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Bimap, T> {
  void
  produce(T& val, nio::StringSource& in, CONFIG__) {
    using Key = Purge<typename T::left_value_type::first_type>;
    constexpr auto KeyDataType = DataTypes<Key>::Value;
    using Elem = Purge<typename T::left_value_type::second_type>;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;

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
      FormattedProducerImpl<KeyDataType, Key>().produce(key, in, config);
      skip(in, config);

      expectColon(in);
      skip(in, config);

      Elem elem;
      FormattedProducerImpl<ElemDataType, Elem>().produce(elem, in, config);
      val.left.insert({ std::move(key), std::move(elem) });
    }
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Declared, T> {
  void
  produce(T& val, nio::StringSource& in, CONFIG__) {
    const auto& refs = rocket::reflect::Declared<T>::refs;
    using Elem = Purge<decltype(refs)>;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    static_assert(ElemDataType == DataType::Tuple);

    // Here we have to pass an additional argument, the instance, to the tuple producer. The tuple producer
    // will pass it on to the member-reference producer
    FormattedProducerImpl<ElemDataType, Elem>().produce(const_cast<Elem&>(refs), in, config, val);
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Instance, T> {
  void
  produce(T& val, nio::StringSource& in, CONFIG__) {
    const auto& refs = T::InnerType::refs;
    using Elem = Purge<decltype(refs)>;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    static_assert(ElemDataType == DataType::Tuple);

    // Here we have to pass an additional argument, the instance, to the tuple producer. The tuple producer
    // will pass it on to the member-reference producer
    FormattedProducerImpl<ElemDataType, Elem>().produce(const_cast<Elem&>(refs), in, config, *val.instance);
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::MemberRef, T> {
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
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    FormattedProducerImpl<ElemDataType, Elem>().produce(val.get(instance), in, config);
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::VarRef, T> {
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
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    FormattedProducerImpl<ElemDataType, Elem>().produce(val.get(), in, config);
  }
};

#undef CONFIG__

} // namespace internal

// #FormattedConsumer ---------------------------------------------------------------------------------------

/// The consumer for #rocket::codec::FormattedCodec.
struct FormattedConsumer {
  /// @type_alias
  template<DataType DataType, typename T>
  using Type = internal::FormattedConsumerImpl<DataType, T>;
};

// #FormattedProducer ---------------------------------------------------------------------------------------

/// The producer for #rocket::codec::FormattedCodec.
struct FormattedProducer {
  /// @type_alias
  template<DataType DataType, typename T>
  using Type = internal::FormattedProducerImpl<DataType, T>;
};

// #FormattedCodec ------------------------------------------------------------------------------------------

/**
 * The codec for formatted string I/O.
 *
 * Decoding to array views is not supported. String views, however, are allowed. This is made possible by
 * storing intermediate strings in the source. Hence, decoded string views are valid for the lifetime of the
 * source.
 */
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
