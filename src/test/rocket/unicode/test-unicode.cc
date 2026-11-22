// test-unicode.cc

#include "rocket-test/rocket-test.h"

#include "rocket/unicode/unicode.h"

using namespace rocket::unicode;

#define MODULE rocketUnicode

// Constants ------------------------------------------------------------------------------------------------

constexpr char TWO_BYTES    = static_cast<char>(0b1101'1111);
constexpr char THREE_BYTES  = static_cast<char>(0b1110'1111);
constexpr char FOUR_BYTES   = static_cast<char>(0b1111'0111);
constexpr char CONT         = static_cast<char>(0b1011'1111);

constexpr char32 D800       = static_cast<char32>(0xD800U);
constexpr char32 MAX_PLUS_1 = static_cast<char32>(0x10FFFFU + 1);

// Tests ====================================================================================================

// `CodePoint` ----------------------------------------------------------------------------------------------

TEST(MODULE, CodePointCtor) {
  EXPECT_THAT(
    [&] { '\x80'_cp; },
    throwsInputFailure(0, EndsWith("invalid ASCII value 0x80")));
  EXPECT_THAT(
    [&] { CodePoint { D800 }; },
    throwsInputFailure(0, EndsWith("invalid code-point value 0xD800")));
  EXPECT_THAT(
    [&] { CodePoint { MAX_PLUS_1 }; },
    throwsInputFailure(0, EndsWith("invalid code-point value 0x110000")));
}

TEST(MODULE, CodePointOpCastString) {
  using type = string;

  EXPECT_EQ(static_cast<type>('\x41'_cp), "A");
  EXPECT_EQ(static_cast<type>(U'\xE4'_cp), "ä");
  EXPECT_EQ(static_cast<type>(U'\u20AC'_cp), "€");
}

TEST(MODULE, CodePointOpCastU32String) {
  using type = u32string;

  EXPECT_EQ(static_cast<type>('\x41'_cp), U"A");
  EXPECT_EQ(static_cast<type>(U'\xE4'_cp), U"ä");
  EXPECT_EQ(static_cast<type>(U'\u20AC'_cp), U"€");
}

TEST(MODULE, CodePointIsPrint) {
  EXPECT_TRUE('a'_cp.isPrint());
  EXPECT_FALSE(U'\uFFF0'_cp.isPrint());
  EXPECT_FALSE(U'\uFFFF'_cp.isPrint());
}

TEST(MODULE, CodePointIsIdentifier) {
  EXPECT_TRUE('a'_cp.isIdStart());
  EXPECT_TRUE(U'ä'_cp.isIdStart());
  EXPECT_FALSE('_'_cp.isIdStart());
  EXPECT_FALSE('0'_cp.isIdStart());
  EXPECT_FALSE('$'_cp.isIdStart());

  EXPECT_TRUE('a'_cp.isIdContinue());
  EXPECT_TRUE('0'_cp.isIdContinue());
  EXPECT_TRUE('_'_cp.isIdContinue());
  EXPECT_TRUE(U'ä'_cp.isIdContinue());
  EXPECT_FALSE('$'_cp.isIdContinue());
  EXPECT_FALSE('='_cp.isIdContinue());
}

TEST(MODULE, CodePointLower) {
  EXPECT_EQ('a'_cp.lower(), U'a');
  EXPECT_EQ(U'Ä'_cp.lower(), U'ä');
  EXPECT_EQ(U'É'_cp.lower(), U'é');
}

TEST(MODULE, CodePointUpper) {
  EXPECT_EQ('A'_cp.upper(), U'A');
  EXPECT_EQ(U'ä'_cp.upper(), U'Ä');
  EXPECT_EQ(U'é'_cp.upper(), U'É');
}

TEST(MODULE, CodePointWidth) {
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

// Functions ------------------------------------------------------------------------------------------------

TEST(MODULE, conversions) {
  EXPECT_EQ(convertUtf8To32("äöü€"), U"äöü€");
  EXPECT_EQ(convertUtf32To8(U"äöü€"), "äöü€");

  const char* str1 = "a€b";

  vector<char32> vec;
  u32string str = convertUtf8To32(str1);
  ranges::copy(str, back_inserter(vec));
  EXPECT_EQ(vec, (vector<char32> { 97, 0x20ac, 98 }));

  const u32string str2 = convertUtf8To32(str1);
  EXPECT_EQ(str2, U"a€b");
  EXPECT_EQ(str2.size(), 3);

  const string str3 = convertUtf32To8(str2);
  EXPECT_EQ(str3, str1);

  // U+1F9D1 (ADULT), U+200D (ZERO WIDTH JOINER), U+1F33E (EAR OF RICE)
  EXPECT_EQ(convertUtf8To32("a🧑‍🌾b"), U"a🧑‍🌾b");

  auto str32 = convertUtf8To32("🧑‍🌾");
  ASSERT_EQ(str32.size(), 3);
  EXPECT_EQ(str32[0], 0x1F9D1);
  EXPECT_EQ(str32[1], 0x200D);
  EXPECT_EQ(str32[2], 0x1F33E);

  // Test invalid UTF-8

  string str4 = "abc\xC8"; // Invalid ASCII: 200
  EXPECT_THAT(
    [&] { convertUtf8To32(str4); },
    throwsInputFailure(3, EndsWith("invalid UTF-8 byte sequence")));
  EXPECT_EQ(convertUtf8To32(str4, unicode::Continue), U"abc�");

  const string str5 { 'a', CONT, 'b' };
  EXPECT_THAT(
    [&] { convertUtf8To32(str5); },
    throwsInputFailure(1, EndsWith("invalid UTF-8 byte sequence")));
  EXPECT_EQ(convertUtf8To32(str5, unicode::Continue), U"a�b");

  // Test invalid UTF-32

  u32string str6(U"abc");
  str6[1] = static_cast<char32>(0xD800);
  EXPECT_THAT(
    [&] { convertUtf32To8(str6); },
    throwsInputFailure(1, EndsWith("invalid code-point value 0xD800")));
  EXPECT_EQ(convertUtf32To8(str6, unicode::Continue), "a�c");
}

// `rocket::unicode::utf8` ----------------------------------------------------------------------------------

TEST(MODULE, utf8Cut) {
  // No cut needed
  EXPECT_EQ(utf8::cut("", 0), "");
  EXPECT_EQ(utf8::cut("", 5), "");
  EXPECT_EQ(utf8::cut("abc", 3), "abc");
  EXPECT_EQ(utf8::cut("abc", 10), "abc");
  EXPECT_EQ(utf8::cut("ä€😀", 9), "ä€😀");

  // ASCII
  EXPECT_EQ(utf8::cut("abc", 0), "");
  EXPECT_EQ(utf8::cut("abc", 2), "ab");

  // Two-byte sequence `ä`
  EXPECT_EQ(utf8::cut("aä", 1), "a");
  EXPECT_EQ(utf8::cut("aä", 2), "a"); // Would split `ä`

  // Three-byte sequence `€`
  EXPECT_EQ(utf8::cut("a€b", 2), "a");
  EXPECT_EQ(utf8::cut("a€b", 3), "a");
  EXPECT_EQ(utf8::cut("a€b", 4), "a€");

  // Four-byte sequence `😀`
  for (u64 i = 0; i < 4; ++i) {
    EXPECT_EQ(utf8::cut("😀b", i), "") << "i = " << i;
  }
  EXPECT_EQ(utf8::cut("😀b", 4), "😀");

  // Mixed: `ä` (2), `€` (3), `😀` (4)
  EXPECT_EQ(utf8::cut("ä€😀", 4), "ä");
  EXPECT_EQ(utf8::cut("ä€😀", 5), "ä€");
  EXPECT_EQ(utf8::cut("ä€😀", 8), "ä€");

  // The result is a view to the input
  const string str = "ä€😀";
  EXPECT_EQ(utf8::cut(str, 5).data(), str.data());

  // Only trail bytes: backs off to the start
  const string conts { CONT, CONT, CONT };
  EXPECT_EQ(utf8::cut(conts, 2), "");
}

TEST(MODULE, utf8Validate) {
  Positions pos;

  {
    auto cow = utf8::validate("äöüß€", InvalidUnicodePolicy::Throw, &pos);
    EXPECT_FALSE(cow.modified());
    EXPECT_EQ(cow.get(), "äöüß€");
    EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 2, 2 }, { 4, 4 }, { 6, 6 }, { 8, 8 }, { 11, 11 }}));
  }

  static_assert("�"sv.size() == 3);

  {
    const string str { 'a', CONT, 'b' };
    auto cow = utf8::validate(str, InvalidUnicodePolicy::Continue, &pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), "a�b");
    EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 1, 1 }, { 2, 4 }, { 3, 5 } }));
  }

  {
    const string str { 'a', TWO_BYTES, 'b', 'c' };
    auto cow = utf8::validate(str, InvalidUnicodePolicy::Continue, &pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), "a�bc");
    EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 1, 1 }, { 2, 4 }, { 3, 5 }, { 4, 6 } }));
  }

  {
    const string str { 'a', THREE_BYTES, CONT };
    auto cow = utf8::validate(str, InvalidUnicodePolicy::Continue, &pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), "a�");
    EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 1, 1 }, { 3, 4 } }));
  }

  {
    const string str { 'a', FOUR_BYTES, CONT, 'b' };
    auto cow = utf8::validate(str, InvalidUnicodePolicy::Continue, &pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), "a��b");
    EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 1, 1 }, { 2, 4 }, { 3, 7 }, { 4, 8 } }));
  }
}

// `rocket::unicode::utf32` ---------------------------------------------------------------------------------

TEST(MODULE, utf32Validate) {
  Positions pos;

  {
    const u32string_view sv = U"abc";
    auto cow = utf32::validate(sv, InvalidUnicodePolicy::Throw, &pos);
    EXPECT_FALSE(cow.modified());
    EXPECT_EQ(cow.get(), U"abc");
    EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 } }));
  }

  {
    const u32string str { 'a', D800, 'b', MAX_PLUS_1 };
    const u32string_view sv = str;
    auto cow = utf32::validate(sv, InvalidUnicodePolicy::Continue,&pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), U"a�b�");
    EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 }, { 4, 4 } }));
  }
}

// EOF
