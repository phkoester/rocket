/*
 * test-FormattedCodec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/FormattedCodec.h"
#include "rocket/log/log.h"
#include "rocket/reflect/reflect-codec.h"

using namespace rocket::codec;
using namespace std;

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

// #TEST ----------------------------------------------------------------------------------------------------

// #FormattedConsumer .......................................................................................

TEST(FormattedCodec, FormattedConsumerBool) {
  FormattedCodec codec;
  nio::StringSink out;
  codec.encode(true, out);
  EXPECT_EQ(out.str(), "true");
}

TEST(FormattedCodec, FormattedConsumerChar) {
  FormattedCodec codec;
  nio::StringSink out;
  codec.encode('\t', out);
  EXPECT_EQ(out.str(), "'\\t'");
  codec.encode(U'€', out);
  EXPECT_EQ(out.str(), "'\\t''€'");
}

TEST(FormattedCodec, FormattedConsumerEnum) {
  enum Color { Red, Green, Blue };
  FormattedCodec codec;
  nio::StringSink out;
  codec.encode(Blue, out);
  EXPECT_EQ(out.str(), "2");
  codec.encode(log::LogLevel::info, out);
  EXPECT_EQ(out.str(), "2info");
}

TEST(FormattedCodec, FormattedConsumerIntegerI64) {
  FormattedCodec codec;
  nio::StringSink out;
  codec.encode(-42_i64, out);
  EXPECT_EQ(out.str(), "-42");
}

TEST(FormattedCodec, FormattedConsumerFloatF64) {
  FormattedCodec codec;
  nio::StringSink out;
  codec.encode(-123.456_f64, out);
  EXPECT_EQ(out.str(), "-123.456");
}

TEST(FormattedCodec, FormattedConsumerPointer) {
  FormattedCodec codec;

  {
    nio::StringSink out;
    nio::StringSink* val = nullptr;
    codec.encode(val, out);
    EXPECT_EQ(out.str(), "<null>");
  }

  {
    nio::StringSink out;
    codec.encode(&codec, out);
    EXPECT_THAT(out.str(), matchesRegex("0x[0-9a-f]+"));
  }
}

TEST(FormattedCodec, FormattedConsumerString) {
  FormattedCodec codec;

  {
    nio::StringSink out;
    codec.encode("Hello"sv, out);
    EXPECT_EQ(out.str(), "\"Hello\"");
  }

  {
    nio::StringSink out;
    codec.encode(U"Hello"sv, out);
    EXPECT_EQ(out.str(), "\"Hello\"");
  }

  {
    nio::StringSink out;
    codec.encode("\x7f"sv, out);
    EXPECT_EQ(out.str(), "\"\\x7F\"");
  }
}

TEST(FormattedCodec, FormattedConsumerOptional) {
  FormattedCodec codec;

  {
    nio::StringSink out;
    optional<string> val;
    codec.encode(val, out);
    EXPECT_EQ(out.str(), "<none>");
  }

  {
    nio::StringSink out;
    optional<string> val = "Hello";
    codec.encode(val, out);
    EXPECT_EQ(out.str(), "\"Hello\"");
  }
}

TEST(FormattedCodec, FormattedConsumerTuplePair) {
  FormattedCodec codec;

  {
    nio::StringSink out;
    codec.encode(make_pair("answer"sv, 42), out);
    EXPECT_EQ(out.str(), "(\"answer\", 42)");
  }

  {
    nio::StringSink out;
    codec.encode(make_pair("answer"sv, 42), out, { .indent=true });
    EXPECT_EQ(out.str(),
      "(\n"
      "  \"answer\",\n"
      "  42\n"
      ")");
  }
}

TEST(FormattedCodec, FormattedConsumerArray) {
  FormattedCodec codec;

  {
    vector<i32> vec = { 1, 2, 3 };
    span<i32> val = vec;
    nio::StringSink out;
    codec.encode(val, out);
    EXPECT_EQ(out.str(), "[1, 2, 3]");
  }

  {
    vector<vector<i32>> val = { { 1, 2, 3 }, { 4, 5, 6 } };
    nio::StringSink out;
    codec.encode(val, out);
    EXPECT_EQ(out.str(), "[[1, 2, 3], [4, 5, 6]]");
  }

  {
    vector<vector<i32>> val = { { 1, 2, 3 }, { 4, 5, 6 } };
    nio::StringSink out;
    codec.encode(val, out, { .indent=true });
    EXPECT_EQ(out.str(),
      "[\n"
      "  [\n"
      "    1,\n"
      "    2,\n"
      "    3\n"
      "  ],\n"
      "  [\n"
      "    4,\n"
      "    5,\n"
      "    6\n"
      "  ]\n"
      "]");
  }
}

TEST(FormattedCodec, FormattedConsumerSet) {
  FormattedCodec codec;

  {
    nio::StringSink out;
    set<i32> val = { 1, 2, 3 };
    codec.encode(val, out);
    EXPECT_EQ(out.str(), "{1, 2, 3}");
  }

  {
    nio::StringSink out;
    set<i32> val;
    codec.encode(val, out);
    EXPECT_EQ(out.str(), "{}");
  }
}

TEST(FormattedCodec, FormattedConsumerMap) {
  FormattedCodec codec;
  nio::StringSink out;
  map<string, i32> val = { { "alpha", 1 }, { "beta", 2 }, { "gamma", 3 } };
  codec.encode(val, out);
  EXPECT_EQ(out.str(), "{\"alpha\": 1, \"beta\": 2, \"gamma\": 3}");
}

TEST(FormattedCodec, FormattedConsumerMemberRefProvider) {
  FormattedCodec codec;
  nio::StringSink out;
  MyStruct val { 42, true, "hello", { 1, 2, 3 } };
  codec.encode(val, out);
  EXPECT_EQ(out.str(), "(ärger=42, ökonom=true, übermut=\"hello\", vec=[1, 2, 3])");
}

// #FormattedProducer .......................................................................................

TEST(FormattedCodec, FormattedProducerBool) {
  FormattedCodec codec;
  FormattedProducerConfig config { .cComments=true, .shellComments=true };

  {
    nio::StringSource in("  /* comment\nanother line in the comment */\r\n# comment\n\ttRUe");
    bool val = codec.decode<bool>(in, config);
    EXPECT_EQ(val, true);
  }

  {
    nio::StringSource in("\r\n  1");
    bool val = codec.decode<bool>(in, config);
    EXPECT_EQ(val, true);
  }

  {
    nio::StringSource in("\r\nx");
    EXPECT_THAT(
      [&] { in.seek(0); codec.decode<bool>(in, config); },
      throwsInputFailure(2, HasSubstr("Expected a boolean value")));
  }
}

TEST(FormattedCodec, FormattedProducerChar) {
  FormattedCodec codec;

  {
    nio::StringSource in("'a'");
    EXPECT_EQ(codec.decode<char>(in), 'a');
  }

  {
    nio::StringSource in("'\\''");
    EXPECT_EQ(codec.decode<char>(in), '\'');
  }

  {
    nio::StringSource in("'\\t'");
    EXPECT_EQ(codec.decode<char>(in), '\t');
  }

  {
    nio::StringSource in("  'ä'");
    EXPECT_THAT(
      [&] { in.seek(0); codec.decode<char>(in); },
      throwsInputFailure(2, HasSubstr("Invalid character literal")));
  }

  {
    nio::StringSource in("'ä'");
    EXPECT_EQ(codec.decode<char32>(in), U'ä');
  }

  {
    nio::StringSource in("'\u20ac'");
    EXPECT_EQ(codec.decode<char32>(in), U'€');
  }
}

TEST(FormattedCodec, FormattedProducerEnum) {
  FormattedCodec codec;

  {
    enum Color { Red, Green, Blue };
    nio::StringSource in("2");
    EXPECT_EQ(codec.decode<Color>(in), Blue);
  }

  {
    nio::StringSource in("  info  ");
    EXPECT_EQ(codec.decode<log::LogLevel>(in), log::LogLevel::info);
    EXPECT_EQ(in.tell(), 6);
  }
}

TEST(FormattedCodec, FormattedProducerInteger) {
  FormattedCodec codec;

  {
    nio::StringSource in("-42");
    EXPECT_EQ(codec.decode<i32>(in), -42);
  }

  {
    nio::StringSource in("0xABcd");
    EXPECT_EQ(codec.decode<i32>(in), 0xABCD);
  }

  {
    nio::StringSource in("  x");
    EXPECT_THAT(
      [&] { in.seek(0); codec.decode<i32>(in); },
      throwsInputFailure(2, HasSubstr("Expected an integer value")));
  }
}

TEST(FormattedCodec, FormattedProducerFloat) {
  FormattedCodec codec;

  {
    nio::StringSource in("  -123.456  ");
    EXPECT_EQ(codec.decode<f64>(in), -123.456);
    EXPECT_EQ(in.tell(), 10);
  }
}

TEST(FormattedCodec, FormattedProducerPointer) {
  FormattedCodec codec;

  {
    nio::StringSource in("  <null>  ");
    EXPECT_EQ(codec.decode<void*>(in), nullptr);
    EXPECT_EQ(in.tell(), 8);
  }

  {
    nio::StringSource in("  0x12345678  ");
    EXPECT_EQ(codec.decode<void*>(in), reinterpret_cast<void*>(0x12345678));
    EXPECT_EQ(in.tell(), 12);
  }
}

TEST(FormattedCodec, FormattedProducerOptionalString) {
  FormattedCodec codec;

  {
    nio::StringSource in("<none>");
    EXPECT_EQ(codec.decode<optional<string>>(in), nullopt);
  }

  {
    nio::StringSource in("\"Hello\"");
    EXPECT_EQ(codec.decode<optional<basic_string_view<char32>>>(in), U"Hello"sv);
  }
}

TEST(FormattedCodec, FormattedProducerTuple) {
  FormattedCodec codec;
  FormattedProducerConfig config { .cComments=true, .shellComments=true };

  {
    nio::StringSource in("  (  \"answer\"   , 42   )");
    EXPECT_EQ((codec.decode<pair<string, i32>>(in, config)), make_pair("answer"s, 42_i32));
  }

  {
    nio::StringSource in("  (  # comment\n \"answer\"   , 42  /* comment */  , // comment\n  )");
    EXPECT_EQ((codec.decode<pair<string, i32>>(in, config)), make_pair("answer"s, 42_i32));
  }

  {
    nio::StringSource in("()");
    EXPECT_EQ((codec.decode<tuple<>>(in, config)), make_tuple());
  }

  {
    nio::StringSource in("(1, 2, true)");
    EXPECT_EQ((codec.decode<tuple<i32, i32, bool>>(in, config)), make_tuple(1_i32, 2_i32, true));
  }
}

TEST(FormattedCodec, FormattedProducerArray) {
  FormattedCodec codec;
  FormattedProducerConfig config { .cComments=true, .shellComments=true };

  {
    nio::StringSource in("  [ 1, 2, 3   ]");
    EXPECT_EQ((codec.decode<array<i32, 3>>(in, config)), (array<i32, 3> { 1, 2, 3 }));
  }

  {
    nio::StringSource in("  [ 1, 2, 3   , ]");
    EXPECT_EQ((codec.decode<array<i32, 3>>(in, config)), (array<i32, 3> { 1, 2, 3 }));
  }

  {
    nio::StringSource in("[]");
    EXPECT_EQ((codec.decode<vector<i32>>(in, config)), (vector<i32> { }));
  }

  {
    nio::StringSource in("[1, 2, 3]");
    EXPECT_EQ((codec.decode<vector<i32>>(in, config)), (vector<i32> { 1, 2, 3 }));
  }

  {
    nio::StringSource in("[1, 2, 3,]");
    EXPECT_EQ((codec.decode<vector<i32>>(in, config)), (vector<i32> { 1, 2, 3 }));
  }

  {
    nio::StringSource in("  [ 1, 2, 3   , ]");
    EXPECT_EQ((codec.decode<vector<i32>>(in, config)), (vector<i32> { 1, 2, 3 }));
  }
}

TEST(FormattedCodec, FormattedProducerSet) {
  FormattedCodec codec;
  FormattedProducerConfig config { .cComments=true, .shellComments=true };

  {
    nio::StringSource in("  { 1, 2, 3  ,  }   ");
    EXPECT_EQ((codec.decode<set<i32>>(in, config)), (set<i32> { 1, 2, 3 }));
  }
}

TEST(FormattedCodec, FormattedProducerBimap) {
  FormattedCodec codec;
  FormattedProducerConfig config { .cComments=true, .shellComments=true };

  {
    nio::StringSource in("  { \"alpha\"\t: 1, \"beta\"  :/* comment */ 2, \"gamma\": 3  ,  }   ");
    using type = Bimap<string, i32>;
    type val = makeBimap<string, i32>({ { "alpha", 1 }, { "beta", 2 }, { "gamma", 3 } });
    EXPECT_EQ(codec.decode<type>(in, config), val);
  }
}

TEST(FormattedCodec, FormattedProducerMemberRefProvider) {
  FormattedCodec codec;
  FormattedProducerConfig config { .cComments=true, .shellComments=true };

  {
    nio::StringSource in("  ( ärger  =  42, ökonom=true, übermut=\"hello\", vec=[1, 2, 3] )   ");
    MyStruct val { 42, true, "hello", { 1, 2, 3 } };
    EXPECT_EQ(codec.decode<MyStruct>(in, config), val);
  }
}

// EOF
