/*
 * test-codec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/codec.h"
#include "rocket/nio/nio.h"
#include "rocket/reflect/reflect-codec.h"
#include "rocket/unicode/ConvertTo.h"

#include <fmt/ranges.h>
#include <fmt/std.h>

using namespace rocket;
using namespace rocket::codec;

// #MyStruct ------------------------------------------------------------------------------------------------

struct MyStruct {
  i32 ärger;
  bool ökonom;
  string übermut;
  vector<i32> vec;

  ROCKET_REFLECT_MEMBERS(MyStruct, index, (ärger)(ökonom)(übermut)(vec));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyStruct, index);
ROCKET_REFLECT_MEMBERS_DEFINE(, MyStruct, index);

// Functions ------------------------------------------------------------------------------------------------

// Tests if #str is available in #in
bool
read(nio::Source& in, std::string_view str) {
  auto pos = in.tell();
  string buf(str.size(), ' ');
  const auto n = in.read(buf);
  bool ret = n == buf.size() && buf == str;
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
struct TracingConsumerImpl<ValueType::Bool, bool> {
  u64
  consume(bool val, nio::StringSink& out) {
    return out.println("consuming boolean: {}", val);
  }
};

template<typename C>
struct TracingConsumerImpl<ValueType::Char, C> {
  u64
  consume(C val, nio::StringSink& out) {
    std::basic_string<C> str = { val };
    return out.println("consuming character: '{}'", unicode::ConvertTo<char>::apply(str));
  }
};

template<typename I>
struct TracingConsumerImpl<ValueType::Integer, I> {
  u64
  consume(I val, nio::StringSink& out) {
    return out.println("consuming integer: {}", val);
  }
};

template<typename T>
struct TracingConsumerImpl<ValueType::String, T> {
  u64
  consume(const T& val, nio::StringSink& out) {
    return out.println("consuming string: {:?}", unicode::ConvertTo<char>::apply(val));
  }
};

template<typename T>
struct TracingConsumerImpl<ValueType::Optional, T> {
  u64
  consume(const T& val, nio::StringSink& out) {
    auto ret = out.println("consuming optional: {}", val);
    if (val) {
      using Elem = typename T::value_type;
      constexpr auto elemValueType = ValueTypes<Elem>::value;
      ret += TracingConsumerImpl<elemValueType, Elem>().consume(*val, out);
    }
    return ret;
  }
};

// For #MemberRef, the tuple consumer must be able to pass additional arguments to the element consumer
template<typename T>
struct TracingConsumerImpl<ValueType::Tuple, T> {
  template<typename... Args>
  u64
  consume(const T& val, nio::StringSink& out, Args&&... args) {
    const auto size = std::tuple_size<T>::value;
    auto ret = out.println("consuming tuple: {}", size);
    std::apply([&](auto&&... arg) {
      (consumeElem(ret, std::forward<decltype(arg)>(arg), out, std::forward<Args>(args)...), ...);
    }, val);
    return ret;
  }

private:

  template<typename Elem, typename... Args>
  u64
  consumeElem(u64& result, const Elem& elem, nio::StringSink& out, Args&&... args) {
    if constexpr (fmt::is_formattable<Elem>::value) {
      result += out.println("consuming tuple elem: {}", elem);
    } else {
      result += out.println("consuming tuple elem");
    }
    constexpr auto elemValueType = ValueTypes<Elem>::value;
    result += TracingConsumerImpl<elemValueType, Elem>().consume(elem, out, std::forward<Args>(args)...);
    return result;
  }
};

template<typename T>
struct TracingConsumerImpl<ValueType::Array, T> {
  u64
  consume(const T& val, nio::StringSink& out) {
    using Elem = typename T::value_type;
    constexpr auto elemValueType = ValueTypes<Elem>::value;

    const auto size = val.size();
    auto ret = out.println("consuming array: {}", size);
    for (const auto& elem : val) {
      ret += TracingConsumerImpl<elemValueType, Elem>().consume(elem, out);
    }
    return ret;
  }
};

template<typename T>
struct TracingConsumerImpl<ValueType::MemberRefProvider, T> {
  u64
  consume(const T& val, nio::StringSink& out) {
    auto ret = out.println("consuming memberrefprovider");

    constexpr auto& refs = rocket::reflect::MemberRefProvider<T>::refs;
    using Elem = PurgeType<decltype(refs)>;
    constexpr auto elemValueType = ValueTypes<Elem>::value;
    static_assert(elemValueType == ValueType::Tuple);
    // Here we have to pass an additional argument, the instance, to the tuple consumer
    ret += TracingConsumerImpl<elemValueType, Elem>().consume(refs, out, val);
    return ret;
  }
};

template<typename T>
struct TracingConsumerImpl<ValueType::MemberRef, T> {
  template<typename C>
  u64
  consume(const T& val, nio::StringSink& out, const C& instance) {
    auto ret = out.println("consuming memberref: \"{}\"", val.name());

    using Elem = typename T::ValueType;
    constexpr auto elemValueType = ValueTypes<Elem>::value;
    ret += TracingConsumerImpl<elemValueType, Elem>().consume(val.get(instance), out);
    return ret;
  }
};

template<typename T>
struct TracingConsumerImpl<ValueType::VarRef, T> {
  u64
  consume(const T& val, nio::StringSink& out) {
    auto ret = out.println("consuming varref: \"{}\"", val.name());

    using Elem = typename T::ValueType;
    constexpr auto elemValueType = ValueTypes<Elem>::value;
    ret += TracingConsumerImpl<elemValueType, Elem>().consume(val.get(), out);
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
struct TracingProducerImpl<ValueType::Bool, bool> {
  void
  produce(bool& val, nio::Source& in, nio::Sink& out) {
    out.println("producing boolean");
    if (read(in, "false")) {
      val = false;
      return;
    }
    if (read(in, "true")) {
      val = true;
      return;
    }
    throw InputFailure(in.tell(), "expected boolean");
  }
};

template<typename I>
struct TracingProducerImpl<ValueType::Integer, I> {
  void
  produce(I& val, nio::Source& in, nio::Sink& out) {
    out.println("producing integer");
    val = readI32(in);
  }
};

template<typename T>
struct TracingProducerImpl<ValueType::String, T> {
  using C = typename T::value_type;
  static_assert(std::is_same_v<C, char>, "Cannot decode UTF-32 string");
  static_assert(std::is_same_v<T, std::basic_string<C>>, "Cannot decode string view");

  void
  produce(T& val, nio::Source& in, nio::Sink& out) {
    out.println("producing string");
    i32 size = readI32(in);
    val = T(size, ' ');
    const auto n = in.read(val);
    if (n != val.size()) {
      throw InputFailure(in.tell(), "expected string");
    }
  }
};

template<typename T>
struct TracingProducerImpl<ValueType::Optional, T> {
  void
  produce(T& val, nio::Source& in, nio::Sink& out) {
    out.println("producing optional");
    if (read(in, "none")) {
      val = nullopt;
      return;
    }
    using Elem = typename T::value_type;
    constexpr auto elemValueType = ValueTypes<Elem>::value;
    val = Elem();
    TracingProducerImpl<elemValueType, Elem>().produce(*val, in, out);
  }
};

template<typename T>
struct TracingProducerImpl<ValueType::Array, T> {
  void
  produce(T& val, nio::Source& in, nio::Sink& out) {
    using Elem = typename T::value_type;
    constexpr auto elemValueType = ValueTypes<Elem>::value;

    u64 size = readI32(in);
    if constexpr (IsArray<T>) {
      out.println("producing array.array");
      for (u64 i = 0; i < size; ++i) {
        TracingProducerImpl<elemValueType, Elem>().produce(val[i], in, out);
      }
    } else {
      out.println("producing array.vector");
      val = T(size);
      for (u64 i = 0; i < size; ++i) {
        TracingProducerImpl<elemValueType, Elem>().produce(val[i], in, out);
      }
    }
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
  auto result = encoder.encode(true, out);
  static_assert(is_same_v<decltype(result), u64>);
  encoder.encode(false, out);
  EXPECT_EQ(out.str(),
    "consuming boolean: true\n"
    "consuming boolean: false\n");
}

TEST(codec, TracingConsumerCharacter) {
  Encoder<TracingConsumer> encoder;
  nio::StringSink out;
  encoder.encode('a', out);
  encoder.encode(U'€', out);
  EXPECT_EQ(out.str(),
    "consuming character: 'a'\n"
    "consuming character: '€'\n");
}

TEST(codec, TracingConsumerString) {
  Encoder<TracingConsumer> encoder;
  nio::StringSink out;
  encoder.encode("hello"sv, out);
  encoder.encode(U"world"sv, out);
  EXPECT_EQ(out.str(),
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
  EXPECT_EQ(out.str(),
    "consuming optional: none\n"
    "consuming optional: optional(42)\n"
    "consuming integer: 42\n");
}

TEST(codec, TracingConsumerTuplePair) {
  Encoder<TracingConsumer> encoder;
  nio::StringSink out;
  encoder.encode(make_pair("answer"sv, 42), out);
  EXPECT_EQ(out.str(),
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
  EXPECT_EQ(out.str(),
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
  EXPECT_EQ(out.str(),
    "consuming array: 3\n"
    "consuming integer: 1\n"
    "consuming integer: 2\n"
    "consuming integer: 3\n");
}

TEST(codec, TracingConsumerArrayVector) {
  Encoder<TracingConsumer> encoder;
  nio::StringSink out;
  encoder.encode(vector { 1, 2, 3, 4 }, out);
  EXPECT_EQ(out.str(),
    "consuming array: 4\n"
    "consuming integer: 1\n"
    "consuming integer: 2\n"
    "consuming integer: 3\n"
    "consuming integer: 4\n");
}

TEST(codec, TracingConsumerMemberRef) {
  Encoder<TracingConsumer> encoder;
  nio::StringSink out;
  MyStruct val { 42, true, "hello", { 1, 2, 3 } };
  encoder.encode(val, out);
  EXPECT_EQ(out.str(),
    "consuming memberrefprovider\n"
    "consuming tuple: 4\n"
    "consuming tuple elem\n"
    "consuming memberref: \"ärger\"\n"
    "consuming integer: 42\n"
    "consuming tuple elem\n"
    "consuming memberref: \"ökonom\"\n"
    "consuming boolean: true\n"
    "consuming tuple elem\n"
    "consuming memberref: \"übermut\"\n"
    "consuming string: \"hello\"\n"
    "consuming tuple elem\n"
    "consuming memberref: \"vec\"\n"
    "consuming array: 3\n"
    "consuming integer: 1\n"
    "consuming integer: 2\n"
    "consuming integer: 3\n");
}

TEST(codec, TracingConsumerVarRef) {
  Encoder<TracingConsumer> encoder;
  nio::StringSink out;
  i32 a = 42;
  bool b = true;
  string c = "hello";
  auto vars = ROCKET_REFLECT_VARS((a)(b)(c));
  encoder.encode(vars, out);
  EXPECT_EQ(out.str(),
    "consuming tuple: 3\n"
    "consuming tuple elem: a=42\n"
    "consuming varref: \"a\"\n"
    "consuming integer: 42\n"
    "consuming tuple elem: b=true\n"
    "consuming varref: \"b\"\n"
    "consuming boolean: true\n"
    "consuming tuple elem: c=hello\n"
    "consuming varref: \"c\"\n"
    "consuming string: \"hello\"\n");
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
  type val = decoder.decode<type>(in, out);
  EXPECT_EQ(val, true);
  EXPECT_EQ(out.str(), "producing boolean\n");
}

TEST(codec, TracingProducerString) {
  using type = string;
  Decoder<TracingProducer> decoder;
  nio::StringSource in("5Hello6Rocket");
  nio::StringSink out;
  type val = decoder.decode<type>(in, out);
  EXPECT_EQ(val, "Hello"sv);
  val = decoder.decode<type>(in, out);
  EXPECT_EQ(val, "Rocket"sv);
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
