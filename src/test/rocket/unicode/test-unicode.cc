/*
 * test-unicode.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/format/std.h"
#include "rocket/unicode/unicode.h"

using namespace rocket;
using namespace rocket::unicode;
using namespace std;
using namespace testing;

// Constants ------------------------------------------------------------------------------------------------

constexpr char TWO_BYTES    = 0b1101'1111;
constexpr char THREE_BYTES  = 0b1110'1111;
constexpr char FOUR_BYTES   = 0b1111'0111;
constexpr char CONT         = 0b1011'1111;

constexpr char32_t D800       = static_cast<char32_t>(0xD800U);
constexpr char32_t MAX_PLUS_1 = static_cast<char32_t>(0x10FFFFU + 1);

// Functions ------------------------------------------------------------------------------------------------

auto
positions(initializer_list<pair<size_t, size_t>> list) {
  return makeUnorderedBimap(list);
}

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(unicode, CodePointCtorChar) {
  EXPECT_EQ(static_cast<uint32_t>('\x7f'_cp), 127);

  EXPECT_THAT(
      [&] { '\x80'_cp; },
      ThrowsMessage<InvalidArgument>(HasSubstr("Parameter `v`: ")));
}

TEST(unicode, CodePointOpCasString) {
  using type = string;

  EXPECT_EQ(static_cast<type>('\x41'_cp), "A");
  EXPECT_EQ(static_cast<type>(U'\xE4'_cp), "ä");
  EXPECT_EQ(static_cast<type>(U'\x20AC'_cp), "€");
}

TEST(unicode, CodePointOpCastU32String) {
  using type = u32string;

  EXPECT_EQ(static_cast<type>('\x41'_cp), U"A");
  EXPECT_EQ(static_cast<type>(U'\xE4'_cp), U"ä");
  EXPECT_EQ(static_cast<type>(U'\x20AC'_cp), U"€");
}

TEST(unicode, CodePointIsPrint) {
  EXPECT_TRUE('a'_cp.isPrint());
  EXPECT_FALSE(U'\uFFF0'_cp.isPrint());
  EXPECT_FALSE(U'\uFFFF'_cp.isPrint());
}

TEST(unicode, CodePointLower) {
  EXPECT_EQ('a'_cp.lower(), U'a');
  EXPECT_EQ(U'Ä'_cp.lower(), U'ä');
  EXPECT_EQ(U'É'_cp.lower(), U'é');
}

TEST(unicode, CodePointUpper) {
  EXPECT_EQ('A'_cp.upper(), U'A');
  EXPECT_EQ(U'ä'_cp.upper(), U'Ä');
  EXPECT_EQ(U'é'_cp.upper(), U'É');
}

TEST(unicode, CodePointWidth) {
  EXPECT_EQ(U'\u0000'_cp.width(), 0); // NULL (0)
  EXPECT_EQ(U'\u0001'_cp.width(), 0); // START OF HEADING (1)
  EXPECT_EQ(U'\u001F'_cp.width(), 0); // INFORMATION SEPARATOR ONE (31)
  EXPECT_EQ(U'\u0020'_cp.width(), 1); // SPACE (32)
  EXPECT_EQ(U'\u007E'_cp.width(), 1); // TILDE (126)
  EXPECT_EQ(U'\u007F'_cp.width(), 0); // DELETE (127)
  EXPECT_EQ(U'\u0080'_cp.width(), 0); // PADDING CHARACTER (128)
  EXPECT_EQ(U'\u009F'_cp.width(), 0); // APPLICATION PROGRAM COMMAND (159)
  EXPECT_EQ(U'\u00A0'_cp.width(), 1); // NO-BREAK SPACE (160)
  EXPECT_EQ(U'\u00AD'_cp.width(), 0); // SOFT HYPHEN (173)
  EXPECT_EQ(U'\u0300'_cp.width(), 0); // COMBINING GRAVE ACCENT, Category Mn (768)
}

TEST(unicode, CodePointFormat) {
  EXPECT_EQ(fmt::format("{}", '\0'_cp), "U+0000");
  EXPECT_EQ(fmt::format("{}", U'\u20AC'_cp), "U+20AC");
  EXPECT_EQ(fmt::format("{}", U'\u00FF'_cp), "U+00FF");
  EXPECT_EQ(fmt::format("{}", U'\U0001ABCD'_cp), "U+1ABCD");
  EXPECT_EQ(fmt::format("{}", U'\U0010FFFF'_cp), "U+10FFFF");

  EXPECT_EQ(fmt::format(U"{}", U'\u20AC'_cp), U"U+20AC");
}

TEST(unicode, CodePointVectorFormat) {
  auto cps = vector<CodePoint> { 'a'_cp, 'b'_cp, 'c'_cp };
  EXPECT_EQ(fmt::format("{::}", cps), "[U+0061, U+0062, U+0063]");
  EXPECT_EQ(fmt::format("{::~>8}", cps), "[~~U+0061, ~~U+0062, ~~U+0063]");
  EXPECT_EQ(fmt::format("{:n:~>8}", cps), "~~U+0061, ~~U+0062, ~~U+0063");
}

TEST(unicode, conversions) {
  EXPECT_EQ(utf8To32("äöü€"), U"äöü€");
  EXPECT_EQ(utf32To8(U"äöü€"), "äöü€");

  const char* s1 = "a€b";

  vector<char32_t> v;
  u32string s = utf8To32(s1);
  copy(s.begin(), s.end(), back_inserter(v));
  EXPECT_EQ(v, (vector<char32_t>{ 97, 0x20ac, 98 }));

  u32string s2 = utf8To32(s1);
  EXPECT_EQ(s2, U"a€b");
  EXPECT_EQ(s2.size(), 3);

  string s3 = utf32To8(s2);
  EXPECT_EQ(s3, s1);

  // U+1F9D1 (ADULT), U+200D (ZERO WIDTH JOINER), U+1F33E (EAR OF RICE)
  EXPECT_EQ(utf8To32("a🧑‍🌾b"), U"a🧑‍🌾b");

  auto s32 = utf8To32("🧑‍🌾");
  ASSERT_EQ(s32.size(), 3);
  EXPECT_EQ(s32[0], 0x1F9D1);
  EXPECT_EQ(s32[1], 0x200D);
  EXPECT_EQ(s32[2], 0x1F33E);
}

// `rocket::unicode::utf8` ..................................................................................

TEST(unicode, utf8Validate) {
  UnorderedBimap<size_t, size_t> pos;

  {
    auto cow = utf8::validate("äöüß€", &pos);
    EXPECT_FALSE(cow.modified());
    EXPECT_EQ(cow.get(), "äöüß€");
    EXPECT_EQ(pos, positions({ { 0, 0 }, { 2, 2 }, { 4, 4 }, { 6, 6 }, { 8, 8 }, { 11, 11 }}));
  }

  static_assert("�"sv.size() == 3);

  {
    string s = { 'a', CONT, 'b' };
    auto cow = utf8::validate(s, &pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), "a�b");
    EXPECT_EQ(pos, positions({ { 0, 0 }, { 1, 1 }, { 2, 4 }, { 3, 5 } }));
  }

  {
    string s = { 'a', TWO_BYTES, 'b', 'c' };
    auto cow = utf8::validate(s, &pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), "a�bc");
    EXPECT_EQ(pos, positions({ { 0, 0 }, { 1, 1 }, { 2, 4 }, { 3, 5 }, { 4, 6 } }));
  }

  {
    string s = { 'a', THREE_BYTES, CONT };
    auto cow = utf8::validate(s, &pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), "a�");
    EXPECT_EQ(pos, positions({ { 0, 0 }, { 1, 1 }, { 3, 4 } }));
  }

  {
    string s = { 'a', FOUR_BYTES, CONT, 'b' };
    auto cow = utf8::validate(s, &pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), "a��b");
    EXPECT_EQ(pos, positions({ { 0, 0 }, { 1, 1 }, { 2, 4 }, { 3, 7 }, { 4, 8 } }));
  }
}

// `rocket::unicode::utf32` ---------------------------------------------------------------------------------

TEST(unicode, utf32Validate) {
  UnorderedBimap<size_t, size_t> pos;

  {
    u32string_view sv = U"abc";
    auto cow = utf32::validate(sv, &pos);
    EXPECT_FALSE(cow.modified());
    EXPECT_EQ(cow.get(), U"abc");
    EXPECT_EQ(pos, positions({ { 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 } }));
  }

  {
    u32string s = { 'a', D800, 'b', MAX_PLUS_1 };
    u32string_view sv = s;
    auto cow = utf32::validate(sv, &pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), U"a�b�");
    EXPECT_EQ(pos, positions({ { 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 }, { 4, 4 } }));
  }
}

// EOF
