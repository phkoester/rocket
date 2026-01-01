/**
 * @file Process.h
 *
 * The central Rocket process class.
 */

#pragma once

#include "rocket/macro.h"
#include "rocket/nio/nio.h"

#include <locale>
#include <optional>
#include <string>
#include <vector>

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Calls #rocket::Process::error.
 *
 * This may be used even if the process isn't initialized yet.
 *
 * Usage: `ROCKET_PROCESS_ERROR(fmt, [args]...])`
 */
#define ROCKET_PROCESS_ERROR(fmt, ...) { \
  ::rocket::nio::StringSink msg; \
  msg.print("{}:{}: ", __FILE__, __LINE__); \
  msg.print( \
      fmt \
      ROCKET_COMMA_AND_VA_ARGS(__VA_ARGS__)); \
  ::rocket::process.error(::rocket::nio::stderr, EXIT_SUCCESS, "{}", msg.str()); \
}

/**
 * Calls #rocket::Process::warn.
 *
 * This may be used even if the process isn't initialized yet.
 *
 * Usage: `ROCKET_PROCESS_WARN(fmt, [args]...])`
 */
 #define ROCKET_PROCESS_WARN(fmt, ...) { \
  ::rocket::nio::StringSink msg; \
  msg.print("{}:{}: ", __FILE__, __LINE__); \
  msg.print( \
      fmt \
      ROCKET_COMMA_AND_VA_ARGS(__VA_ARGS__)); \
  ::rocket::process.warn(::rocket::nio::stderr, "{}", msg.str()); \
}

namespace rocket {

// Constants ------------------------------------------------------------------------------------------------

/**
 * An exit value that may be used when the program cannot start, e.g. due to a a bad command line, a bad
 * environment or missing ressources.
 *
 * Once the program started operating properly, `EXIT_FAILURE` (1) should be used to indicate a problem.
 */
constexpr int EXIT_SERIOUS_FAILURE = 2;

// `Process` ------------------------------------------------------------------------------------------------

/**
 * A central Rocket class to be used in `main`.
 *
 * ## Examples
 *
 * A very basic Rocket program may look like this:
 *
 * ```
 * #include <rocket/Process.h>
 * #include <rocket/cl/cl.h>
 * #include <rocket/nio/nio.h>
 *
 * using namespace rocket;
 * using namespace std;
 *
 * int
 * main(int argc, char** argv) {
 *   try {
 *     process.init(argc, argv, "my-program");
 *     cl::CommandLine cl;
 *     try {
 *       cl.parse(process.args());
 *     } catch (const exception& ex) {
 *       cl.handleException(ex, nio::stderr);
 *     }
 *     nio::stdout.println("This is {}", process.name());
 *     process.exit(EXIT_SUCCESS);
 *   } catch (...) {
 *     terminate();
 *   }
 * }
 * ```
 */
struct Process {
  /**
   * Returns the command line.
   *
   * @return the command line, as separate strings. `argv[0]` is not included
   */
  const std::vector<std::string>& args() const { return args_; }

  /**
   * Registers a function to be called upon exit, and even on abnormal termination.
   *
   * This function may be called even if the process isn't initialized yet.
   *
   * @param f the function to register
   */
  void atExit(std::function<void()> f);

  /**
   * Returns the classic locale. This is the locale as returned by `std::locale::classic`.
   *
   * @return a locale
   */
  const std::locale& classicLocale() const { return classicLocale_; }

  /**
   * Returns the code locale used for logging and error messages, which is `en_US.UTF-8`.
   *
   * This is also the default locale for unit tests and benchmarks.
   *
   * On Linux, the locale may be configured like this:
   *
   * ```bash
   * sudo locale-gen en_US.UTF-8
   * sudo dpkg-reconfigure locales
   * ```
   *
   * @return a locale
   */
  const std::locale& codeLocale() const { return codeLocale_; }

  /**
   * Outputs an error message.
   *
   * This function may be called even if the process isn't initialized yet.
   *
   * @param sink the sink to write to, usually `rocket::nio::stderr`
   * @param status the exit status. If not `EXIT_SUCCESS` (0), then #exit is called
   * @param fmt the format string
   * @param args the format arguments
   */
  template<typename... T>
  void error(nio::Sink& sink, int status, fmt::format_string<T...> fmt, T&&... args) {
    std::string name = inited_ ? this->name() : invocationShortName();
    sink.print("{}: error: ", name);
    sink.println(fmt, std::forward<T>(args)...);

    if (status != EXIT_SUCCESS) {
      exit(status);
    }
  }

  /**
   * Exits the program.
   *
   * Depending on how #init was parametrized, this either calls `std::exit` or `std::quick_exit`.
   *
   * @param status the exit status
   */
  [[noreturn]] void exit(int status) const;

  /**
   * Initializes the process.
   *
   * @attention This function must be called by any Rocket program; see the example in the documentation of
   * this class.
   *
   * @param argc `argc` from `main`
   * @param argv `argv` from `main`
   * @param name the name of the program. If this is null, the file name from `argv[0]` is used. It is
   *    recommended to give the program a proper name independent from `argv[0]`
   * @param locale the locale to use for the process. If this is null, the locale from the environment is
   *    used
   * @param quickExit if `true`, #exit calls `std::quick_exit`, otherwise it calls `std::exit`
   */
  void init(
      int argc,
      char** argv,
      std::optional<std::string_view> name = std::nullopt,
      std::optional<std::locale> locale = std::nullopt,
      bool quickExit = true);

  /**
   * Returns the locale this process picked in #init.
   *
   * @return a locale
   */
  const std::locale& initLocale() const { return initLocale_; }

  /**
   * Returns the name of the process.
   *
   * @return the name of the process
   */
  const std::string& name() const;

  /**
   * Returns the command this program was started with.
   *
   * @return the invocation name
   */
  const std::string& invocationName() const;

  /**
   * Returns the file-name portion of the command this program was started with.
   *
   * @return the invocation short name
   */
  const std::string& invocationShortName() const;

  /**
   * Returns the system locale. This is the locale that was returned by the first call of
   * `std::locale::global`.
   *
   * @return a locale
   */
  const std::locale& systemLocale() const { return systemLocale_; }

  /**
   * Outputs a warning.
   *
   * This function may be called even if the process isn't initialized yet.
   *
   * @param sink the sink to write to, usually `rocket::nio::stderr`
   * @param fmt the format string
   * @param args the format arguments
   */
  template<typename... T>
  void warn(nio::Sink& sink, fmt::format_string<T...> fmt, T&&... args) {
    std::string name = inited_ ? this->name() : invocationShortName();
    sink.print("{}: warning: ", name);
    sink.println(fmt, std::forward<T>(args)...);
  }

private:

  int argc_ = 0;
  char** argv_ = nullptr;
  std::string name_;
  bool quickExit_ = true;

  std::vector<std::string> args_;
  bool inited_ = false;

  const std::locale classicLocale_ = std::locale::classic();
  const std::locale codeLocale_ = std::locale("en_US.UTF-8");
  std::locale initLocale_;
  std::locale systemLocale_;
};

/**
 * The Process singleton.
 */
extern Process process;

} // namespace rocket

// EOF
