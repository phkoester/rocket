/*
 * test-assert.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/Exception.h"
#include "rocket/assert.h"

// Local functions ------------------------------------------------------------------------------------------

bool oopsCalled = false;

const char* oops() { oopsCalled = true; return "oops"; }

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(assert, RocketAssertTrue) {
  EXPECT_NO_THROW({ ROCKET_ASSERT(true); });

  oopsCalled = false;
  ROCKET_ASSERT(true, "{}", oops());
  EXPECT_FALSE(oopsCalled);
}

TEST(assertDeathTest, RocketAssertFalse) {
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_EXIT(
      { ROCKET_ASSERT(false, "My message: {}", 42); },
      KilledBySignal(SIGABRT), "Assertion `false` failed: My message: 42");
}

#define NDEBUG 1
#include "rocket/assert.h"
TEST(assert, RocketAssertFalseNdebug) {
  ROCKET_ASSERT(false, "This must have no effect");
}
#undef NDEBUG
#include "rocket/assert.h"

TEST(assert, RocketCheck) {
  char c = 'a';

  EXPECT_THAT(
      [&] { ROCKET_CHECK(c, c == 'b'); },
      ThrowsMessage<InvalidArgument>(HasSubstr("Parameter `c`: Check `c == 'b'` failed")));

  EXPECT_THAT(
      [&] { ROCKET_CHECK(c, c == 'b', "oops"); },
      ThrowsMessage<InvalidArgument>(HasSubstr("Parameter `c`: Check `c == 'b'` failed: oops")));
}

TEST(assert, RocketExpect) {
  oopsCalled = false;
  EXPECT_THAT(
      [] { ROCKET_EXPECT(true && false, "{}", oops()); },
      ThrowsMessage<InvalidState>(HasSubstr("Expectation `true && false` failed: oops")));
  EXPECT_TRUE(oopsCalled);
}

TEST(assert, RocketFail) {
  oopsCalled = false;
  EXPECT_THAT(
      [] { ROCKET_FAIL("{}", oops()); },
      ThrowsMessage<InvalidState>(HasSubstr("oops")));
  EXPECT_TRUE(oopsCalled);
}

TEST(assertDeathTest, RocketTerminate) {
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_EXIT(
      { ROCKET_TERMINATE("My message: {}", 42); },
      KilledBySignal(SIGABRT), "My message: 42");
}

// EOF
