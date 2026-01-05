/*
 * test-reflect.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/Type.h"
#include "rocket/format/std.h"
#include "rocket/reflect/reflect.h"
#include "rocket/unicode/ConvertTo.h"

#include "rocket-gtest/matcher/matcher.h"

using namespace rocket;
using namespace rocket::gtest::matcher;
using namespace rocket::reflect;
using namespace std;
using namespace testing;

// `MyStruct` -----------------------------------------------------------------------------------------------

struct MyStruct {
  int ä = 0;
  string b;

  MyStruct() {}

  MyStruct(int ä, string_view b, bool c) : ä(ä), b(b), c(c) {}

  bool getC() const { return c; }

private:

  bool c = false;

public:

  ROCKET_REFLECT_MEMBERS(MyStruct, index, (ä)(b)(c));
};

ROCKET_REFLECT_MEMBERS_DECLARE_GLOBAL(MyStruct, index);
ROCKET_REFLECT_MEMBERS_DECLARE_LOCAL(MyStruct, index);

// `TEST` ---------------------------------------------------------------------------------------------------

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
    int ä;
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
  MyStruct m(42, "rocket", true);
  EXPECT_EQ(fmt::format("{}", m), "(ä=42, b=rocket, c=true)");
  EXPECT_EQ(fmt::format("{:?}", m), "(ä=42, b=\"rocket\", c=true)");
  EXPECT_EQ(fmt::format("{:t}", m), "MyStruct(ä=42, b=rocket, c=true)");
  EXPECT_EQ(fmt::format("{:?t}", m), "MyStruct(ä=42, b=\"rocket\", c=true)");
}

TEST(reflect, VarRef) {
  int ä1 = 2;
  string b1 = "hi";
  float f1 = .5f;

  auto vars1 = ROCKET_REFLECT_VARS((ä1)(b1)(f1));
  EXPECT_EQ(fmt::format("{}", vars1), "(ä1=2, b1=\"hi\", f1=0.5)");

  int ä2 = 2;
  string b2 = "hi";
  float f2 = .5f;
  auto vars2 = ROCKET_REFLECT_VARS((ä2)(b2)(f2));
  EXPECT_EQ(vars2, vars1); // Only the values are compared, not the names
  EXPECT_EQ(fmt::format("{}", vars2), "(ä2=2, b2=\"hi\", f2=0.5)");

  int ä3 = 2;
  string b3 = "hi";
  float f3 = .6f;

  auto vars3 = ROCKET_REFLECT_VARS((ä3)(b3)(f3));
  EXPECT_EQ(fmt::format("{}", vars3), "(ä3=2, b3=\"hi\", f3=0.6)");

  EXPECT_NE(vars3, vars1);
  EXPECT_LT(vars1, vars3);
  EXPECT_GT(vars3, vars1);

  get<0>(vars1).get() = 3;
  EXPECT_EQ(ä1, 3);
}

// EOF
