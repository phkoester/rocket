/*
 * test-enum.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/enum.h"

#include <fmt/xchar.h>

#include <scn/istream.h>

// `MyEnum` -------------------------------------------------------------------------------------------------

enum MyEnum : u8 { fröb, fröber, fröberer, pörk, pörker, pörkerer };

ROCKET_ENUM_DECLARE(, MyEnum, MyEnum); // NOLINT(*-internal-linkage)
ROCKET_ENUM_DEFINE(, MyEnum, MyEnum, (fröb)(fröber)(fröberer)(pörk)(pörker)(pörkerer));

// `MyEnumClass` --------------------------------------------------------------------------------------------

enum class MyEnumClass : u8 { hürx, hürxer, hürxerer };

ROCKET_ENUM_DECLARE(, MyEnumClass, MyEnumClass); // NOLINT(*-internal-linkage)
ROCKET_ENUM_DEFINE(, MyEnumClass, MyEnumClass, (hürx)(hürxer)(hürxerer));

// `MyEnumInNamespace` --------------------------------------------------------------------------------------

namespace mynamespace {

enum MyEnumInNamespace : u8 { red, green, blue };

} // namespace my_namespace

ROCKET_ENUM_DECLARE(mynamespace, MyEnumInNamespace, MyEnumInNamespace); // NOLINT(*-internal-linkage)
ROCKET_ENUM_DEFINE(mynamespace, MyEnumInNamespace, MyEnumInNamespace, (red)(green)(blue));

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(enum, MyEnumOpOutput) {
  ostringstream os;
  os << fröb;
  EXPECT_EQ(os.str(), "fröb");
}

TEST(enum, MyEnumFormat) {
  EXPECT_EQ(fmt::format("{}", fröber), "fröber");
  EXPECT_EQ(fmt::format("{}", fröberer), "fröberer");
  EXPECT_EQ(fmt::format("{}", pörk), "pörk");
  EXPECT_EQ(fmt::format("{}", pörker), "pörker");
  EXPECT_EQ(fmt::format("{}", pörkerer), "pörkerer");
  EXPECT_EQ(fmt::format("{: >10}", fröber), "    fröber"); // Tests UTF-8 alignment; 4 spaces expected

  EXPECT_THAT(
    [] { static_cast<void>(fmt::format("{}", static_cast<MyEnum>(10))); }, // NOLINT
    ThrowsMessage<InvalidState>(HasSubstr("Invalid `MyEnum` value 10")));

  EXPECT_EQ(fmt::format(U"{}", fröb), U"fröb");
}

TEST(enum, MyEnumScan) {
  {
    const auto result = scn::scan<MyEnum>("", "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(string_view(result.error().msg()), "EOF"sv);
  }

  {
    const auto result = scn::scan<MyEnum>("frö", "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(string_view(result.error().msg()), "Invalid enum value"sv);
  }

  {
    const auto result = scn::scan<MyEnum>("fröb", "{}");
    ASSERT_TRUE(result);
    const auto val = result->value();
    EXPECT_EQ(val, fröb);
  }

  {
    const auto result = scn::scan<MyEnum>("fröbZZZ", "{}");
    ASSERT_TRUE(result);
    const auto val = result->value();
    EXPECT_EQ(val, fröb);
    EXPECT_EQ(string_view(result->begin()), "ZZZ"sv);
  }

  {
    const auto result = scn::scan<MyEnum>("  fröberer  xx", "{}");
    ASSERT_TRUE(result);
    const auto val = result->value();
    EXPECT_EQ(val, fröberer);
    EXPECT_EQ(string_view(result->begin()), "  xx"sv);
  }

  {
    const string input = "fröb, fröberer...";
    auto is = istringstream(input);
    const auto result = scn::scan<MyEnum, MyEnum>(is, "{}, {}");
    ASSERT_TRUE(result);
    const auto [val1, val2] = result->values();
    EXPECT_EQ(val1, fröb);
    EXPECT_EQ(val2, fröberer);
    EXPECT_EQ(io::tellg(is), 16);
  }
}

TEST(enum, MyEnumToType) {
  EXPECT_EQ(Enum<MyEnum>::toType("fröb", true), make_pair(5_u64, fröb ));
  EXPECT_EQ(Enum<MyEnum>::toType("fröbx", false), make_pair(5_u64, fröb ));
  EXPECT_EQ(Enum<MyEnum>::toType("fröber", true), make_pair(7_u64, fröber));
  EXPECT_EQ(Enum<MyEnum>::toType("fröberer", true), make_pair(9_u64, fröberer));
  EXPECT_EQ(Enum<MyEnum>::toType("pörk", true), make_pair(5_u64, pörk));
  EXPECT_EQ(Enum<MyEnum>::toType("pörker", true), make_pair(7_u64, pörker));
  EXPECT_EQ(Enum<MyEnum>::toType("pörkerer", true), make_pair(9_u64, pörkerer));

  EXPECT_THAT(
    [] { Enum<MyEnum>::toType("foo", true); },
    ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"foo\" as `MyEnum`"))
  );

  EXPECT_THAT(
    [] { Enum<MyEnum>::toType("fröbx", true); },
    ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"fröbx\" as `MyEnum`"))
  );
}

TEST(enum, MyEnumClassOpOutput) {
  ostringstream os;
  os << MyEnumClass::hürx;
  EXPECT_EQ(os.str(), "hürx");
}

TEST(enum, MyEnumFClassFormat) {
  EXPECT_EQ(fmt::format("{}", MyEnumClass::hürx), "hürx");
}

TEST(enum, MyEnumClassToType) {
  EXPECT_EQ(Enum<MyEnumClass>::toType("hürxer", true), make_pair(7_u64, MyEnumClass::hürxer));

  EXPECT_THAT(
    [] { Enum<MyEnumClass>::toType("foo", true); },
    ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"foo\" as `MyEnumClass`")));
  EXPECT_THAT(
    [] { Enum<MyEnumClass>::toType("hürxerx", true); },
    ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"hürxerx\" as `MyEnumClass`")));
}

TEST(enum, MyEnumInNamespaceOpOutput) {
  ostringstream os;
  os << mynamespace::red;
  EXPECT_EQ(os.str(), "red");
}

// EOF
