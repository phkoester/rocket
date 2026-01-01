/*
 * test-system.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/log/log.h"
#include "rocket/system/system.h"

using namespace rocket;
using namespace rocket::system;
using namespace std;
using namespace testing;

// Constants ------------------------------------------------------------------------------------------------

const string BUILD_DIR = getenv("BUILD_DIR");
const string PRINT_ARGS = BUILD_DIR + "/print-args";
const string PRINT_ARGS_WITH_SPACE = BUILD_DIR + "/print args";

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(system, env_bool) {
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
  EXPECT_EQ(env::get<type>(name), nullopt);
}

TEST(system, env_double) {
  using type = double;

  const char* name = "MY_DOUBLE";

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

TEST(system, env_LogLevel) {
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

TEST(system, env_string_view) {
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
  auto bytes = exec({ PRINT_ARGS, "a" });
  string_view out(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  EXPECT_THAT(out, HasSubstr("1=a="));
}

TEST(system, execPrintArgsWithSpace) {
  auto bytes = exec({ PRINT_ARGS_WITH_SPACE, "a", "b c", "d\\\"", "'hi'", "some\\text" });
  string_view out(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  EXPECT_THAT(out, AllOf(
      HasSubstr("1=a="),
      HasSubstr("2=b c="),
      HasSubstr("3=d\\\"="),
      HasSubstr("4='hi'="),
      HasSubstr("5=some\\text=")));
}

TEST(system, execPrintArgsWithSpaceUnicode) {
  auto bytes = exec({ PRINT_ARGS_WITH_SPACE, "ä", "€" });
  string_view out(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  EXPECT_THAT(out, AllOf(
      HasSubstr("1=ä="),
      HasSubstr("2=€=")));
}

// EOF
