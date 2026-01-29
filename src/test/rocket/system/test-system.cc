/*
 * test-system.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/log/log.h"
#include "rocket/system/system.h"

using namespace rocket::system;
using namespace std;
using namespace std::filesystem;

// Constants ------------------------------------------------------------------------------------------------

const auto BINARY_DIR = env::get<string>("BINARY_DIR");

// Functions ------------------------------------------------------------------------------------------------

path
findPrintArgs() {
  path binaryDir = path(*BINARY_DIR);
  string name = fmt::format("print-args{}", executableSuffix());
  path test = binaryDir / name;
  if (is_regular_file(test)) {
    return test;
  }
  for (const auto config : { "Release", "Debug" }) {
    test = binaryDir / config / name;
    if (is_regular_file(test)) {
      return test;
    }
  }
  ROCKET_FAIL("Cannot find `print-args`");
}

// #TEST ----------------------------------------------------------------------------------------------------

TEST(system, envBool) {
  using type = bool;

  const char* name = "MY_BOOL";

  EXPECT_EQ(env::get<log::LogLevel>(name), nullopt);

  env::set(name, true);
  // Check twice, there was a bad surprise with #std::putenv ...
  EXPECT_EQ(env::get<type>(name), true);
  EXPECT_EQ(env::get<type>(name), true);

  env::unset(name);
  EXPECT_EQ(env::get<type>(name), nullopt);

  env::set(name, "0");
  EXPECT_EQ(env::get<type>(name), false);
  env::set(name, "1");
  EXPECT_EQ(env::get<type>(name), true);
  env::set(name, "foo");
  EXPECT_EQ(env::get<type>(name), true);
}

TEST(system, envF64) {
  using type = f64;

  const char* name = "MY_F64";

  EXPECT_EQ(env::get<type>(name), nullopt);

  env::set(name, -1.2);
  // Check twice, there was a bad surprise with #std::putenv ...
  EXPECT_EQ(env::get<type>(name), -1.2);
  EXPECT_EQ(env::get<type>(name), -1.2);

  env::unset(name);
  EXPECT_EQ(env::get<type>(name), nullopt);

  env::set(name, -1.3);
  EXPECT_EQ(env::get<type>(name), -1.3);
}

TEST(system, envLogLevel) {
  using type = log::LogLevel;

  const char* name = "MY_LOG_LEVEL";

  EXPECT_EQ(env::get<type>(name), nullopt);

  env::set(name, type::debug);
  // Check twice, there was a bad surprise with #std::putenv ...
  EXPECT_EQ(env::get<type>(name), type::debug);
  EXPECT_EQ(env::get<type>(name), type::debug);

  env::unset(name);
  EXPECT_EQ(env::get<type>(name), nullopt);
}

TEST(system, envString) {
  using type = string;

  const char* name = "MY_STRING_VIEW";

  EXPECT_EQ(env::get<type>(name), nullopt);

  env::set(name, "some text"sv);
  // Check twice, there was a bad surprise with #std::putenv ...
  EXPECT_EQ(env::get<type>(name), "some text");
  EXPECT_EQ(env::get<type>(name), "some text");

  env::unset(name);
  EXPECT_EQ(env::get<type>(name), nullopt);
}

TEST(system, envGet) {
  env::set("MY_ENV1", "");
  env::set("MY_ENV2", "value");
  auto env = env::get();
#ifdef ROCKET_OS_WINDOWS
  EXPECT_FALSE(env.contains("MY_ENV1"));
#else
  EXPECT_EQ(env.at("MY_ENV1"), "");
#endif
  EXPECT_EQ(env.at("MY_ENV2"), "value");
}

TEST(system, execEcho) {
  auto bytes = exec("echo Hello");
  string_view out(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  EXPECT_EQ(out, "Hello\n");
}

TEST(system, execPrintf) {
  auto bytes = exec("printf \"%s\" Hello");
  string_view out(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  EXPECT_EQ(out, "Hello");
}

TEST(system, execPrintArgs) {
  path printArgs = findPrintArgs();
  string executable = printArgs.string();

  {
    // Test spaces and quotes
    auto bytes = exec({ executable, "a", "b c", " d ", "'", "\"" });
    string_view out(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    EXPECT_THAT(out, HasSubstr("1=a="));
    EXPECT_THAT(out, HasSubstr("2=b c="));
    EXPECT_THAT(out, HasSubstr("3= d ="));
    EXPECT_THAT(out, HasSubstr("4='="));
    EXPECT_THAT(out, HasSubstr("5=\"="));
  }

  {
    // Test Unicode
    auto bytes = exec({ executable, "€ ÄÖÜ € 🧑‍🌾" });
    string_view out(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    EXPECT_THAT(out, HasSubstr("1=€ ÄÖÜ € 🧑‍🌾="));
  }
}

TEST(system, execPrintArgsWithSpace) {
  // Copy `print-args` to `print args`, se we have a space in the executable name
  path printArgs = findPrintArgs();
  path printArgsWithSpace = printArgs.parent_path() / fmt::format("print args{}", executableSuffix());
  copy_file(printArgs, printArgsWithSpace, copy_options::overwrite_existing);
  string executable = printArgsWithSpace.string();

  {
    // Test spaces and quotes
    auto bytes = exec({ executable, "a", "b c", " d ", "'", "\"" });
    string_view out(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    EXPECT_THAT(out, HasSubstr("1=a="));
    EXPECT_THAT(out, HasSubstr("2=b c="));
    EXPECT_THAT(out, HasSubstr("3= d ="));
    EXPECT_THAT(out, HasSubstr("4='="));
    EXPECT_THAT(out, HasSubstr("5=\"="));
  }

  {
    // Test Unicode
    auto bytes = exec({ executable, "€ ÄÖÜ € 🧑‍🌾" });
    string_view out(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    EXPECT_THAT(out, HasSubstr("1=€ ÄÖÜ € 🧑‍🌾="));
  }
}

// EOF
