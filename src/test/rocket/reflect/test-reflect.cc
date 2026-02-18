/*
 * test-reflect.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/reflect/reflect-codec.h"

#include <fmt/ranges.h>

using namespace rocket::reflect;

// #MyStruct ------------------------------------------------------------------------------------------------

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

ROCKET_REFLECT_MEMBERS_DECLARE(, MyStruct, Index);
ROCKET_REFLECT_MEMBERS_DEFINE(, MyStruct, Index);

// #MyDerivedStruct -----------------------------------------------------------------------------------------

struct MyDerivedStruct : MyStruct {
  string d;

  MyDerivedStruct(i32 ä, string_view b, bool c, string_view d, u64 e) : MyStruct(ä, b, c), d(d), e(e) {}

private:

  u64 e = 0;

public:

  ROCKET_REFLECT_MEMBERS_DERIVED(MyStruct, Index, MyDerivedStruct, Index, (d)(e));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyDerivedStruct, Index);
ROCKET_REFLECT_MEMBERS_DEFINE(, MyDerivedStruct, Index);

// #TEST ----------------------------------------------------------------------------------------------------

TEST(reflect, MyStruct) {
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
TEST(reflect, MyStructSizeof) {
  struct BareStruct {
    i32 ä;
    string b;
    bool c;
  };
  EXPECT_EQ(sizeof(BareStruct), sizeof(MyStruct));
}

TEST(reflect, MyStructOpEq) {
  EXPECT_EQ(MyStruct(42, "rocket", true), MyStruct(42, "rocket", true));
}

TEST(reflect, MyStructOpNe) {
  EXPECT_NE(MyStruct(42, "rocket", true), MyStruct(42, "rocket", false));
}

#if 0 // XXX
TEST(reflect, MyStructOpLt) {
  EXPECT_THAT(MyStruct(42, "rocket", false), Lt(MyStruct(43, "rocket", false)));
  EXPECT_THAT(MyStruct(42, "rocket", false), Lt(MyStruct(42, "rocket", true)));
  EXPECT_THAT(MyStruct(42, "rocket", false), Not(Lt(MyStruct(42, "rocket", false))));
}

TEST(reflect, MyStructOpGt) {
  EXPECT_THAT(MyStruct(43, "rocket", false), Gt(MyStruct(42, "rocket", false)));
  EXPECT_THAT(MyStruct(42, "rocket", true), Gt(MyStruct(42, "rocket", false)));
  EXPECT_THAT(MyStruct(42, "rocket", false), Not(Gt(MyStruct(42, "rocket", false))));
}
#endif

TEST(reflect, MyStructOpOutput) {
  ostringstream os;
  os << MyStruct(42, "rocket", true);
  EXPECT_EQ(os.str(), "(ä=42, b=\"rocket\", c=true)");
}

TEST(reflect, MyStructFormat) {
  const MyStruct m(42, "rocket", true);
  EXPECT_EQ(fmt::format("{}", m), "(ä=42, b=\"rocket\", c=true)");
  EXPECT_EQ(fmt::format("{:it}", m),
    "MyStruct(\n"
    "  ä=42,\n"
    "  b=\"rocket\",\n"
    "  c=true\n"
    ")");
  EXPECT_EQ(fmt::format("{:t}", m), "MyStruct(ä=42, b=\"rocket\", c=true)");
}

TEST(reflect, MyStructHash) {
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

TEST(reflect, MyStructPublic) {
  MyStruct m1(12, "here", true);
  EXPECT_EQ(m1.b, "here");
  get<1>(MyStruct::Public::refs).get(m1) = "everywhere";
  EXPECT_EQ(m1.b, "everywhere");

  const MyStruct m2(13, "there", true);
  EXPECT_EQ(get<0>(MyStruct::Public::refs).get(m2), 13);
  EXPECT_EQ(get<1>(MyStruct::Public::refs).get(m2), "there");
}

TEST(reflect, MyStructPublicOpEqNe) {
  using type = Instance<MyStruct, MyStruct::Public>;

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

TEST(reflect, MyStructPublicHash) {
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

TEST(reflect, MyStructPublicFormat) {
  using type = Instance<MyStruct, MyStruct::Public>;

  const MyStruct m(42, "rocket", true);
  type val(m);
  EXPECT_EQ(fmt::format("{}", val), "(ä=42, b=\"rocket\")");
  EXPECT_EQ(fmt::format("{:t}", val), "MyStruct(ä=42, b=\"rocket\")");
}

TEST(reflect, MyDerivedStructFormat) {
  const MyDerivedStruct m(41, "rocket", true, "everywhere", 42_u64);
  EXPECT_EQ(fmt::format("{:t}", m), "MyDerivedStruct(ä=41, b=\"rocket\", c=true, d=\"everywhere\", e=42)");
}

TEST(reflect, VarRef) {
  i32 ä1 = 2;
  string b1 = "hi";
  f32 f1 = .5f;

  auto vars1 = ROCKET_REFLECT_VARS((ä1)(b1)(f1));
  EXPECT_EQ(fmt::format("{}", vars1), "(ä1=2, b1=\"hi\", f1=0.5)");

  i32 ä2 = 2;
  string b2 = "hi";
  f32 f2 = .5f;
  auto vars2 = ROCKET_REFLECT_VARS((ä2)(b2)(f2));
  EXPECT_EQ(vars2, vars1); // Only the values are compared, not the names
  EXPECT_EQ(fmt::format("{}", vars2), "(ä2=2, b2=\"hi\", f2=0.5)");

  i32 ä3 = 2;
  string b3 = "hi";
  f32 f3 = .6f;

  const auto vars3 = ROCKET_REFLECT_VARS((ä3)(b3)(f3));
  EXPECT_EQ(fmt::format("{}", vars3), "(ä3=2, b3=\"hi\", f3=0.6)");

  EXPECT_NE(vars3, vars1);
  // XXX EXPECT_LT(vars1, vars3);
  // XXX EXPECT_GT(vars3, vars1);

  get<0>(vars1).get() = 3;
  EXPECT_EQ(ä1, 3);
}

TEST(reflect, VarRefOpEqNe) {
  i32 i1 = 2;
  i64 l1 = 3;
  const auto vars1 = ROCKET_REFLECT_VARS((i1)(l1));

  i32 i2 = 2;
  i64 l2 = 4;
  const auto vars2 = ROCKET_REFLECT_VARS((i2)(l2));

  EXPECT_EQ(get<0>(vars1), get<0>(vars2));
  EXPECT_NE(get<1>(vars1), get<1>(vars2));
}

TEST(reflect, VarRefOpOutput) {
  i32 i = 2;
  i64 l = 3;
  const auto vars = ROCKET_REFLECT_VARS((i)(l));

  const auto v0 = get<0>(vars);
  ostringstream os;
  os << v0;
  EXPECT_EQ(os.str(), "i=2");
}

TEST(reflect, VarRefHash) {
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
