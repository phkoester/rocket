/*
 * test-terminal.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/nio/nio.h"
#include "rocket/system/terminal/terminal.h"

using namespace rocket::system::terminal;

// #TEST ----------------------------------------------------------------------------------------------------

/**
 * This test requires `ROCKET_TEST_TERMINAL=1`.
 */
TEST(terminal, position) {
  EXPECT_ENV(ROCKET_TEST_TERMINAL);

  Ansi ansi(true);

  auto& out = nio::out;

  out.write(ansi.clear());
  EXPECT_EQ(position(out), make_pair(1UL, 1UL));

  out.write("abcd");
  EXPECT_EQ(position(out), make_pair(5UL, 1UL));

  out.write("\nab");
  EXPECT_EQ(position(out), make_pair(3UL, 2UL));

  out.write(ansi.move(4, 7));
  EXPECT_EQ(position(out), make_pair(4UL, 7UL));
}

/**
 * This test requires `ROCKET_TEST_TERMINAL=1`.
 */
TEST(terminal, size) {
  EXPECT_ENV(ROCKET_TEST_TERMINAL);

  auto size = system::terminal::size(nio::out);
  EXPECT_EQ(static_cast<bool>(size), true);
  EXPECT_GT(size->first, 0UL);
  EXPECT_GT(size->second, 0UL);

  size = system::terminal::size(nio::err);
  EXPECT_EQ(static_cast<bool>(size), true);
  EXPECT_GT(size->first, 0UL);
  EXPECT_GT(size->second, 0UL);
}

// EOF
