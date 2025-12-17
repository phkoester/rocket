/*
 * test-terminal.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/terminal.h"

using namespace rocket;
using namespace rocket::terminal;
using namespace std;
using namespace testing;

// `TEST` ---------------------------------------------------------------------------------------------------

/**
 * This test requires `ROCKET_TEST_TERMINAL=1`.
 */
TEST(terminal, position) {
  EXPECT_ENV("ROCKET_TEST_TERMINAL");

  Ansi ansi(true);

  cout << ansi.clear();
  EXPECT_EQ(position(cout), make_pair(1UL, 1UL));

  cout << "abcd";
  EXPECT_EQ(position(cout), make_pair(5UL, 1UL));

  cout << "\nab";
  EXPECT_EQ(position(cout), make_pair(3UL, 2UL)); // XXX

  cout << ansi.move(4, 7);
  EXPECT_EQ(position(cout), make_pair(4UL, 7UL)); // XXX
}

/**
 * This test requires `ROCKET_TEST_TERMINAL=1`.
 */
TEST(terminal, size) {
  EXPECT_ENV("ROCKET_TEST_TERMINAL");

  auto size = terminal::size(cout);
  EXPECT_EQ(static_cast<bool>(size), true);
  EXPECT_GT(size->first, 0UL);
  EXPECT_GT(size->second, 0UL);

  size = terminal::size(cerr);
  EXPECT_EQ(static_cast<bool>(size), true);
  EXPECT_GT(size->first, 0UL);
  EXPECT_GT(size->second, 0UL);

  size = terminal::size(cin);
  EXPECT_EQ(static_cast<bool>(size), true);
  EXPECT_GT(size->first, 0UL);
  EXPECT_GT(size->second, 0UL);
}

// EOF
