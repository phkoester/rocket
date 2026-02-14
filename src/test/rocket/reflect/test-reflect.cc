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

  ROCKET_REFLECT_MEMBERS(MyStruct, index, (ä)(b)(c));

  ROCKET_REFLECT_MEMBERS(MyStruct, index2, (ä)(b));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyStruct, index);
ROCKET_REFLECT_MEMBERS_DEFINE(, MyStruct, index);

// #MyDerivedStruct -----------------------------------------------------------------------------------------

struct MyDerivedStruct : MyStruct {
  string d;

  MyDerivedStruct(i32 ä, string_view b, bool c, string_view d, u64 e) : MyStruct(ä, b, c), d(d), e(e) {}

private:

  u64 e = 0;

public:

  ROCKET_REFLECT_MEMBERS_DERIVED(MyStruct, index, MyDerivedStruct, index, (d)(e));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyDerivedStruct, index);
ROCKET_REFLECT_MEMBERS_DEFINE(, MyDerivedStruct, index);

// #TEST ----------------------------------------------------------------------------------------------------

TEST(reflect, MyStruct) {
  MyStruct m1(12, "here", true);
  EXPECT_EQ(m1.b, "here");
  get<1>(MyStruct::index()).get(m1) = "everywhere";
  EXPECT_EQ(m1.b, "everywhere");
  EXPECT_EQ(m1.getC(), true);
  get<2>(MyStruct::index()).get(m1) = false;
  EXPECT_EQ(m1.getC(), false);

  const MyStruct m2(13, "there", true);
  EXPECT_EQ(get<1>(MyStruct::index()).get(m2), "there");
  EXPECT_EQ(get<2>(MyStruct::index()).get(m2), true);
}

/**
 * Tests that the macro #ROCKET_REFLECT_MEMBERS doesn't affect the size of a class.
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
  EXPECT_NE(MyStruct(42, "rocket", true), MyStruct(42, "rocket", false));
}

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

TEST(reflect, MyStructOpOutput) {
  ostringstream os;
  os << MyStruct(42, "rocket", true);
  EXPECT_EQ(os.str(), "(ä=42, b=rocket, c=true)");
}

TEST(reflect, MyStructFormat) {
  const MyStruct m(42, "rocket", true);
  EXPECT_EQ(fmt::format("{}", m), "(ä=42, b=rocket, c=true)");
  EXPECT_EQ(fmt::format("{:?}", m), "(ä=42, b=\"rocket\", c=true)");
  EXPECT_EQ(fmt::format("{:t}", m), "MyStruct(ä=42, b=rocket, c=true)");
  EXPECT_EQ(fmt::format("{:?t}", m), "MyStruct(ä=42, b=\"rocket\", c=true)");
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

TEST(reflect, MyStructIndex2) {
  MyStruct m1(12, "here", true);
  EXPECT_EQ(m1.b, "here");
  get<1>(MyStruct::index()).get(m1) = "everywhere";
  EXPECT_EQ(m1.b, "everywhere");

  const MyStruct m2(13, "there", true);
  EXPECT_EQ(get<0>(MyStruct::index()).get(m2), 13);
  EXPECT_EQ(get<1>(MyStruct::index()).get(m2), "there");
}

TEST(reflect, MyStructIndex2Eq) {
  const MyStruct m1(42, "rocket", true);
  const MyStruct m2(42, "rocket", false);
  const MyStruct m3(43, "rocket", true);
  const auto& index2 = MyStruct::index2();
  EXPECT_EQ(eq(m1, m2, index2), true);
  EXPECT_EQ(eq(m1, m3, index2), false);
}

TEST(reflect, MyStructIndex2Ne) {
  const MyStruct m1(42, "rocket", true);
  const MyStruct m2(42, "rocket", false);
  const MyStruct m3(43, "rocket", true);
  const auto& index2 = MyStruct::index2();
  EXPECT_EQ(ne(m1, m2, index2), false);
  EXPECT_EQ(ne(m1, m3, index2), true);
}

TEST(reflect, MyStructIndex2Hash) {
  const MyStruct m1(42, "rocket", true);
  const MyStruct m2(42, "rocket", false);
  const MyStruct m3(43, "rocket", true);

  const u64 hash1 = reflect::hash(m1, MyStruct::index2());
  const u64 hash2 = reflect::hash(m2, MyStruct::index2());
  const u64 hash3 = reflect::hash(m3, MyStruct::index2());

  EXPECT_EQ(hash1, hash2);
  EXPECT_NE(hash1, hash3);
}

TEST(reflect, MyStructIndex2Write) {
  const MyStruct m(42, "rocket", true);
  nio::StringSink out;
  write(out, m, MyStruct::index2());
  EXPECT_EQ(out.str(), "(ä=42, b=rocket)");
}

TEST(reflect, MyDerivedStructWrite) {
  const MyDerivedStruct m(41, "rocket", true, "everywhere", 42_u64);
  nio::StringSink out;
  write(out, m, MyDerivedStruct::index());
  EXPECT_EQ(out.str(), "(ä=41, b=rocket, c=true, d=everywhere, e=42)");
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
  EXPECT_LT(vars1, vars3);
  EXPECT_GT(vars3, vars1);

  get<0>(vars1).get() = 3;
  EXPECT_EQ(ä1, 3);
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
