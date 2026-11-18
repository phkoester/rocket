/*
 * test-assert.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Exception.h"
#include "rocket/assert.h"

#include <boost/preprocessor/cat.hpp>

#include <csignal>

#define MODULE rocketAssert
#define MODULE_DEATH_TEST BOOST_PP_CAT(MODULE, DeathTest)

namespace {

// Local functions ------------------------------------------------------------------------------------------

bool oopsCalled = false;

const char* oops() { oopsCalled = true; return "oops"; }

} // namespace

// Tests ====================================================================================================

TEST(MODULE, assertTrue) {
  EXPECT_NO_THROW({ ROCKET_ASSERT(true); });

  oopsCalled = false;
  ROCKET_ASSERT(true, "{}", oops());
  EXPECT_FALSE(oopsCalled);
}

TEST(MODULE_DEATH_TEST, assertFalse) { // NOLINT(*-complexity,*-underscore-*)
  EXPECT_EXIT(
      { ROCKET_ASSERT(false, "my message: {}", 42); },
      ABORTED(), "assertion `false` failed: my message: 42");
}

TEST(MODULE, check) {
  char c = 'a';

  EXPECT_THAT(
      [&] { ROCKET_CHECK(c, c == 'b'); },
      ThrowsMessage<InvalidArgument>(EndsWith("parameter `c`: check `c == 'b'` failed")));

  EXPECT_THAT(
      [&] { ROCKET_CHECK(c, c == 'b', "oops"); },
      ThrowsMessage<InvalidArgument>(EndsWith("parameter `c`: check `c == 'b'` failed: oops")));
}

TEST(MODULE, expect) {
  oopsCalled = false;
  EXPECT_THAT(
      [] { ROCKET_EXPECT(true && false, "{}", oops()); },
      ThrowsMessage<InvalidState>(EndsWith("expectation `true && false` failed: oops")));
  EXPECT_TRUE(oopsCalled);
}

TEST(MODULE, fail) {
  oopsCalled = false;
  EXPECT_THAT(
      [] { ROCKET_FAIL("{}", oops()); },
      ThrowsMessage<InvalidState>(EndsWith("oops")));
  EXPECT_TRUE(oopsCalled);
}

TEST(MODULE_DEATH_TEST, terminate) { // NOLINT(*-underscore-*)
  EXPECT_EXIT(
      { ROCKET_TERMINATE("my message: {}", 42); },
      ABORTED(), "my message: 42");
}

// EOF
