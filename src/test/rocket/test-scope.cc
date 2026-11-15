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

// Tests ====================================================================================================

// `scope_exit` ---------------------------------------------------------------------------------------------

TEST(scope, scopeExitRunsOnNormalExit) {
  i32 count = 0;
  {
    const scope_exit guard([&count] { ++count; });
    EXPECT_EQ(count, 0);
  }
  EXPECT_EQ(count, 1);
}

TEST(scope, scopeExitRunsOnException) {
  i32 count = 0;
  try {
    const scope_exit guard([&count] { ++count; });
    throw runtime_error("test");
  } catch (const runtime_error&) {
    EXPECT_EQ(count, 1);
  }
  EXPECT_EQ(count, 1);
}

TEST(scope, scopeExitRelease) {
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

TEST(scope, scopeExitReverseOrder) {
  vector<i32> order;
  {
    const scope_exit guard1([&order] { order.push_back(1); });
    const scope_exit guard2([&order] { order.push_back(2); });
    const scope_exit guard3([&order] { order.push_back(3); });
  }
  EXPECT_EQ(order, (vector<i32> { 3, 2, 1 }));
}

TEST(scope, scopeExitMacro) {
  vector<i32> order;
  {
    ROCKET_SCOPE_EXIT([&order] { order.push_back(1); });
    ROCKET_SCOPE_EXIT([&order] { order.push_back(2); });
    EXPECT_TRUE(order.empty());
  }
  EXPECT_EQ(order, (vector<i32> { 2, 1 }));
}

TEST(scope, scopeExitFunctionFromLvalue) {
  i32 count = 0;
  const auto fn = [&count] { ++count; };
  {
    const scope_exit guard(fn);
  }
  EXPECT_EQ(count, 1);
}

// `scope_fail` ---------------------------------------------------------------------------------------------

TEST(scope, scopeFailDoesNotRunOnNormalExit) {
  i32 count = 0;
  {
    const scope_fail guard([&count] { ++count; });
  }
  EXPECT_EQ(count, 0);
}

TEST(scope, scopeFailRunsOnException) {
  i32 count = 0;
  try {
    const scope_fail guard([&count] { ++count; });
    throw runtime_error("test");
  } catch (const runtime_error&) {
    EXPECT_EQ(count, 1);
  }
  EXPECT_EQ(count, 1);
}

TEST(scope, scopeFailDoesNotRunOnCaughtException) {
  i32 count = 0;
  {
    const scope_fail guard([&count] { ++count; });
    try {
      throw runtime_error("test");
    } catch (const runtime_error&) {}
  }
  EXPECT_EQ(count, 0);
}

TEST(scope, scopeFailRelease) {
  i32 count = 0;
  try {
    scope_fail guard([&count] { ++count; });
    guard.release();
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 0);
}

TEST(scope, scopeFailDuringUnwinding) {
  // A guard created during stack unwinding must not fire if its own scope exits normally
  i32 count = 0;
  try {
    const OnDestroy onDestroy([&count] {
      const scope_fail guard([&count] { ++count; });
    });
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 0);

  // ... but it must fire if its own scope is left by a new exception
  try {
    const OnDestroy onDestroy([&count] {
      try {
        const scope_fail guard([&count] { ++count; });
        throw logic_error("nested");
      } catch (const logic_error&) {}
    });
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 1);
}

TEST(scope, scopeFailMacro) {
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

// `scope_success` ------------------------------------------------------------------------------------------

TEST(scope, scopeSuccessRunsOnNormalExit) {
  i32 count = 0;
  {
    const scope_success guard([&count] { ++count; });
    EXPECT_EQ(count, 0);
  }
  EXPECT_EQ(count, 1);
}

TEST(scope, scopeSuccessDoesNotRunOnException) {
  i32 count = 0;
  try {
    const scope_success guard([&count] { ++count; });
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 0);
}

TEST(scope, scopeSuccessRunsAfterCaughtException) {
  i32 count = 0;
  {
    const scope_success guard([&count] { ++count; });
    try {
      throw runtime_error("test");
    } catch (const runtime_error&) {}
  }
  EXPECT_EQ(count, 1);
}

TEST(scope, scopeSuccessRelease) {
  i32 count = 0;
  {
    scope_success guard([&count] { ++count; });
    guard.release();
  }
  EXPECT_EQ(count, 0);
}

TEST(scope, scopeSuccessDuringUnwinding) {
  // A guard created during stack unwinding must fire if its own scope exits normally
  i32 count = 0;
  try {
    const OnDestroy onDestroy([&count] {
      const scope_success guard([&count] { ++count; });
    });
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 1);

  // ... but it must not fire if its own scope is left by a new exception
  try {
    const OnDestroy onDestroy([&count] {
      try {
        const scope_success guard([&count] { ++count; });
        throw logic_error("nested");
      } catch (const logic_error&) {}
    });
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 1);
}

TEST(scope, scopeSuccessMacro) {
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

// Move construction ----------------------------------------------------------------------------------------

TEST(scope, moveTransfersOwnership) {
  i32 count = 0;
  {
    scope_exit a([&count] { ++count; });
    const scope_exit b(std::move(a));
  }
  EXPECT_EQ(count, 1);

  count = 0;
  {
    scope_success a([&count] { ++count; });
    const scope_success b(std::move(a));
  }
  EXPECT_EQ(count, 1);

  count = 0;
  try {
    scope_fail a([&count] { ++count; });
    const scope_fail b(std::move(a));
    throw runtime_error("test");
  } catch (const runtime_error&) {}
  EXPECT_EQ(count, 1);
}

// EOF
