/*
 * test-spanstream.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/io/spanstream.h"

#include <array>
#include <string_view>
#include <type_traits>
#include <utility>

// Static checks --------------------------------------------------------------------------------------------

// `ispanstream` takes read-only ranges, `ospanstream` and `spanstream` do not

static_assert(std::is_constructible_v<io::ispanstream, std::string_view>);
static_assert(std::is_constructible_v<io::ispanstream, std::span<const char>>);
static_assert(std::is_constructible_v<io::ispanstream, std::span<char>>);
static_assert(!std::is_constructible_v<io::ospanstream, std::span<const char>>);
static_assert(!std::is_constructible_v<io::spanstream, std::span<const char>>);

// `ISpanStream::span` returns a read-only span

static_assert(std::is_same_v<decltype(std::declval<io::ISpanStream&>().span()), std::span<const char>>);
static_assert(std::is_same_v<decltype(std::declval<io::OSpanStream&>().span()), std::span<char>>);
static_assert(std::is_same_v<decltype(std::declval<io::SpanStream&>().span()), std::span<char>>);

// No implicit conversion from a range

static_assert(!std::is_convertible_v<std::string_view, io::ISpanStream>);

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(SpanStream, ISpanStream) {
  const string_view input = "12 34";

  io::ISpanStream is(input);
  EXPECT_EQ(is.span().data(), input.data());
  EXPECT_EQ(is.span().size(), input.size());

  int a = 0;
  int b = 0;
  is >> a >> b;
  EXPECT_EQ(a, 12);
  EXPECT_EQ(b, 34);
  EXPECT_TRUE(is.eof());

  // Reading past the end fails

  is >> a;
  EXPECT_TRUE(is.fail());

  // Setting a new span resets the input position

  is.clear();
  is.span(string_view("56"));
  is >> a;
  EXPECT_EQ(a, 56);

  // Seeking

  is.clear();
  is.span(input);
  is.seekg(3);
  is >> a;
  EXPECT_EQ(a, 34);

  is.clear();
  is.seekg(-2, ios::end);
  is >> a;
  EXPECT_EQ(a, 34);

  // Writing is not possible

  EXPECT_EQ(is.rdbuf()->sputc('x'), char_traits<char>::eof());
}

TEST(SpanStream, ISpanStreamEmpty) {
  io::ISpanStream is(span<const char>{});
  EXPECT_TRUE(is.span().empty());

  int a = 0;
  is >> a;
  EXPECT_TRUE(is.fail());
  EXPECT_TRUE(is.eof());
}

TEST(SpanStream, ISpanStreamMove) {
  const string_view input = "12 34";

  io::ISpanStream is(input);
  int a = 0;
  is >> a;
  EXPECT_EQ(a, 12);

  io::ISpanStream is2(std::move(is));
  EXPECT_EQ(static_cast<istream&>(is2).rdbuf(), is2.rdbuf());
  is2 >> a;
  EXPECT_EQ(a, 34);

  io::ISpanStream is3(string_view("56"));
  is3 = std::move(is2);
  EXPECT_EQ(static_cast<istream&>(is3).rdbuf(), is3.rdbuf());
  EXPECT_EQ(is3.span().data(), input.data());
}

TEST(SpanStream, ISpanStreamSwap) {
  const string_view input1 = "1";
  const string_view input2 = "2";

  io::ISpanStream is1(input1);
  io::ISpanStream is2(input2);
  swap(is1, is2);

  EXPECT_EQ(static_cast<istream&>(is1).rdbuf(), is1.rdbuf());
  EXPECT_EQ(static_cast<istream&>(is2).rdbuf(), is2.rdbuf());

  int a = 0;
  is1 >> a;
  EXPECT_EQ(a, 2);
  is2 >> a;
  EXPECT_EQ(a, 1);
}

TEST(SpanStream, OSpanStream) {
  array<char, 8> buf{};

  io::OSpanStream os(buf);
  EXPECT_TRUE(os.span().empty());

  os << 12 << ' ' << 34;
  EXPECT_TRUE(os.good());
  EXPECT_EQ(string_view(os.span().data(), os.span().size()), "12 34");
  EXPECT_EQ(os.span().data(), buf.data());

  // Writing past the end fails

  os << "5678";
  EXPECT_TRUE(os.bad());
  EXPECT_EQ(string_view(os.span().data(), os.span().size()), "12 34567");

  // Setting a new span resets the output position

  os.clear();
  os.span(buf);
  os << "ab";
  EXPECT_EQ(string_view(os.span().data(), os.span().size()), "ab");
  EXPECT_EQ(string_view(buf.data(), buf.size()), "ab 34567");

  // Seeking

  os.seekp(1);
  os << 'X';
  EXPECT_EQ(string_view(os.span().data(), os.span().size()), "aX");

  // Not checked: That reading is not possible. libstdc++ (GCC 16) wrongly opens the buffer with
  // `std::ios_base::in` instead of `std::ios_base::out`
}

TEST(SpanStream, OSpanStreamAte) {
  array<char, 8> buf = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

  // `std::ios_base::out` is always added

  io::OSpanStream os(span<char>(buf.data(), 4), ios::ate);
  EXPECT_EQ(os.span().size(), 4U);

  os << 'x';
  EXPECT_TRUE(os.bad());
}

TEST(SpanStream, OSpanStreamMove) {
  array<char, 8> buf{};

  io::OSpanStream os(buf);
  os << "ab";

  io::OSpanStream os2(std::move(os));
  EXPECT_EQ(static_cast<ostream&>(os2).rdbuf(), os2.rdbuf());
  os2 << "cd";
  EXPECT_EQ(string_view(os2.span().data(), os2.span().size()), "abcd");

  array<char, 4> buf3{};
  io::OSpanStream os3(buf3);
  os3 = std::move(os2);
  EXPECT_EQ(static_cast<ostream&>(os3).rdbuf(), os3.rdbuf());
  os3 << "e";
  EXPECT_EQ(string_view(os3.span().data(), os3.span().size()), "abcde");
}

TEST(SpanStream, SpanStream) {
  array<char, 8> buf{};

  io::SpanStream ss(buf);
  ss << 12 << ' ' << 34;
  EXPECT_EQ(string_view(ss.span().data(), ss.span().size()), "12 34");

  int a = 0;
  int b = 0;
  ss >> a >> b;
  EXPECT_EQ(a, 12);
  EXPECT_EQ(b, 34);
}

TEST(SpanStream, SpanStreamIn) {
  array<char, 5> buf = {'1', '2', ' ', '3', '4'};

  // Opened for input only, `span` returns the whole buffer

  io::SpanStream ss(buf, ios::in);
  EXPECT_EQ(ss.span().size(), buf.size());

  int a = 0;
  int b = 0;
  ss >> a >> b;
  EXPECT_EQ(a, 12);
  EXPECT_EQ(b, 34);
}

// EOF
