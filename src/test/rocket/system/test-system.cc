/*
 * test-system.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/log/log.h"
#include "rocket/system/system.h"

using namespace rocket::system;
using namespace std::filesystem;

// Constants ------------------------------------------------------------------------------------------------

const auto CURRENT_BINARY_DIR = env::get<string>("CURRENT_BINARY_DIR");

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(system, envBool) {
  using type = bool;

  const char* name = "MY_BOOL";

  EXPECT_EQ(env::get<log::LogLevel>(name), nullopt);

  env::set(name, true);
  // Check twice, there was a bad surprise with `putenv()` ...
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
  // Check twice, there was a bad surprise with `putenv()` ...
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
  // Check twice, there was a bad surprise with `putenv()` ...
  EXPECT_EQ(env::get<type>(name), type::debug);
  EXPECT_EQ(env::get<type>(name), type::debug);

  env::unset(name);
  EXPECT_EQ(env::get<type>(name), nullopt);
}

TEST(system, envStringView) {
  using type = string_view;

  const char* name = "MY_STRING_VIEW";

  EXPECT_EQ(env::get<type>(name), nullopt);

  env::set(name, "some text"sv);
  // Check twice, there was a bad surprise with `putenv()` ...
  EXPECT_EQ(env::get<type>(name), "some text");
  EXPECT_EQ(env::get<type>(name), "some text");

  env::unset(name);
  EXPECT_EQ(env::get<type>(name), nullopt);
}

TEST(system, envGet) {
  env::set("MY_ENV1", "");
  env::set("MY_ENV2", "value");
  auto env = env::get();
  EXPECT_EQ(env.at("MY_ENV1"), "");
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
  path mainBinaryDir = path(*CURRENT_BINARY_DIR).parent_path() / "main";
  path printArgs = mainBinaryDir / fmt::format("print-args{}", executableSuffix());
  string executable = printArgs.string();

  auto bytes = exec({ executable, "a" });
  string_view out(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  EXPECT_THAT(out, HasSubstr("1=a="));
}

TEST(system, execPrintArgsWithSpace) {
  // Copy `print-args` to `print args`
  path mainBinaryDir = path(*CURRENT_BINARY_DIR).parent_path() / "main";
  path printArgs = mainBinaryDir / fmt::format("print-args{}", executableSuffix());
  path printArgsWithSpace = mainBinaryDir / fmt::format("print args{}", executableSuffix());
  if (not exists(printArgsWithSpace)) {
    copy(printArgs, printArgsWithSpace);
  }
  string executable = printArgsWithSpace.string();

  {
    // Test spaces in executable name and in arguments
    auto bytes = exec({ executable, "a", "b c", "d\\\"", "'hi'", "some\\text" });
    string_view out(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    EXPECT_THAT(out, AllOf(
        HasSubstr("1=a="),
        HasSubstr("2=b c="),
        HasSubstr("3=d\\\"="),
        HasSubstr("4='hi'="),
        HasSubstr("5=some\\text=")));
  }

  {
    // Test Unicode
    auto bytes = exec({ executable, "ä", "€" });
    string_view out(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    EXPECT_THAT(out, AllOf(
        HasSubstr("1=ä="),
        HasSubstr("2=€=")));
    }
}

// EOF
