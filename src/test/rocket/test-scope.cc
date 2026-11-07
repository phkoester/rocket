/*
 * test-scope.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/scope.h"

#include <stdexcept>
#include <vector>

namespace {

// `OnDestroy` ----------------------------------------------------------------------------------------------

/// Runs a function from its destructor, i.e., during stack unwinding if an exception is in flight.
struct OnDestroy {
  explicit OnDestroy(function<void()>&& fn) : fn_(std::move(fn)) {}

  OnDestroy(const OnDestroy&) = delete;

  ~OnDestroy() { fn_(); }

  OnDestroy& operator=(const OnDestroy&) = delete;

private:

  function<void()> fn_;
};

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

// `ScopeExit` ..............................................................................................

TEST(scope, ScopeExitRunsOnNormalExit) {
  i32 count = 0;
  {
    scope_exit guard([&count] { ++count; });
    EXPECT_EQ(count, 0);
  }
  EXPECT_EQ(count, 1);
}

TEST(scope, ScopeExitRunsOnException) {
  i32 count = 0;
  try {
    scope_exit guard([&count] { ++count; });
    throw runtime_error("test");
  } catch (const runtime_error&) {
    EXPECT_EQ(count, 1);
  }
  EXPECT_EQ(count, 1);
}

TEST(scope, ScopeExitRelease) {
  i32 count = 0;
  {
    scope_exit guard([&count] { ++count; });
    guard.release();
  }
  EXPECT_EQ(count, 0);

  try {
    scope_exit guard([&count] { ++count; });
    guard.release();
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 0);
}

TEST(scope, ScopeExitReverseOrder) {
  vector<i32> order;
  {
    scope_exit guard1([&order] { order.push_back(1); });
    scope_exit guard2([&order] { order.push_back(2); });
    scope_exit guard3([&order] { order.push_back(3); });
  }
  EXPECT_EQ(order, (vector<i32> { 3, 2, 1 }));
}

TEST(scope, ScopeExitMacro) {
  vector<i32> order;
  {
    ROCKET_SCOPE_EXIT([&order] { order.push_back(1); });
    ROCKET_SCOPE_EXIT([&order] { order.push_back(2); });
    EXPECT_TRUE(order.empty());
  }
  EXPECT_EQ(order, (vector<i32> { 2, 1 }));
}

// `ScopeFail` ..............................................................................................

TEST(scope, ScopeFailDoesNotRunOnNormalExit) {
  i32 count = 0;
  {
    scope_fail guard([&count] { ++count; });
  }
  EXPECT_EQ(count, 0);
}

TEST(scope, ScopeFailRunsOnException) {
  i32 count = 0;
  try {
    scope_fail guard([&count] { ++count; });
    throw runtime_error("test");
  } catch (const runtime_error&) {
    EXPECT_EQ(count, 1);
  }
  EXPECT_EQ(count, 1);
}

TEST(scope, ScopeFailDoesNotRunOnCaughtException) {
  i32 count = 0;
  {
    scope_fail guard([&count] { ++count; });
    try {
      throw runtime_error("test");
    } catch (const runtime_error&) {}
  }
  EXPECT_EQ(count, 0);
}

TEST(scope, ScopeFailRelease) {
  i32 count = 0;
  try {
    scope_fail guard([&count] { ++count; });
    guard.release();
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 0);
}

TEST(scope, ScopeFailDuringUnwinding) {
  // A guard created during stack unwinding must not fire if its own scope exits normally
  i32 count = 0;
  try {
    OnDestroy onDestroy([&count] {
      scope_fail guard([&count] { ++count; });
    });
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 0);

  // ... but it must fire if its own scope is left by a new exception
  try {
    OnDestroy onDestroy([&count] {
      try {
        scope_fail guard([&count] { ++count; });
        throw logic_error("nested");
      } catch (const logic_error&) {}
    });
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 1);
}

TEST(scope, ScopeFailMacro) {
  i32 count = 0;
  {
    ROCKET_SCOPE_FAIL([&count] { ++count; });
  }
  EXPECT_EQ(count, 0);

  try {
    ROCKET_SCOPE_FAIL([&count] { ++count; });
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 1);
}

// `ScopeSuccess` ...........................................................................................

TEST(scope, ScopeSuccessRunsOnNormalExit) {
  i32 count = 0;
  {
    scope_success guard([&count] { ++count; });
    EXPECT_EQ(count, 0);
  }
  EXPECT_EQ(count, 1);
}

TEST(scope, ScopeSuccessDoesNotRunOnException) {
  i32 count = 0;
  try {
    scope_success guard([&count] { ++count; });
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 0);
}

TEST(scope, ScopeSuccessRunsAfterCaughtException) {
  i32 count = 0;
  {
    scope_success guard([&count] { ++count; });
    try {
      throw runtime_error("test");
    } catch (const runtime_error&) {}
  }
  EXPECT_EQ(count, 1);
}

TEST(scope, ScopeSuccessRelease) {
  i32 count = 0;
  {
    scope_success guard([&count] { ++count; });
    guard.release();
  }
  EXPECT_EQ(count, 0);
}

TEST(scope, ScopeSuccessDuringUnwinding) {
  // A guard created during stack unwinding must fire if its own scope exits normally
  i32 count = 0;
  try {
    OnDestroy onDestroy([&count] {
      scope_success guard([&count] { ++count; });
    });
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 1);

  // ... but it must not fire if its own scope is left by a new exception
  try {
    OnDestroy onDestroy([&count] {
      try {
        scope_success guard([&count] { ++count; });
        throw logic_error("nested");
      } catch (const logic_error&) {}
    });
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 1);
}

TEST(scope, ScopeSuccessMacro) {
  i32 count = 0;
  {
    ROCKET_SCOPE_SUCCESS([&count] { ++count; });
  }
  EXPECT_EQ(count, 1);

  try {
    ROCKET_SCOPE_SUCCESS([&count] { ++count; });
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 1);
}

// Move construction ........................................................................................

TEST(scope, MoveTransfersOwnership) {
  i32 count = 0;
  {
    scope_exit a([&count] { ++count; });
    scope_exit b(std::move(a));
  }
  EXPECT_EQ(count, 1);

  count = 0;
  {
    scope_success a([&count] { ++count; });
    scope_success b(std::move(a));
  }
  EXPECT_EQ(count, 1);

  count = 0;
  try {
    scope_fail a([&count] { ++count; });
    scope_fail b(std::move(a));
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 1);
}

TEST(scope, ExitFunctionFromLvalue) {
  i32 count = 0;
  const auto fn = [&count] { ++count; };
  {
    scope_exit guard(fn);
  }
  EXPECT_EQ(count, 1);
}

// EOF
