/*
 * test-Type.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/Type.h"
#include "rocket/base.h"

#include <map>
#include <tuple>
#include <variant>
#include <vector>

using namespace rocket;
using namespace std;

// `Enum1` --------------------------------------------------------------------------------------------------

enum Enum1 { A, B, C };

// `Enum2` --------------------------------------------------------------------------------------------------

namespace {

enum Enum2 { D, E, F };

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(Type, eq) {
  EXPECT_EQ(Type::of<int>(), Type::of<long>());
}

TEST(Type, format) {
  EXPECT_EQ(fmt::format("{}", Type::of<int>()), "int");
  EXPECT_EQ(fmt::format("{}", Type::of<Type>()), "rocket::Type");
}

TEST(Type, name) {
  Enum1 e1 = A;
  EXPECT_EQ(Type::of(e1).name(), "Enum1");
  EXPECT_EQ(Type::of<Enum1>().name(), "Enum1");

  Enum2 e2 = D;
  EXPECT_EQ(Type::of(e2).name(), "(anonymous namespace)::Enum2");
  EXPECT_EQ(Type::of<Enum2>().name(), "(anonymous namespace)::Enum2");

  using type = variant<int, tuple<string, vector<uint128_t>>>;
  EXPECT_EQ(
      Type::of<type>().name(),
      "std::variant<int, std::tuple<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>, std::vector<unsigned __int128, std::allocator<unsigned __int128>>>>");
}

/**
 * Test passes if it compiles.
 */
TEST(Type, map) {
  map<Type, string> map;
  map.emplace(Type::of<int>(), "int");
  map.emplace(Type::of<string>(), "string");
}

/**
 * Test passes if it compiles.
 */
TEST(Type, unorderedMap) {
  unordered_map<Type, string> map;
  map.emplace(Type::of<int>(), "int");
  map.emplace(Type::of<string>(), "string");
}

// EOF
