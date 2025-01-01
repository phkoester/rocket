/*
 * test-assert.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/S.h"
#include "rocket/assert.h"
#include "rocket/except.h"

#include <boost/preprocessor/tuple/elem.hpp>

using namespace rocket;
using namespace std;
using namespace testing;

// Local functions ------------------------------------------------------------------------------------------

bool oopsCalled = false;

const char* oops() { oopsCalled = true; return "oops"; }

// 'TEST' ---------------------------------------------------------------------------------------------------

TEST(assert, ROCKET_ASSERT) {
  EXPECT_NO_THROW({ ROCKET_ASSERT(true); });

  oopsCalled = false;
  ROCKET_ASSERT(true, S << oops());
  EXPECT_FALSE(oopsCalled);
}

#define NDEBUG 1
#include "rocket/assert.h"
TEST(assert, ROCKET_ASSERT_NDEBUG) {
  ROCKET_ASSERT(false, "This must have no effect");
}
#undef NDEBUG
#include "rocket/assert.h"

TEST(assert, ROCKET_CHECK) {
  char c = 'a';
  
  EXPECT_THAT(
      [&] { ROCKET_CHECK(c, c == 'b'); },
      ThrowsMessage<except::InvalidArgument>(HasSubstr("Parameter 'c': Check \"c == 'b'\" failed")));

  EXPECT_THAT(
      [&] { ROCKET_CHECK(c, c == 'b', "oops"); },
      ThrowsMessage<except::InvalidArgument>(HasSubstr("Parameter 'c': Check \"c == 'b'\" failed: oops")));
}

TEST(assert, ROCKET_EXPECT) {
  oopsCalled = false;
  EXPECT_THAT(
      [] { ROCKET_EXPECT(true && false, S << oops()); },
      ThrowsMessage<except::InvalidState>(HasSubstr("Expectation \"true && false\" failed: oops")));
  EXPECT_TRUE(oopsCalled);
}

// EOF
