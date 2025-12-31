/*
 * test-assert.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/Exception.h"
#include "rocket/assert.h"

using namespace rocket;
using namespace std;
using namespace testing;

// Local functions ------------------------------------------------------------------------------------------

bool oopsCalled = false;

const char* oops() { oopsCalled = true; return "oops"; }

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(assert, ROCKET_ASSERT) {
  EXPECT_NO_THROW({ ROCKET_ASSERT(true); });

  oopsCalled = false;
  ROCKET_ASSERT(true, "{}", oops());
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
      ThrowsMessage<InvalidArgument>(HasSubstr("Parameter `c`: Check `c == 'b'` failed")));

  EXPECT_THAT(
      [&] { ROCKET_CHECK(c, c == 'b', "oops"); },
      ThrowsMessage<InvalidArgument>(HasSubstr("Parameter `c`: Check `c == 'b'` failed: oops")));
}

TEST(assert, ROCKET_EXPECT) {
  oopsCalled = false;
  EXPECT_THAT(
      [] { ROCKET_EXPECT(true && false, "{}", oops()); },
      ThrowsMessage<InvalidState>(HasSubstr("Expectation `true && false` failed: oops")));
  EXPECT_TRUE(oopsCalled);
}

// EOF
