/*
 * test-FormattedCodec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/FormattedCodec.h"

using namespace rocket::codec;

// #TEST ----------------------------------------------------------------------------------------------------

TEST(FormattedCodec, FormattedConsumerBool) {
  FormattedCodec codec;
  nio::StringSink out;
  codec.encode(true, out);
  EXPECT_EQ(out.str(), "true");
}

#if 0

TEST(Formatted, optionalI32) {
  using type = std::optional<i32>;

  EXPECT_EQ(encode<Formatted>(type()), "<none>");
  EXPECT_EQ(encode<Formatted>(type(42)), "42");

  EXPECT_EQ((decode<Formatted, type>("<none>"sv)), nullopt);
  EXPECT_EQ((decode<Formatted, type>("42"sv)), 42);
}

TEST(Formatted, char) {
  EXPECT_EQ(encode<Formatted>('a'), "'a'");
  EXPECT_EQ((decode<Formatted, char>("'a'"sv)), 'a');

  EXPECT_EQ(encode<Formatted>('\x01'), "'\\x01'");
  EXPECT_EQ((decode<Formatted, char>("'\\x01'"sv)), '\x01');
}

TEST(Formatted, optionalAndVectorInTypeLoopFormat) {
  using type = optional<vector<optional<i32>>>;

  type val1 = nullopt;
  EXPECT_EQ(encode<Formatted>(val1), "<none>");
  type val2 = vector<optional<i32>> { optional<i32>(1), nullopt, optional<i32>(3) };
  EXPECT_EQ(encode<Formatted>(val2), "[1, <none>, 3]");

  EXPECT_EQ((decode<Formatted, type>("<none>"sv)), nullopt);
  EXPECT_EQ((decode<Formatted, type>("[1, <none>, 3]"sv)), (type { 1, nullopt, 3 }));
}

TEST(Formatted, pair) {
  using type = std::pair<i32, bool>;

  EXPECT_EQ(encode<Formatted>(type { 1, true }), "(1, true)");
  EXPECT_EQ((decode<Formatted, type>("(1, true)"sv)), (type { 1, true }));
}

TEST(Formatted, tuple) {
  using type = std::tuple<i32, bool, u64>;

  EXPECT_EQ(encode<Formatted>(type { 1, true, 42 }), "(1, true, 42)");
  EXPECT_EQ((decode<Formatted, type>("(1, true, 42)"sv)), (type { 1, true, 42 }));
}

TEST(Formatted, vectoru8) {
  using type = std::vector<u8>;

  EXPECT_EQ(encode<Formatted>(type()), "[]");
  EXPECT_EQ(encode<Formatted>(type({ 1, 2, 3 })), "[1, 2, 3]");

  EXPECT_EQ((decode<Formatted, type>("[]"sv)), type());
  EXPECT_EQ((decode<Formatted, type>("[1, 2, 3]"sv)), (type { 1, 2, 3 }));
}

TEST(Formatted, vectorAndOptionalInTypeLoopFormat) {
  using type = vector<optional<vector<i32>>>;

  type val1 = {};
  EXPECT_EQ(encode<Formatted>(val1), "[]");
  type val2 = type { vector<i32> { vector<i32> { 1, 2 } } };
  EXPECT_EQ(encode<Formatted>(val2), "[[1, 2]]");

  EXPECT_EQ((decode<Formatted, type>("[]"sv)), (type {}));
  EXPECT_EQ((decode<Formatted, type>("[[1, 2]]"sv)), (type { vector<i32> { 1, 2 } }));
}
#endif

// EOF
