/*
 * test-assert.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Exception.h"
#include "rocket/assert.h"

#include <csignal>

#define MODULE rocketAssert
#define MODULE_DEATH_TEST MODULE##DeathTest
namespace {

// Local functions ------------------------------------------------------------------------------------------

bool oopsCalled = false;

const char* oops() { oopsCalled = true; return "oops"; }

} // namespace

// Tests ====================================================================================================

TEST(MODULE, RocketAssertTrue) {
  EXPECT_NO_THROW({ ROCKET_ASSERT(true); });

  oopsCalled = false;
  ROCKET_ASSERT(true, "{}", oops());
  EXPECT_FALSE(oopsCalled);
}

TEST(MODULE_DEATH_TEST, RocketAssertFalse) { // NOLINT(*-complexity)
  EXPECT_EXIT(
      { ROCKET_ASSERT(false, "My message: {}", 42); },
      ABORTED(), "Assertion `false` failed: My message: 42");
}

TEST(MODULE, RocketCheck) {
  char c = 'a';

  EXPECT_THAT(
      [&] { ROCKET_CHECK(c, c == 'b'); },
      ThrowsMessage<InvalidArgument>(EndsWith("Parameter `c`: Check `c == 'b'` failed")));

  EXPECT_THAT(
      [&] { ROCKET_CHECK(c, c == 'b', "oops"); },
      ThrowsMessage<InvalidArgument>(EndsWith("Parameter `c`: Check `c == 'b'` failed: oops")));
}

TEST(MODULE, RocketExpect) {
  oopsCalled = false;
  EXPECT_THAT(
      [] { ROCKET_EXPECT(true && false, "{}", oops()); },
      ThrowsMessage<InvalidState>(EndsWith("Expectation `true && false` failed: oops")));
  EXPECT_TRUE(oopsCalled);
}

TEST(MODULE, RocketFail) {
  oopsCalled = false;
  EXPECT_THAT(
      [] { ROCKET_FAIL("{}", oops()); },
      ThrowsMessage<InvalidState>(EndsWith("oops")));
  EXPECT_TRUE(oopsCalled);
}

TEST(MODULE_DEATH_TEST, RocketTerminate) {
  EXPECT_EXIT(
      { ROCKET_TERMINATE("My message: {}", 42); },
      ABORTED(), "My message: 42");
}

// EOF
