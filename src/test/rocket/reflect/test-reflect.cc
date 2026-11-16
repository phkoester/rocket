/*
 * test-reflect.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/reflect/reflect.h"

using namespace rocket::reflect;

#define MODULE rocketReflect

// `MyStruct` -----------------------------------------------------------------------------------------------

struct MyStruct {
  i32 ä = 0;
  string b;

  MyStruct() = default;

  MyStruct(i32 ä, string_view b, bool c) : ä(ä), b(b), c(c) {}

  [[nodiscard]] bool getC() const { return c; }

private:

  bool c = false;

public:

  ROCKET_REFLECT_MEMBERS(MyStruct, Index, (ä)(b)(c));

  ROCKET_REFLECT_MEMBERS(MyStruct, Public, (ä)(b));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyStruct, Index); // NOLINT(*-internal-linkage)
ROCKET_REFLECT_MEMBERS_DEFINE(, MyStruct, Index);

// `MyDerivedStruct` ----------------------------------------------------------------------------------------

struct MyDerivedStruct : MyStruct {
  string d;

  MyDerivedStruct(i32 ä, string_view b, bool c, string_view d, f32 e) : MyStruct(ä, b, c), d(d), e(e) {}

private:

  f32 e = 0;

public:

  ROCKET_REFLECT_MEMBERS_DERIVED(MyStruct, Index, MyDerivedStruct, Index, (d)(e));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyDerivedStruct, Index); // NOLINT(*-internal-linkage)
ROCKET_REFLECT_MEMBERS_DEFINE(, MyDerivedStruct, Index);

// Tests ====================================================================================================

TEST(MODULE, MyStruct) {
  static_assert(same_as<MyStruct::Index::Ordering, strong_ordering>);
  static_assert(same_as<MyStruct::Public::Ordering, strong_ordering>);

  MyStruct m1(12, "here", true);
  EXPECT_EQ(m1.b, "here");
  get<1>(MyStruct::Index::refs).get(m1) = "everywhere";
  EXPECT_EQ(m1.b, "everywhere");
  EXPECT_EQ(m1.getC(), true);
  get<2>(MyStruct::Index::refs).get(m1) = false;
  EXPECT_EQ(m1.getC(), false);

  const MyStruct m2(13, "there", true);
  EXPECT_EQ(get<1>(MyStruct::Index::refs).get(m2), "there");
  EXPECT_EQ(get<2>(MyStruct::Index::refs).get(m2), true);
}

/**
 * Tests that the macro #ROCKET_REFLECT_MEMBERS does not affect the size of a class.
 */
TEST(MODULE, MyStructSizeof) {
  struct BareStruct {
    i32 ä;
    string b;
    bool c;
  };
  EXPECT_EQ(sizeof(BareStruct), sizeof(MyStruct));
}

TEST(MODULE, MyStructOpEq) {
  EXPECT_EQ(MyStruct(42, "rocket", true), MyStruct(42, "rocket", true));
}

TEST(MODULE, MyStructOpNe) {
  EXPECT_NE(MyStruct(42, "rocket", true), MyStruct(42, "rocket", false));
}

TEST(MODULE, MyStructOpLt) {
  EXPECT_THAT(MyStruct(42, "rocket", false), Lt(MyStruct(43, "rocket", false)));
  EXPECT_THAT(MyStruct(42, "rocket", false), Lt(MyStruct(42, "rocket", true)));
  EXPECT_THAT(MyStruct(42, "rocket", false), Not(Lt(MyStruct(42, "rocket", false))));
}

TEST(MODULE, MyStructOpGt) {
  EXPECT_THAT(MyStruct(43, "rocket", false), Gt(MyStruct(42, "rocket", false)));
  EXPECT_THAT(MyStruct(42, "rocket", true), Gt(MyStruct(42, "rocket", false)));
  EXPECT_THAT(MyStruct(42, "rocket", false), Not(Gt(MyStruct(42, "rocket", false))));
}

TEST(MODULE, MyDerivedStructOrdering) {
  static_assert(same_as<MyDerivedStruct::Index::Ordering, partial_ordering>);
}

TEST(MODULE, MyStructHash) {
  const MyStruct m1(42, "rocket", true);
  const MyStruct m2(42, "rocket", true);
  const MyStruct m3(43, "rocket", true);

  const auto hash1 = std::hash<MyStruct>()(m1);
  const auto hash2 = std::hash<MyStruct>()(m2);
  const auto hash3 = std::hash<MyStruct>()(m3);

  EXPECT_NE(hash1, 0);
  EXPECT_EQ(hash2, hash1);
  EXPECT_NE(hash1, hash3);
  EXPECT_NE(hash3, 0);
}

TEST(MODULE, MyStructPublic) {
  MyStruct m1(12, "here", true);
  EXPECT_EQ(m1.b, "here");
  get<1>(MyStruct::Public::refs).get(m1) = "everywhere";
  EXPECT_EQ(m1.b, "everywhere");

  const MyStruct m2(13, "there", true);
  EXPECT_EQ(get<0>(MyStruct::Public::refs).get(m2), 13);
  EXPECT_EQ(get<1>(MyStruct::Public::refs).get(m2), "there");
}

TEST(MODULE, MyStructPublicOpEqNe) {
  using type = Instance<MyStruct, MyStruct::Public>;

  static_assert(same_as<decltype(declval<type>() <=> declval<type>()), strong_ordering>);

  const MyStruct m1(42, "rocket", true);
  const MyStruct m2(42, "rocket", false);
  const MyStruct m3(43, "rocket", true);

  const type val1(m1);
  const type val2(m2);
  const type val3(m3);

  EXPECT_EQ(val1, val2);
  EXPECT_EQ(val2, val1);
  EXPECT_NE(val2, val3);
  EXPECT_NE(val3, val2);
}

TEST(MODULE, MyStructPublicHash) {
  using type = Instance<MyStruct, MyStruct::Public>;

  const MyStruct m1 = MyStruct(42, "rocket", true);
  const auto val1 = type(m1);
  const MyStruct m2(42, "rocket", false);
  const auto val2 = type(m2);
  const MyStruct m3(43, "rocket", true);
  const auto val3 = type(m3);

  const u64 hash1 = std::hash<type>()(val1);
  const u64 hash2 = std::hash<type>()(val2);
  const u64 hash3 = std::hash<type>()(val3);

  EXPECT_EQ(hash1, hash2);
  EXPECT_NE(hash1, hash3);
}

TEST(MODULE, VarRef) {
  i32 ä1 = 2;
  string b1 = "hi";
  f32 f1 = .5f;
  auto vars1 = ROCKET_REFLECT_VARS((ä1)(b1)(f1));

  i32 ä2 = 2;
  string b2 = "hi";
  f32 f2 = .5f;
  auto vars2 = ROCKET_REFLECT_VARS((ä2)(b2)(f2));
  EXPECT_EQ(vars2, vars1); // Only the values are compared, not the names

  i32 ä3 = 2;
  string b3 = "hi";
  f32 f3 = .6f;
  const auto vars3 = ROCKET_REFLECT_VARS((ä3)(b3)(f3));

  EXPECT_NE(vars3, vars1);
  EXPECT_LT(vars1, vars3);
  EXPECT_GT(vars3, vars1);

  get<0>(vars1).get() = 3;
  EXPECT_EQ(ä1, 3);
}

TEST(MODULE, VarRefOpEqNe) {
  i32 i1 = 2;
  i64 l1 = 3;
  const auto vars1 = ROCKET_REFLECT_VARS((i1)(l1));

  i32 i2 = 2;
  i64 l2 = 4;
  const auto vars2 = ROCKET_REFLECT_VARS((i2)(l2));

  EXPECT_EQ(get<0>(vars1), get<0>(vars2));
  EXPECT_NE(get<1>(vars1), get<1>(vars2));
}

TEST(MODULE, VarRefOpCmp) {
  static_assert(same_as<decltype(declval<VarRef<i32>>() <=> declval<VarRef<i32>>()), strong_ordering>);
  static_assert(same_as<decltype(declval<VarRef<f32>>() <=> declval<VarRef<f32>>()), partial_ordering>);

  i32 i1 = 1;
  i32 i2 = 2;
  i32 i3 = 3;
  i32 i4 = 4;

  const auto& vars1 = ROCKET_REFLECT_VARS((i1)(i2));
  using type1 = decltype(vars1);
  static_assert(same_as<decltype(declval<type1>() <=> declval<type1>()), strong_ordering>);
  const auto& vars2 = ROCKET_REFLECT_VARS((i3)(i4));

  EXPECT_LT(vars1, vars2);

  f32 f1 = 1.0F;
  f32 f2 = 2.0F;
  f32 f3 = 3.0F;
  f32 f4 = 4.0F;

  const auto& vars3 = ROCKET_REFLECT_VARS((f1)(f2));
  using type3 = decltype(vars3);
  static_assert(same_as<decltype(declval<type3>() <=> declval<type3>()), partial_ordering>);
  const auto& vars4 = ROCKET_REFLECT_VARS((f3)(f4));

  EXPECT_LT(vars3, vars4);
}

TEST(MODULE, VarRefHash) {
  i32 i1 = 2;
  i64 l1 = 3;
  auto vars1 = ROCKET_REFLECT_VARS((i1)(l1));
  const u64 hash1 = codec::HashEncoder<>().encode(vars1);

  auto vars2 = ROCKET_REFLECT_VARS((i1)(l1));
  const u64 hash2 = codec::HashEncoder<>().encode(vars2);

  EXPECT_NE(hash1, 0);
  EXPECT_EQ(hash2, hash1);
}

// EOF
