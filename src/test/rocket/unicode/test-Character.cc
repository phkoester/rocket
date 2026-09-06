/*
 * test-Character.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/system/terminal/terminal.h"
#include "rocket/unicode/Character.h"
#include "rocket/unicode/ConvertTo.h"

using namespace rocket::unicode;

namespace {

// Local functions ------------------------------------------------------------------------------------------

template<typename C> requires IsChar<C>
void
testCharacter(const CharacterView<C>& c) {
  auto& out = nio::out;

  if (not TEST_TERMINAL) {
    static bool info = false;
    if (not info) {
      ROCKET_PROCESS_INFO("Not testing character because `" ROCKET_TEST_TERMINAL "` is not set");
      info = true;
    }
    return;
  }

  out.print("[{}]", ConvertTo<char>::apply(static_cast<basic_string_view<C>>(c)));
  auto pos = system::terminal::position(out);
  EXPECT_TRUE(pos);
  EXPECT_EQ(pos->first, c.width() + 3); // Check terminal's cursor position
  out.write('\n');
  out.println("[{:~<{}}]", "", c.width());
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(Character, charWidth) {
  EXPECT_EQ("\t"_c.width(), 0);
  EXPECT_EQ("\u200D"_c.width(), 0); // u+0200D (ZERO WIDTH JOINER)

  {
    // ề: LATIN SMALL LETTER E, COMBINING CIRCUMFLEX ACCENT, COMBINING GRAVE ACCENT
    auto c = "\u0065\u0302\u0300"_c;
    EXPECT_EQ(c.width(), 1);
    testCharacter<char>(CharacterView<char>(c));
  }

  {
    // U+1F9D1 (ADULT), U+200D (ZERO WIDTH JOINER), U+1F33E (EAR OF RICE)
    static_assert("🧑‍🌾"sv == "🧑\u200D🌾"sv);
    static_assert("🧑‍🌾"sv == "\U0001F9D1\u200D\U0001F33E"sv);
    auto c = "🧑‍🌾"_c;
    EXPECT_EQ(c.width(), 2);
    testCharacter<char>(CharacterView<char>(c));
  }

  {
    // U+1F468 (MAN), U+200D (ZERO WIDTH JOINER), U+1F469 (WOMAN), U+200D (ZERO WIDTH JOINER), U+1F466 (BOY)
    static_assert("👨‍👩‍👦"sv == "👨\u200D👩\u200D👦"sv);
    static_assert("👨‍👩‍👦"sv == "\U0001F468\u200D\U0001F469\u200D\U0001F466"sv);
    auto c = "👨‍👩‍👦"_c;
    EXPECT_EQ(c.countCodePoints(), 5);
    EXPECT_EQ(c.width(), 2);
    testCharacter<char>(CharacterView<char>(c));
  }
}

TEST(Character, char32Width) {
  {
    // U+1F468 (MAN), U+200D (ZERO WIDTH JOINER), U+1F469 (WOMAN), U+200D (ZERO WIDTH JOINER), U+1F466 (BOY)
    static_assert(U"👨‍👩‍👦"sv == U"👨\u200D👩\u200D👦"sv);
    static_assert(U"👨‍👩‍👦"sv == U"\U0001F468\u200D\U0001F469\u200D\U0001F466"sv);
    auto c = U"👨‍👩‍👦"_c;
    EXPECT_EQ(c.countCodePoints(), 5);
    EXPECT_EQ(c.width(), 2);
    testCharacter<char32>(CharacterView<char32>(c));
  }
}

// EOF
