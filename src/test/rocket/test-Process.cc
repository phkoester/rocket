/*
 * test-Process.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Process.h"
#include "rocket/scope.h"
#include "rocket/nio/nio.h"
#include "rocket/system/system.h"

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(Process, error0) {
  nio::StringSink buf;
  process().error(buf, 0, "Test error");
  EXPECT_EQ(buf.str(), "test-rocket-Process: error: Test error\n");
}

TEST(ProcessDeathTest, error2) {
  EXPECT_EXIT(
    { process().error(nio::err, 2, "Test error"); },
    testing::ExitedWithCode(2), "test-rocket-Process: fatal error: Test error\n");
}

TEST(ProcessDeathTest, exit) {
  EXPECT_EXIT(
    { process().error(nio::err, 7, "Exiting ..."); },
    testing::ExitedWithCode(7), "test-rocket-Process: fatal error: Exiting ...\n");
}

TEST(Process, tryLocale) {
  namespace impl = rocket::internal::process;

  EXPECT_TRUE(impl::tryLocale("C").has_value());
  EXPECT_FALSE(impl::tryLocale("xx_XX.bogus").has_value());
}

// `Process::init` delegates to this function. It cannot be tested through `init` itself, not even in a death
// test, because `main` has already called `init` by then and `init` may only be called once
TEST(Process, localeFromEnvironment) {
  namespace impl = rocket::internal::process;

  const auto oldLcAll = system::env::get<string>("LC_ALL");
  ROCKET_SCOPE_EXIT([&] {
    if (oldLcAll) {
      system::env::set("LC_ALL", *oldLcAll);
    } else {
      system::env::unset("LC_ALL");
    }
  });

  // An available locale causes no warning
  {
    system::env::set("LC_ALL", "C");
    nio::StringSink buf;
    impl::localeFromEnvironment(buf);
    EXPECT_EQ(buf.str(), "");
  }

#ifndef ROCKET_OS_WINDOWS // On Windows, the C++ runtime does not take the locale from `LC_ALL`
  // An unavailable locale causes a warning and the fallback, but no exception
  {
    system::env::set("LC_ALL", "xx_XX.bogus");
    nio::StringSink buf;
    const std::locale loc = impl::localeFromEnvironment(buf);
    EXPECT_TRUE(loc == std::locale::classic());
    EXPECT_EQ(
      buf.str(),
      "test-rocket-Process: warning: Cannot use the locale from the environment; "
      "using the classic locale instead\n");
  }
#endif
}

// EOF
