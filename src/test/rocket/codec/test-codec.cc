/*
 * test-codec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/codec.h"
#include "rocket/nio/nio.h"
#include "rocket/unicode/ConvertTo.h"

using namespace rocket;
using namespace rocket::codec;

// Functions ------------------------------------------------------------------------------------------------

// Tests if #str is available in #in
bool
read(nio::Source& in, std::string_view str) {
  auto pos = in.tell();
  string buf(str.size(), ' ');
  const span<u8> span(reinterpret_cast<u8*>(buf.data()), buf.size());
  const auto n = in.read(span);
  bool ret = n == span.size() && buf == str;
  if (not ret) {
    in.seek(pos);
  }
  return ret;
}

// Understands "0" to "9"
i32
readI32(nio::Source& in) {
  for (i32 i = 0; i < 10; ++i) {
    string str = fmt::format("{}", i);
    if (read(in, str)) {
      return i;
    }
  }
  throw InputFailure(in.tell(), "expected integer");
}

// #TracingConsumerImpl -------------------------------------------------------------------------------------

template<ValueType, typename T>
struct TracingConsumerImpl;

template<>
struct TracingConsumerImpl<ValueType::boolean, bool> {
  i64 consume(bool val, nio::StringSink& out) {
    return out.println("consuming boolean: {}", val);
  }
};

template<typename C>
struct TracingConsumerImpl<ValueType::character, C> {
  i64 consume(C val, nio::StringSink& out) {
    std::basic_string<C> str = { val };
    return out.println("consuming character: '{}'", unicode::ConvertTo<char>::apply(str));
  }
};

template<typename I>
struct TracingConsumerImpl<ValueType::integer, I> {
  u64 consume(I val, nio::StringSink& out) {
    return out.println("consuming integer: {}", val);
  }
};

template<typename T>
struct TracingConsumerImpl<ValueType::string, T> {
  u64 consume(const T& val, nio::StringSink& out) {
    return out.println("consuming string: {:?}", unicode::ConvertTo<char>::apply(val));
  }
};

template<typename T>
struct TracingConsumerImpl<ValueType::optional, T> {
  u64 consume(const T& val, nio::StringSink& out) {
    auto ret = out.println("consuming optional: {}", val);
    if (val) {
      using Elem = typename T::value_type;
      constexpr auto elemValueType = ValueTypes<Elem>::value;
      ret += TracingConsumerImpl<elemValueType, Elem>().consume(*val, out);
    }
    return ret;
  }
};

template<typename T>
struct TracingConsumerImpl<ValueType::tuple, T> {
  u64 consume(const T& val, nio::StringSink& out) {
    const auto size = std::tuple_size<T>::value;
    auto ret = out.println("consuming tuple: {}", size);
    std::apply([&](auto&&... arg) {
      (consumeElem(ret, std::forward<decltype(arg)>(arg), out), ...);
    }, val);
    return ret;
  }

private:

  template<typename Elem>
  u64 consumeElem(u64& result, const Elem& elem, nio::StringSink& out) {
    result += out.println("consuming tuple elem: {}", elem);
    constexpr auto elemValueType = ValueTypes<Elem>::value;
    result += TracingConsumerImpl<elemValueType, Elem>().consume(elem, out);
    return result;
  }
};

template<typename T>
struct TracingConsumerImpl<ValueType::array, T> {
  u64 consume(const T& val, nio::StringSink& out) {
    const auto size = val.size();
    auto ret = out.println("consuming array: {}", size);
    for (auto it = val.begin(), end = val.end(); it != end; ++it) {
      using Elem = typename T::value_type;
      constexpr auto elemValueType = ValueTypes<Elem>::value;
      ret += TracingConsumerImpl<elemValueType, Elem>().consume(*it, out);
    }
    return ret;
  }
};

// #TracingConsumer -----------------------------------------------------------------------------------------

struct TracingConsumer {
  template<ValueType ValueType, typename T>
  using Type = TracingConsumerImpl<ValueType, T>;
};

// #TracingProducerImpl -------------------------------------------------------------------------------------

template<ValueType, typename T>
struct TracingProducerImpl;

template<>
struct TracingProducerImpl<ValueType::boolean, bool> {
  bool produce(nio::Source& in, nio::Sink& out) {
    out.println("producing boolean");
    if (read(in, "false")) {
      return false;
    }
    if (read(in, "true")) {
      return true;
    }
    throw InputFailure(in.tell(), "expected boolean");
  }
};

template<typename I>
struct TracingProducerImpl<ValueType::integer, I> {
  I produce(nio::Source& in, nio::Sink& out) {
    out.println("producing integer");
    i32 val = readI32(in);
    return static_cast<I>(val);
  }
};

template<typename T>
struct TracingProducerImpl<ValueType::string, T> {
  using C = typename T::value_type;
  static_assert(std::is_same_v<C, char>, "Cannot decode UTF-32 string<");

  // Always produce a string here, even if #T is a string view
  basic_string<C>
  produce(nio::Source& in, nio::Sink& out) {
    out.println("producing string");
    i32 size = readI32(in);
    basic_string<C> ret(size, ' ');
    const span<u8> span(reinterpret_cast<u8*>(ret.data()), ret.size());
    const auto n = in.read(span);
    if (n != span.size()) {
      throw InputFailure(in.tell(), "expected string");
    }
    return ret;
  }
};

template<typename T>
struct TracingProducerImpl<ValueType::optional, T> {
  T produce(nio::Source& in, nio::Sink& out) {
    out.println("producing optional");
    if (read(in, "none")) {
      return nullopt;
    }
    using Elem = typename T::value_type;
    constexpr auto elemValueType = ValueTypes<Elem>::value;
    return TracingProducerImpl<elemValueType, Elem>().produce(in, out);
  }
};

template<typename T>
struct TracingProducerImpl<ValueType::array, T> {
  T produce(nio::Source& in, nio::Sink& out) {
    using Elem = typename T::value_type;
    constexpr auto elemValueType = ValueTypes<Elem>::value;

    T ret;
    u64 size = readI32(in);
    if constexpr (IsArray<T>) {
      out.println("producing array.array");
      for (u64 i = 0; i < size; ++i) {
        ret[i] = TracingProducerImpl<elemValueType, Elem>().produce(in, out);
      }
    } else {
      out.println("producing array.vector");
      ret.reserve(size);
      for (u64 i = 0; i < size; ++i) {
        ret.emplace_back(TracingProducerImpl<elemValueType, Elem>().produce(in, out));
      }
    }
    return ret;
  }
};

// #TracingProducer -----------------------------------------------------------------------------------------

struct TracingProducer {
  template<ValueType ValueType, typename T>
  using Type = TracingProducerImpl<ValueType, T>;
};

// #TEST ----------------------------------------------------------------------------------------------------

// #TracingConsumer .........................................................................................

TEST(codec, TracingConsumerBool) {
  Encoder<TracingConsumer> encoder;
  nio::StringSink out;
  encoder.encode(true, out);
  encoder.encode(false, out);
  EXPECT_EQ((out.str()),
    "consuming boolean: true\n"
    "consuming boolean: false\n");
}

TEST(codec, TracingConsumerCharacter) {
  Encoder<TracingConsumer> encoder;
  nio::StringSink out;
  encoder.encode('a', out);
  encoder.encode(U'€', out);
  EXPECT_EQ((out.str()),
    "consuming character: 'a'\n"
    "consuming character: '€'\n");
}

TEST(codec, TracingConsumerString) {
  Encoder<TracingConsumer> encoder;
  nio::StringSink out;
  encoder.encode("hello"sv, out);
  encoder.encode(U"world"sv, out);
  EXPECT_EQ((out.str()),
    "consuming string: \"hello\"\n"
    "consuming string: \"world\"\n");
}

TEST(codec, TracingConsumerOptionalI32) {
  using type = std::optional<i32>;

  Encoder<TracingConsumer> encoder;
  nio::StringSink out;
  type val;
  encoder.encode(val, out);
  val = 42;
  encoder.encode(val, out);
  EXPECT_EQ((out.str()),
    "consuming optional: none\n"
    "consuming optional: optional(42)\n"
    "consuming integer: 42\n");
}

TEST(codec, TracingConsumerTuplePair) {
  Encoder<TracingConsumer> encoder;
  nio::StringSink out;
  encoder.encode(make_pair("answer"sv, 42), out);
  EXPECT_EQ((out.str()),
    "consuming tuple: 2\n"
    "consuming tuple elem: answer\n"
    "consuming string: \"answer\"\n"
    "consuming tuple elem: 42\n"
    "consuming integer: 42\n");
}

TEST(codec, TracingConsumerTupleTuple) {
  Encoder<TracingConsumer> encoder;
  nio::StringSink out;
  encoder.encode(make_tuple("answer"sv, 42, true), out);
  EXPECT_EQ((out.str()),
    "consuming tuple: 3\n"
    "consuming tuple elem: answer\n"
    "consuming string: \"answer\"\n"
    "consuming tuple elem: 42\n"
    "consuming integer: 42\n"
    "consuming tuple elem: true\n"
    "consuming boolean: true\n");
}

TEST(codec, TracingConsumerArrayArray) {
  Encoder<TracingConsumer> encoder;
  nio::StringSink out;
  encoder.encode(array { 1, 2, 3 }, out);
  EXPECT_EQ((out.str()),
    "consuming array: 3\n"
    "consuming integer: 1\n"
    "consuming integer: 2\n"
    "consuming integer: 3\n");
}

TEST(codec, TracingConsumerArrayVector) {
  Encoder<TracingConsumer> encoder;
  nio::StringSink out;
  encoder.encode(vector { 1, 2, 3, 4 }, out);
  EXPECT_EQ((out.str()),
    "consuming array: 4\n"
    "consuming integer: 1\n"
    "consuming integer: 2\n"
    "consuming integer: 3\n"
    "consuming integer: 4\n");
}

TEST(codec, TracingConsumerOptionalAndVectorInTypeLoop) {
  using type = optional<vector<optional<i32>>>;

  Encoder<TracingConsumer> encoder;

  {
    nio::StringSink out;
    type val = nullopt;
    encoder.encode(val, out);
    EXPECT_EQ(out.str(), "consuming optional: none\n");
  }

  {
    nio::StringSink out;
    type val = vector<optional<i32>> { optional<i32>(1), nullopt, optional<i32>(3) };
    encoder.encode(val, out);
    EXPECT_EQ(out.str(),
      "consuming optional: optional([optional(1), none, optional(3)])\n"
      "consuming array: 3\n"
      "consuming optional: optional(1)\n"
      "consuming integer: 1\n"
      "consuming optional: none\n"
      "consuming optional: optional(3)\n"
      "consuming integer: 3\n");
  }
}

TEST(codec, TracingConsumerVectorAndOptionalInTypeLoop) {
  using type = vector<optional<vector<i32>>>;

  Encoder<TracingConsumer> encoder;

  {
    nio::StringSink out;
    type val = {};
    encoder.encode(val, out);
    EXPECT_EQ(out.str(), "consuming array: 0\n");
  }

  {
    nio::StringSink out;
    type val = { optional<vector<i32>> { vector<i32> { 1, 2 } } };
    encoder.encode(val, out);
    EXPECT_EQ(out.str(),
      "consuming array: 1\n"
      "consuming optional: optional([1, 2])\n"
      "consuming array: 2\n"
      "consuming integer: 1\n"
      "consuming integer: 2\n");
  }
}

// #TracingProducer .........................................................................................

TEST(codec, TracingProducerBool) {
  using type = bool;
  Decoder<TracingProducer> decoder;
  nio::StringSource in("true");
  nio::StringSink out;
  auto val = decoder.decode<type>(in, out);
  EXPECT_EQ(val, true);
  EXPECT_EQ(out.str(), "producing boolean\n");
}

TEST(codec, TracingProducerString) {
  using type = string;
  Decoder<TracingProducer> decoder;
  nio::StringSource in("5Hello6Rocket");
  nio::StringSink out;
  auto val = decoder.decode<type>(in, out);
  EXPECT_EQ(val, "Hello"sv);
  val = decoder.decode<type>(in, out);
  EXPECT_EQ(val, "Rocket"sv);
  EXPECT_EQ(out.str(),
    "producing string\n"
    "producing string\n");
}

TEST(codec, TracingProducerStringView) {
  using type = string_view;
  Decoder<TracingProducer> decoder;
  nio::StringSource in("5Hello6Rocket");
  nio::StringSink out;
  auto val = decoder.decode<type>(in, out);
  // The decoder must return a string, not a string view
  static_assert(std::is_same_v<decltype(val), string>);
  EXPECT_EQ(val, "Hello"sv);
  auto optVal = decoder.tryDecode<type>(in, out);
  // The decoder must return an optional string, not an optional string view
  static_assert(std::is_same_v<decltype(optVal), std::optional<string>>);
  EXPECT_EQ(*optVal, "Rocket"sv);
  EXPECT_EQ(out.str(),
    "producing string\n"
    "producing string\n");
}

TEST(codec, TracingProducerOptionalBool) {
  using type = optional<bool>;
  Decoder<TracingProducer> decoder;

  {
    nio::StringSource in("none");
    nio::StringSink out;
    type val = decoder.decode<type>(in, out);
    EXPECT_EQ(val, nullopt);
    EXPECT_EQ(out.str(), "producing optional\n");
  }

  {
    nio::StringSource in("false");
    nio::StringSink out;
    type val = decoder.decode<type>(in, out);
    EXPECT_EQ(val, false);
    EXPECT_EQ(out.str(),
      "producing optional\n"
      "producing boolean\n");
  }
}

TEST(codec, TracingProducerArrayArray) {
  using type = array<i32, 3>;
  Decoder<TracingProducer> decoder;
  nio::StringSource in("3012");
  nio::StringSink out;
  type val = decoder.decode<type>(in, out);
  EXPECT_EQ(val, (array<i32, 3> { 0, 1, 2 }));
  EXPECT_EQ(out.str(),
    "producing array.array\n"
    "producing integer\n"
    "producing integer\n"
    "producing integer\n");
}

TEST(codec, TracingProducerArrayVector) {
  using type = vector<i32>;
  Decoder<TracingProducer> decoder;
  nio::StringSource in("43210");
  nio::StringSink out;
  type val = decoder.decode<type>(in, out);
  EXPECT_EQ(val, (vector<i32> { 3, 2, 1, 0 }));
  EXPECT_EQ(out.str(),
    "producing array.vector\n"
    "producing integer\n"
    "producing integer\n"
    "producing integer\n"
    "producing integer\n");
}

// EOF
