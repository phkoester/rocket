/*
 * test-EqualToEncoder.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/EqualToEncoder.h"
#include "rocket/reflect/reflect-codec.h"

using namespace rocket;
using namespace rocket::codec;

// #MyStruct ------------------------------------------------------------------------------------------------

struct MyStruct {
  int ärger;
  bool ökonom;
  string übermut;
  vector<int> vec;

  ROCKET_REFLECT_MEMBERS(MyStruct, Index, (ärger)(ökonom)(übermut)(vec));
  ROCKET_REFLECT_MEMBERS(MyStruct, Three, (ärger)(ökonom)(übermut));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyStruct, Index);
ROCKET_REFLECT_MEMBERS_DEFINE(, MyStruct, Index);

// #TEST ----------------------------------------------------------------------------------------------------

TEST(EqualToEncoder, bool) {
  EqualToEncoder<> encoder;
  EXPECT_EQ(encoder.encode(false, false), true);
  EXPECT_EQ(encoder.encode(false, true), false);
  EXPECT_EQ(encoder.encode(true, false), false);
  EXPECT_EQ(encoder.encode(true, true), true);
}

// EOF
