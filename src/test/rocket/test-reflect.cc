/*
 * test-reflect.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-std-decl.h"
#include "rocket/codec-std.h"

#include "rocket/reflect.h"

#include "rocket-gtest/matcher.h"

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

ROCKET_REFLECT_MEMBERS_DEFINE_OP_EQ(MyStruct, index);
ROCKET_REFLECT_MEMBERS_DEFINE_OP_NE(MyStruct, index);

ROCKET_REFLECT_MEMBERS_DEFINE_OP_GT(MyStruct, index);
ROCKET_REFLECT_MEMBERS_DEFINE_OP_LT(MyStruct, index);

bool hashValueCalled = false;

#if 0
ROCKET_REFLECT_MEMBERS_DEFINE_FN_HASH_VALUE(MyStruct, index);
#else
size_t
hash_value(const MyStruct& v) {
  hashValueCalled = true;
  return reflect::hash(&v, MyStruct::index());
}
#endif

ROCKET_REFLECT_MEMBERS_DEFINE_FN_PARSE_RON(MyStruct, index);
// XXX ROCKET_REFLECT_MEMBERS_DEFINE_FN_PRINT_RON(MyStruct, index);

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(reflect, eq) {
  EXPECT_EQ(MyStruct(42, "rocket", true), MyStruct(42, "rocket", true));
  EXPECT_NE(MyStruct(42, "rocket", true), MyStruct(42, "rocket", false));
}

TEST(reflect, lt) {
  EXPECT_THAT(MyStruct(42, "rocket", false), Lt(MyStruct(43, "rocket", false)));
  EXPECT_THAT(MyStruct(42, "rocket", false), Lt(MyStruct(42, "rocket", true)));
  EXPECT_THAT(MyStruct(42, "rocket", false), Not(Lt(MyStruct(42, "rocket", false))));
}

TEST(reflect, gt) {
  EXPECT_THAT(MyStruct(43, "rocket", false), Gt(MyStruct(42, "rocket", false)));
  EXPECT_THAT(MyStruct(42, "rocket", true), Gt(MyStruct(42, "rocket", false)));
  EXPECT_THAT(MyStruct(42, "rocket", false), Not(Gt(MyStruct(42, "rocket", false))));
}

TEST(reflect, hash) {
  EXPECT_EQ(
      ::boost::hash<MyStruct>()(MyStruct(42, "rocket", false)),
      ::boost::hash<MyStruct>()(MyStruct(42, "rocket", false)));
  EXPECT_NE(
      ::boost::hash<MyStruct>()(MyStruct(42, "rocket", true)),
      ::boost::hash<MyStruct>()(MyStruct(42, "rocket", false)));

  EXPECT_TRUE(hashValueCalled);
}

TEST(reflect, parseRon) {
  using type = MyStruct;

  type v { 12, "hey", true };
#if 0 // XXX
  EXPECT_EQ(S << v, "{ä=12, b=\"hey\", c=true}");
#endif

  {
    auto is = io::is("{}");
    parseRon(is, v);
    EXPECT_EQ(v, type(0, "", false));
  }

  {
    auto is = io::is("{c=true, b=\"ho\", ä=13}");
    parseRon(is, v);
    EXPECT_EQ(v, type(13, "ho", true));
  }

  {
    auto is = io::is("{ä=13, b=\"ho\", =true}");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure(16, { 16, 17 }, HasSubstr("Expected at least 1 character before whitespace, '=', ',', or '}', got 0")));
  }

  {
    auto is = io::is("\t{ä=13, b=\"ho\", charles=true}");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure(17, { 17, 24 }, HasSubstr("Invalid name: \"charles\"")));
  }
}

#if 0 // XXX Mit format() testen
TEST(reflect, printRon) {
  using type = MyStruct;

  EXPECT_EQ(S << type(42, "rocket", true), "{ä=42, b=\"rocket\", c=true}");
}
#endif

TEST(reflect, memberRef) {
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

#if 0 // XXX
  ostringstream os;
  reflect::printRon(os, &m1, MyStruct::index());
  EXPECT_EQ(os.str(), "{ä=12, b=\"everywhere\", c=false}");
#endif
}

TEST(reflect, varRef) {
  int ä1 = 2;
  string b1 = "hi";
  float f1 = .5f;

  auto vars1 = ROCKET_REFLECT_VARS((ä1)(b1)(f1));

  int ä2 = 2;
  string b2 = "hi";
  float f2 = .6f;

  auto vars2 = ROCKET_REFLECT_VARS((ä2)(b2)(f2));

  EXPECT_TRUE(reflect::eq<void>(nullptr, vars1, nullptr, vars1));
  EXPECT_FALSE(reflect::eq<void>(nullptr, vars1, nullptr, vars2));

#if 0 // XXX
  ostringstream os;
  reflect::printRon<void>(os, nullptr, vars1);
  {
    EXPECT_EQ(os.str(), "{ä1=2, b1=\"hi\", f1=0.5}");
  }
#endif

  get<0>(vars1).get() = 3;
  EXPECT_EQ(ä1, 3);
}

/**
 * Tests that the macro #ROCKET_REFLECT_MEMBERS doesn't affect the size of a class.
 */
TEST(reflect, sizeof) {
  struct BareStruct {
    int ä;
    string b;
    bool c;
  };
  EXPECT_EQ(sizeof(BareStruct), sizeof(MyStruct));
}

// EOF
