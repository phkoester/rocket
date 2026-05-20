/**
 * @file FormattedCodec.h
 */

#pragma once

#include "rocket/InputFailure.h"
#include "rocket/std.h"
#include "rocket/codec/codec.h"
#include "rocket/codec/codec-utils.h"
#include "rocket/nio/nio.h"
#include "rocket/str/escape/escape.h"
#include "rocket/unicode/ConvertTo.h"

#include <boost/safe_numerics/safe_integer.hpp>

#include <fmt/std.h>

namespace rocket::codec {

// #FormattedConsumerConfig ---------------------------------------------------------------------------------

/// Configuration for #rocket::codec::FormattedConsumer.
struct FormattedConsumerConfig {
  /// Whether to indent the output and format a tree.
  bool indent = false;
  /// The current level of indentation.
  u64 level = 0;
};

// #FormattedProducerConfig ---------------------------------------------------------------------------------

/// Configuration for #rocket::codec::FormattedProducer.
struct FormattedProducerConfig {
  /// Whether to allow C-style comments starting with <code>//</code> or <code>/*</code>.
  bool cComments = false;
  /// Whether to allow shell-style comments starting with <code>#</code>.
  bool shellComments = false;
};

namespace internal {

// Internal utilitities -------------------------------------------------------------------------------------

inline void
beginContainer(nio::Sink& out, FormattedConsumerConfig& config, char c) {
  rocket::codec::beginContainer(out, config.indent, config.level, c);
}

inline void
endContainer(nio::Sink& out, FormattedConsumerConfig& config, u64 size, char c) {
  rocket::codec::endContainer(out, config.indent, config.level, size, c);
}

inline void
nextElem(nio::Sink& out, FormattedConsumerConfig& config, u64 index) {
  rocket::codec::nextElem(out, config.indent, config.level, index);
}

inline void
skip(nio::Source& in, const FormattedProducerConfig& config) {
  rocket::codec::skip(in, config.cComments, config.shellComments);
}

// #FormattedConsumerImpl -----------------------------------------------------------------------------------

/// @cond undocumented
#define CONFIG__ [[maybe_unused]] FormattedConsumerConfig& config
/// @endcond

template<DataType DataType, typename T>
struct FormattedConsumerImpl;

template<>
struct FormattedConsumerImpl<DataType::Bool, bool> {
  void
  consume(bool val, nio::Sink& out, CONFIG__) const {
    out.print("{}", val);
  }
};

template<typename C>
struct FormattedConsumerImpl<DataType::Char, C> {
  void
  consume(C val, nio::Sink& out, CONFIG__) const {
    const std::basic_string<C> str { val };
    const std::string utf8(unicode::ConvertTo<char>::apply(str));
    const std::string escaped = str::escape::escapeCString(utf8, { .quote='\'' });
    out.print("{}", escaped);
  }
};

template<typename E>
struct FormattedConsumerImpl<DataType::Enum, E> {
  using Underlying = std::underlying_type_t<E>;
  static constexpr auto UnderlyingDataType = DataTypes<Underlying>::Value;

  void
  consume(E val, nio::Sink& out, CONFIG__) const {
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
  consume(I val, nio::Sink& out, CONFIG__) const {
    out.print("{}", val);
  }
};

template<typename F>
struct FormattedConsumerImpl<DataType::Float, F> {
  using Limits = std::numeric_limits<F>;

  void
  consume(F val, nio::Sink& out, CONFIG__) const {
    if (val == -Limits::infinity()) {
      out.write("-∞");
      return;
    }
    if (val == Limits::infinity()) {
      out.write("∞");
      return;
    }
    out.print("{}", val);
  }
};

template<typename P>
struct FormattedConsumerImpl<DataType::Pointer, P> {
  void
  consume(P val, nio::Sink& out, CONFIG__) const {
    if (val == nullptr) {
      out.write("null");
      return;
    }
    out.print("{}", static_cast<const void*>(val));
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::String, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    const std::string utf8(unicode::ConvertTo<char>::apply(val));
    const std::string escaped = str::escape::escapeCString(utf8, { .quote='"' });
    out.print("{}", escaped);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Optional, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    if (not val) {
      out.write("null");
      return;
    }

    FormattedConsumerImpl<ElemDataType, Elem>().consume(*val, out, config);
  }
};

// For #MemberRef, the tuple consumer must be able to pass additional arguments to the element consumer
template<typename T>
struct FormattedConsumerImpl<DataType::Tuple, T> {
  template<typename... Args>
  void
  consume(const T& val, nio::Sink& out, CONFIG__, Args&&... args) const {
    beginContainer(out, config, '(');
    u64 index = 0;
    std::apply([&](auto&&... arg) {
      (consumeElem(std::forward<decltype(arg)>(arg), out, config, index++, std::forward<Args>(args)...), ...);
    }, val);
    endContainer(out, config, std::tuple_size_v<T>, ')');
  }

private:

  template<typename Elem, typename... Args>
  void
  consumeElem(const Elem& elem, nio::Sink& out, CONFIG__, u64 index, Args&&... args) const {
    nextElem(out, config, index);
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
      FormattedConsumerImpl<ElemDataType, Elem>().consume(elem, out, config, std::forward<Args>(args)...);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::List, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    beginContainer(out, config, '[');
    u64 index = 0;
    for (const auto& elem : val) {
      nextElem(out, config, index++);
      FormattedConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, index, ']');
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Set, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
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
  using Key = T::key_type;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = T::mapped_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
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
  using Key = Purge<typename T::left_value_type::first_type>;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = Purge<typename T::left_value_type::second_type>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
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
struct FormattedConsumerImpl<DataType::Interval, T> {
  using A = T::A;
  static constexpr auto ADataType = DataTypes<A>::Value;
  using B = T::B;
  static constexpr auto BDataType = DataTypes<B>::Value;

  using Left = T::LeftType;
  using Right = T::RightType;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    if (val.empty()) {
      out.write("∅");
      return;
    }

    out.write(Left::Symbol);
    const auto optA = option(val.a);
    if (not optA) {
      out.write("-∞");
    } else {
      FormattedConsumerImpl<ADataType, A>().consume(val.a, out, config);
    }
    out.write(',');
    const auto optB = option(val.b);
    if (not optB) {
      out.write("∞");
    } else {
      FormattedConsumerImpl<BDataType, B>().consume(val.b, out, config);
    }
    out.write(Right::Symbol);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Declared, T> {
  static constexpr auto& refs = rocket::reflect::Declared<T>::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    // Here we have to pass an additional argument, the instance, to the tuple consumer. The tuple consumer
    // will pass it on to the member-reference consumer
    FormattedConsumerImpl<ElemDataType, Elem>().consume(refs, out, config, val);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Instance, T> {
  static constexpr auto& refs = T::InnerType::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    // Here we have to pass an additional argument, the instance, to the tuple consumer. The tuple consumer
    // will pass it on to the member-reference consumer
    FormattedConsumerImpl<ElemDataType, Elem>().consume(refs, out, config, val.get());
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::MemberRef, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  template<typename C>
  void
  consume(const T& val, nio::Sink& out, CONFIG__, const C& instance) const {
    out.write(val.name());
    out.write('=');
    FormattedConsumerImpl<ElemDataType, Elem>().consume(val.get(instance), out, config);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::VarRef, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.write(val.name());
    out.write('=');
    FormattedConsumerImpl<ElemDataType, Elem>().consume(val.get(), out, config);
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::CodePoint, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    if (val.valid()) {
      out.print("U+{:0>4X}", static_cast<u32>(val));
    } else {
      out.write("<invalid>");
    }
  }
};

template<typename T>
struct FormattedConsumerImpl<DataType::Character, T> {
  using Elem = T::View;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    const auto elem = static_cast<Elem>(val);
    FormattedConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
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
  produce(bool& val, nio::Source& in, CONFIG__) const {
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
  produce(C& val, nio::Source& in, CONFIG__) const {
    using boost::safe_numerics::safe;

    skip(in, config);
    const auto pos = in.tell();

    if (not read(in, '\'')) {
      throw InputFailure(pos, "Expected a character");
    }

    auto input = readUntilUnescaped(in, '\'');
    if (not input) {
      throw InputFailure(pos, "Unterminated character literal");
    }

    const std::string unescaped = str::escape::unescapeCString(*input);
    const std::basic_string<C> str(unicode::ConvertTo<C>::apply(unescaped));
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
  produce(E& val, nio::Source& in, CONFIG__) const {
    skip(in, config);
    const auto pos = in.tell();

    if constexpr (scn::detail::is_scannable<E, char>::value) {
      const auto result = scan<E>(in);
      if (result) {
        val = *result;
        return;
      }
      throw InputFailure(pos, fmt::format("Invalid value for enumeration `{}`", typeid(E)));
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
  produce(I& val, nio::Source& in, CONFIG__) const {
    skip(in, config);
    const auto pos = in.tell();

    const auto result = scanInteger<I>(in);
    if (result) {
      val = *result;
      return;
    }
    throw InputFailure(pos, "Expected an integer value");
  }
};

template<typename F>
struct FormattedProducerImpl<DataType::Float, F> {
  using Limits = std::numeric_limits<F>;

  void
  produce(F& val, nio::Source& in, CONFIG__) const {
    skip(in, config);
    const auto pos = in.tell();

    if (read(in, { "-∞" })) {
      val = -Limits::infinity();
      return;
    }
    if (read(in, { "∞" })) {
      val = Limits::infinity();
      return;
    }

    const auto result = scan<F>(in);
    if (result) {
      val = *result;
      return;
    }
    throw InputFailure(pos, "Expected a floating-point value");
  }
};

template<typename P>
struct FormattedProducerImpl<DataType::Pointer, P> {
  void
  produce(P& val, nio::Source& in, CONFIG__) const {
    skip(in, config);
    const auto pos = in.tell();

    if (read(in, { "null" })) {
      val = nullptr;
      return;
    }

    const auto result = scan<P>(in);
    if (result) {
      val = *result;
      return;
    }
    throw InputFailure(pos, "Expected a pointer value");
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::String, T> {
  void
  produce(T& val, nio::Source& in, CONFIG__) const {
    using boost::safe_numerics::safe;

    skip(in, config);
    const auto pos = in.tell();

    if (not read(in, '"')) {
      throw InputFailure(pos, "Expected a string");
    }

    auto input = readUntilUnescaped(in, '"');
    if (not input) {
      throw InputFailure(pos, "Unterminated string literal");
    }

    const std::string unescaped = str::escape::unescapeCString(*input);
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
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in, CONFIG__) const {
    skip(in, config);

    if (read(in, { "null" })) {
      val = std::nullopt;
      return;
    }

    val = Elem();
    FormattedProducerImpl<ElemDataType, Elem>().produce(*val, in, config);
  }
};

// For #MemberRef, the tuple producer must be able to pass additional arguments to the element producer
template<typename T>
struct FormattedProducerImpl<DataType::Tuple, T> {
  template<typename... Args>
  void
  produce(T& val, nio::Source& in, CONFIG__, Args&&... args) const {
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
    if (std::tuple_size_v<T> > 0 && read(in, ',')) { // Allow trailing comma if nonempty
      skip(in, config);
    }
    if (not read(in, ')')) {
      throw InputFailure(in.tell(), { pos, in.tell() }, "Unterminated tuple");
    }
  }

private:

  template<typename Elem, typename... Args>
  void
  produceElem(Elem& elem, nio::Source& in, CONFIG__, u64 index, Args&&... args) const {
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
struct FormattedProducerImpl<DataType::List, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in, CONFIG__) const {
    static_assert(not IsView<T>, "Cannot decode list view");
    static_assert(not IsForwardList<T>, "Cannot decode forward list");

    skip(in, config);
    const auto pos = in.tell();

    if (not read(in, '[')) {
      throw InputFailure(pos, "Expected a list");
    }

    if constexpr (IsArray<T>) {
      // Fixed-size array
      produceArray(val, in, config, pos);
    } else {
      // Container with `push_back`
      produceContainerWithPushBack(val, in, config);
    }
  }

private:

  void
  produceArray(T& val, nio::Source& in, CONFIG__, u64 pos) const {
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
      throw InputFailure(in.tell(), { pos, in.tell() }, fmt::format("Unterminated array of size {}", size));
    }
  }

  void
  produceContainerWithPushBack(T& val, nio::Source& in, CONFIG__) const {
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
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in, CONFIG__) const {
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
  using Key = T::key_type;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = T::mapped_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in, CONFIG__) const {
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
  using Key = Purge<typename T::left_value_type::first_type>;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = Purge<typename T::left_value_type::second_type>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in, CONFIG__) const {
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
struct FormattedProducerImpl<DataType::Interval, T> {
  using A = T::A;
  static constexpr auto ADataType = DataTypes<A>::Value;
  using B = T::B;
  static constexpr auto BDataType = DataTypes<B>::Value;

  using Left = T::LeftType;
  using Right = T::RightType;

  void
  produce(T& val, nio::Source& in, CONFIG__) const {
    skip(in, config);
    const auto pos = in.tell();

    if (read(in, { "∅" })) {
      val = T();
      return;
    }

    if (not read(in, Left::Symbol)) {
      throw InputFailure(pos, "Expected an interval");
    }
    skip(in, config);

    A a = A();
    if constexpr (IsOptional<A>) {
      if (not read(in, { "-∞" })) {
        FormattedProducerImpl<ADataType, A>().produce(a, in, config);
      }
    } else {
      FormattedProducerImpl<ADataType, A>().produce(a, in, config);
    }
    val.a = a;

    skip(in, config);
    expectComma(in);
    skip(in, config);

    B b = B();
    if constexpr (IsOptional<B>) {
      if (not read(in, { "∞" })) {
        FormattedProducerImpl<BDataType, B>().produce(b, in, config);
      }
    } else {
      FormattedProducerImpl<BDataType, B>().produce(b, in, config);
    }
    val.b = b;

    skip(in, config);
    if (not read(in, Right::Symbol)) {
      throw InputFailure(in.tell(), { pos, in.tell() }, "Unterminated interval");
    }
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Declared, T> {
  static constexpr auto& refs = rocket::reflect::Declared<T>::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  produce(T& val, nio::Source& in, CONFIG__) const {
    // Here we have to pass an additional argument, the instance, to the tuple producer. The tuple producer
    // will pass it on to the member-reference producer
    FormattedProducerImpl<ElemDataType, Elem>().produce(const_cast<Elem&>(refs), in, config, val);
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Instance, T> {
  static constexpr auto& refs = T::InnerType::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  produce(T& val, nio::Source& in, CONFIG__) const {
    // Here we have to pass an additional argument, the instance, to the tuple producer. The tuple producer
    // will pass it on to the member-reference producer
    FormattedProducerImpl<ElemDataType, Elem>().produce(const_cast<Elem&>(refs), in, config, val.get());
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::MemberRef, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  template<typename C>
  void
  produce(T& val, nio::Source& in, CONFIG__, C& instance) const {
    using boost::safe_numerics::safe;

    skip(in, config);
    const auto pos = in.tell();

    auto name = readUntil(in, '=');
    if (not name) {
      throw InputFailure(pos, "Expected a member reference");
    }
    name = str::removeTrailing<char>(*name, " "); // @todo Trim all trailing whitespace

    // For `MemberRef`, we demand the name to match
    if (*name != val.name()) {
      throw InputFailure(pos, fmt::format("Expected name `{}`, got `{}`", val.name(), *name));
    }
    skip(in, config);

    FormattedProducerImpl<ElemDataType, Elem>().produce(val.get(instance), in, config);
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::VarRef, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in, CONFIG__) const {
    using boost::safe_numerics::safe;

    skip(in, config);
    const auto pos = in.tell();

    auto name = readUntil(in, '=');
    if (not name) {
      throw InputFailure(pos, "Expected a variable reference");
    }

    // For `VarRef`, we ignore the name altogether and do not demand it to match
    skip(in, config);

    FormattedProducerImpl<ElemDataType, Elem>().produce(val.get(), in, config);
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::CodePoint, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in, CONFIG__) const {
    skip(in, config);
    const auto pos = in.tell();

    if (read(in, '\'')) {
      in.seek(-1, nio::SeekMode::cur);
      Elem elem = Elem();
      FormattedProducerImpl<ElemDataType, Elem>().produce(elem, in, config);
      val = T(elem);
      return;
    }

    auto result = scanCodePoint<u32>(in);
    if (result) {
      val = static_cast<Elem>(*result);
      return;
    }
    throw InputFailure(pos, "Expected a code point");
  }
};

template<typename T>
struct FormattedProducerImpl<DataType::Character, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in, CONFIG__) const {
    Elem elem;
    FormattedProducerImpl<ElemDataType, Elem>().produce(elem, in, config);
    val = T(std::move(elem));
    return;
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
 * A codec for formatted string I/O.
 *
 * The encoder can serialize an arbirary C++ data structure to a formatted string. The result is a format
 * similar to JSON. Tuples are enclosed in parentheses. Lists are enclosed in square brackets. Sets and maps
 * are enclosed in curly braces. If a configuration is provided, the output may be indented and formatted as
 * a tree.
 *
 * The decoder can scan such a formatted string and construct an arbitrary C++ data structure from it. While
 * scanning, any irrelevant whitespace, including line breaks, is ignored. If a configuration is provided,
 * the decoder can skip single-line C-style comments starting with <code>//</code>, multi-line C-style
 * comments starting with <code>/</code><code>*</code>, and single-line shell-style comments starting with
 * <code>#</code>.
 *
 * Decoding to list views and to forward lists is not supported. String views and character views, however,
 * are allowed. This is made possible by storing intermediate strings in the source. Hence, decoded string
 * views and character views are valid, and valid only, for the lifetime of the source.
 *
 * There are various optimizations for decoding from contiguous sources.
 *
 * @see #rocket::codec::FormattedConsumerConfig
 * @see #rocket::codec::FormattedProducerConfig
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
    FormattedConsumerConfig localConfig = config;
    localConfig.level = 0;
    return Base::encode(val, out, localConfig);
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
  decode(nio::Source& in) const {
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
  decode(nio::Source& in, const FormattedProducerConfig& config) const {
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
  tryDecode(nio::Source& in) const {
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
  tryDecode(nio::Source& in, const FormattedProducerConfig& config) const {
    return Base::tryDecode<T>(in, config);
  }
};

} // namespace rocket::codec

// EOF
