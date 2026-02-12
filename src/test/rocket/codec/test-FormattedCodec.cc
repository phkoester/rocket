/*
 * test-FormattedCodec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/FormattedCodec.h"
#include "rocket/log/log.h"
#include "rocket/reflect/reflect.h"

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

// EOF
