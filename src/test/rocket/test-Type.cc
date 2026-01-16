/*
 * test-Type.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/Type.h"

#include <map>
#include <tuple>
#include <variant>
#include <vector>

// `Enum1` --------------------------------------------------------------------------------------------------

enum Enum1 { One, Two, Three };

// `Enum2` --------------------------------------------------------------------------------------------------

namespace {

enum Enum2 { Four, Five, Six };

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(Type, opEq) {
  EXPECT_EQ(Type::of<i32>(), Type::of<i32>());
}

TEST(Type, opNe) {
  EXPECT_NE(Type::of<i32>(), Type::of<i64>());
}

TEST(Type, format) {
  EXPECT_EQ(fmt::format("{}", Type::of<i32>()), "int");
  EXPECT_EQ(fmt::format("{}", Type::of<Type>()), "rocket::Type");
  EXPECT_EQ(fmt::format("{:?}", Type::of<Type>()), "\"rocket::Type\"");
  EXPECT_EQ(fmt::format(U"{:?}", Type::of<Type>()), U"\"rocket::Type\"");
}

/**
 * Test passes if it compiles.
 */
TEST(Type, map) {
  map<Type, string> map;
  map.emplace(Type::of<i32>(), "i32");
  map.emplace(Type::of<string>(), "string");
}

TEST(Type, name) {
  Enum1 e1 = One;
  EXPECT_EQ(Type::of(e1).name(), "Enum1");
  EXPECT_EQ(Type::of<Enum1>().name(), "Enum1");

  Enum2 e2 = Four;
  EXPECT_EQ(Type::of(e2).name(), "(anonymous namespace)::Enum2");
  EXPECT_EQ(Type::of<Enum2>().name(), "(anonymous namespace)::Enum2");

  using type = variant<i32, tuple<string, vector<u128>>>;
  EXPECT_EQ(
      Type::of<type>().name(),
      "std::variant<int, std::tuple<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>, std::vector<unsigned __int128, std::allocator<unsigned __int128>>>>");
}

/**
 * Test passes if it compiles.
 */
TEST(Type, unorderedMap) {
  unordered_map<Type, string> map;
  map.emplace(Type::of<i32>(), "i32");
  map.emplace(Type::of<string>(), "string");
}

// EOF
