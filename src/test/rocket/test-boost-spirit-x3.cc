/*
 * test-boost-spirit-x3.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/experimental/boost-spirit-x3.h"

#include <boost/fusion/adapted/std_pair.hpp>
#include <boost/spirit/home/x3.hpp>

using namespace std;

namespace x3 = boost::spirit::x3;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(boost_spirit_x3, parseComplex) {
  using Attr = pair<double, double>;

  auto parse = [] (string_view in, Attr& attr) {
    return x3::phrase_parse(
        in.begin(), in.end(),
        '(' >> x3::double_ >> ',' >> x3::double_ >> ')',
        x3::space,
        attr);
  };

  Attr attr;

  EXPECT_TRUE(parse("(1.1,-2.2)", attr));
  EXPECT_EQ(attr, Attr(1.1, -2.2));

  EXPECT_TRUE(parse(" (2.2 , -3.3) ", attr));
  EXPECT_EQ(attr, Attr(2.2, -3.3));

  EXPECT_FALSE(parse("1.0 a", attr));
}

TEST(boost_spirit_x3, parseEscaped) {
  using Attr = string;

  auto parse = [](string_view in, Attr& attr) {
    struct EscapeChar : x3::symbols<char> {
      EscapeChar() { add("t", '\t')("n", '\n')("\\", '\\'); }
    } escapeChar;

    auto literalChar = !x3::lit('\\') >> x3::char_;
    auto escapeSequence = '\\' >> escapeChar;
    auto grammar = *(literalChar | escapeSequence);

    auto it = in.begin();
    bool parse = x3::phrase_parse(
        it, in.end(),
        grammar,
        x3::eps(false), // Don't skip
        attr);
    return parse && it == in.end();
  };

  {
    Attr attr;
    EXPECT_TRUE(parse("a\\nb\\t", attr));
    EXPECT_EQ(attr, "a\nb\t");
  }

  {
    Attr attr;
    EXPECT_TRUE(parse(" a \\\\ b ", attr));
    EXPECT_EQ(attr, " a \\ b ");
  }

  {
    Attr attr;
    EXPECT_FALSE(parse("abc\\a", attr));
    EXPECT_EQ(attr, "abc");
  }
}

TEST(boost_spirit_x3, parse_char) {
  using Char = char;
  using Attr = basic_string<Char>;

  auto parse = [](basic_string_view<Char> in, Attr& attr) {
    using x3::char_;
    using x3::lit;

    auto grammar = lit('a') >> char_ >> lit("cd");

    auto it = in.begin();
    bool parse = x3::phrase_parse(
        it, in.end(),
        grammar,
        x3::standard::space,
        attr);
    return parse && it == in.end();
  };

  {
    Attr attr;
    EXPECT_TRUE(parse("  a   b  cd  ", attr));
    EXPECT_EQ(attr, "b");
  }
}

TEST(boost_spirit_x3, parse_wchar_t) {
  using Char = wchar_t;
  using Attr = basic_string<Char>;

  auto parse = [](basic_string_view<Char> in, Attr& attr) {
    using x3::standard_wide::char_;
    using x3::standard_wide::lit;

    auto grammar = lit(L'a') >> char_ >> lit(L"cd");

    auto it = in.begin();
    bool parse = x3::phrase_parse(
        it, in.end(),
        grammar,
        x3::standard_wide::space,
        attr);
    return parse && it == in.end();
  };

  {
    Attr attr;
    EXPECT_TRUE(parse(L"  a   b  cd  ", attr));
    EXPECT_EQ(attr, L"b");
  }
}

TEST(boost_spirit_x3, parse_char32_t) {
  using Char = char32_t;
  using Attr = basic_string<Char>;

  auto parse = [](basic_string_view<Char> in, Attr& attr) {
    using x3::unicode::char_;
    using x3::unicode::lit;

    auto grammar = lit(U'a') >> char_ >> lit(U"cd");

    auto it = in.begin();
    bool parse = x3::phrase_parse(
        it, in.end(),
        grammar,
        x3::unicode::space,
        attr);
    return parse && it == in.end();
  };

  {
    Attr attr;
    EXPECT_TRUE(parse(U"  a   b  cd  ", attr));
    EXPECT_EQ(attr, U"b");
  }
}

// EOF
