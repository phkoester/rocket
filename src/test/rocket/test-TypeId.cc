/*
 * test-TypeId.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/TypeId.h"

#include <unordered_set>

namespace test_typeid {

// Local types ----------------------------------------------------------------------------------------------

struct Plain {};

struct Base {
  Base() = default;
  Base(const Base&) = default;
  Base(Base&&) = default;
  Base& operator=(const Base&) = default;
  Base& operator=(Base&&) = default;
  virtual ~Base() = default;
};

struct Derived : Base {};

template<typename T>
struct Box {};

} // namespace test_typeid

using namespace test_typeid;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(TypeId, of) {
  EXPECT_EQ(TypeId::of<i32>(), TypeId(type_identity<i32>()));
  EXPECT_EQ(TypeId::of<i32>(), TypeId(typeid(i32)));
  EXPECT_EQ(TypeId::of<Plain>(), TypeId(typeid(Plain)));
  EXPECT_NE(TypeId::of<i32>(), TypeId::of<i64>());
  EXPECT_NE(TypeId::of<Base>(), TypeId::of<Derived>());
}

TEST(TypeId, ofIgnoresCvAndReferences) {
  // Like `typeid`, top-level cv-qualifiers and references are ignored
  EXPECT_EQ(TypeId::of<const i32>(), TypeId::of<i32>());
  EXPECT_EQ(TypeId::of<volatile i32>(), TypeId::of<i32>());
  EXPECT_EQ(TypeId::of<i32&>(), TypeId::of<i32>());
  EXPECT_EQ(TypeId::of<const i32&>(), TypeId::of<i32>());
  EXPECT_EQ(TypeId::of<i32&&>(), TypeId::of<i32>());

  // Pointers are distinct types
  EXPECT_NE(TypeId::of<i32*>(), TypeId::of<i32>());
  EXPECT_NE(TypeId::of<const i32*>(), TypeId::of<i32*>());
}

TEST(TypeId, ctorObject) {
  EXPECT_EQ(TypeId(42), TypeId::of<int>());
  EXPECT_EQ(TypeId(string("hi")), TypeId::of<string>());

  const Plain plain;
  EXPECT_EQ(TypeId(plain), TypeId::of<Plain>());
}

TEST(TypeId, ctorObjectPolymorphic) {
  const Derived derived;
  const Base& base = derived;
  EXPECT_EQ(TypeId(base), TypeId::of<Derived>());
  EXPECT_NE(TypeId(base), TypeId::of<Base>());

  const Base base2;
  EXPECT_EQ(TypeId(base2), TypeId::of<Base>());
}

TEST(TypeId, ctorObjectPointer) {
  // A pointer is not polymorphic, so its static type is used
  const Derived derived;
  const Base* ptr = &derived;
  EXPECT_EQ(TypeId(ptr), TypeId::of<const Base*>());
}

TEST(TypeId, copy) {
  const TypeId a = TypeId::of<Plain>();
  const TypeId b(a);
  EXPECT_EQ(a, b);
  EXPECT_EQ(b, TypeId::of<Plain>());
}

TEST(TypeId, compare) {
  const TypeId a = TypeId::of<i32>();
  const TypeId b = TypeId::of<Plain>();

  EXPECT_EQ(a <=> a, strong_ordering::equal);
  EXPECT_TRUE(a == a);
  EXPECT_FALSE(a != a);
  EXPECT_TRUE(a <= a);
  EXPECT_TRUE(a >= a);
  EXPECT_FALSE(a < a);
  EXPECT_FALSE(a > a);

  // The order is implementation-defined, but must be strict and consistent
  EXPECT_NE(a <=> b, strong_ordering::equal);
  EXPECT_TRUE(a != b);
  EXPECT_NE(a < b, b < a);
  EXPECT_EQ(a < b, b > a);
  EXPECT_EQ(a <=> b, 0 <=> (b <=> a));
}

TEST(TypeId, hash) {
  EXPECT_EQ(TypeId::of<i32>().hash(), TypeId::of<const i32&>().hash());
  EXPECT_EQ(TypeId::of<i32>().hash(), typeid(i32).hash_code());
  EXPECT_EQ(TypeId::of<i32>().hash(), std::hash<TypeId>()(TypeId::of<i32>()));
  EXPECT_EQ(TypeId::of<Plain>().hash(), std::hash<TypeId>()(TypeId::of<Plain>()));
}

TEST(TypeId, unorderedSet) {
  unordered_set<TypeId> set;
  EXPECT_TRUE(set.insert(TypeId::of<i32>()).second);
  EXPECT_TRUE(set.insert(TypeId::of<Plain>()).second);
  EXPECT_FALSE(set.insert(TypeId(42)).second);
  EXPECT_EQ(set.size(), 2U);
  EXPECT_TRUE(set.contains(TypeId::of<const i32>()));
  EXPECT_FALSE(set.contains(TypeId::of<i64>()));
}

TEST(TypeId, name) {
  EXPECT_STREQ(TypeId::of<i32>().name(), typeid(i32).name());
  EXPECT_STREQ(TypeId::of<Plain>().name(), typeid(Plain).name());
}

TEST(TypeId, prettyName) {
  EXPECT_EQ(TypeId::of<int>().prettyName(), "int");
  EXPECT_EQ(TypeId::of<double>().prettyName(), "double");
  EXPECT_EQ(TypeId::of<int*>().prettyName(), "int*");
  EXPECT_EQ(TypeId::of<const int*>().prettyName(), "int const*");
  EXPECT_EQ(TypeId::of<TypeId>().prettyName(), "rocket::TypeId");
  EXPECT_EQ(TypeId::of<Plain>().prettyName(), "test_typeid::Plain");
  EXPECT_EQ(TypeId::of<Box<int>>().prettyName(), "test_typeid::Box<int>");
  EXPECT_EQ(TypeId::of<Box<Plain>>().prettyName(), "test_typeid::Box<test_typeid::Plain>");

  const Derived derived;
  const Base& base = derived;
  EXPECT_EQ(TypeId(base).prettyName(), "test_typeid::Derived");
}

TEST(TypeId, format) {
  EXPECT_EQ(fmt::format("{}", TypeId::of<int>()), "`int`");
  EXPECT_EQ(fmt::format("{}", TypeId::of<Plain>()), "`test_typeid::Plain`");
  EXPECT_EQ(fmt::format("{}", TypeId::of<Box<int>>()), "`test_typeid::Box<int>`");

  const Derived derived;
  const Base& base = derived;
  EXPECT_EQ(fmt::format("{}", TypeId(base)), "`test_typeid::Derived`");
}

TEST(TypeId, formatSpec) {
  // Format specifiers apply to the name including the backticks
  EXPECT_EQ(fmt::format("{:>7}", TypeId::of<int>()), "  `int`");
  EXPECT_EQ(fmt::format("{:<7}|", TypeId::of<int>()), "`int`  |");
  EXPECT_EQ(fmt::format("{:*^9}", TypeId::of<int>()), "**`int`**");
  EXPECT_EQ(fmt::format("{:.3}", TypeId::of<int>()), "`in");
  EXPECT_EQ(fmt::format("{:?}", TypeId::of<int>()), "\"`int`\"");
}

TEST(TypeId, formatChar32) {
  EXPECT_EQ(fmt::format(U"{}", TypeId::of<int>()), U"`int`");
  EXPECT_EQ(fmt::format(U"{}", TypeId::of<Plain>()), U"`test_typeid::Plain`");
  EXPECT_EQ(fmt::format(U"{:>7}", TypeId::of<int>()), U"  `int`");
}

// EOF
