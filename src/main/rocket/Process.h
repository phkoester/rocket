/**
 * @file Process.h
 *
 * The central Rocket process class.
 */

#pragma once

#include "nio.h"

#include <locale>
#include <optional>
#include <string>
#include <vector>

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/facilities/check_empty.hpp>
#include <boost/preprocessor/logical/not.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Calls #rocket::Process::error. This may be used even if the process isn't initialized yet.
 *
 * Usage: `ROCKET_PROCESS_ERROR(fmt, [args]...])`
 */
#define ROCKET_PROCESS_ERROR(fmt, ...) { \
  ::std::string msg; \
  ::rocket::nio::StringSink sink(msg); \
  sink.print("{}:{}: ", __FILE__, __LINE__); \
  sink.print( \
      fmt \
      BOOST_PP_COMMA_IF(BOOST_PP_NOT(BOOST_PP_CHECK_EMPTY(BOOST_PP_TUPLE_ELEM(0, (__VA_ARGS__))))) \
      __VA_ARGS__); \
  ::rocket::process.error(::rocket::nio::stderr, EXIT_SUCCESS, "{}", msg, EXIT_SUCCESS); \
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
 * #include <rocket/cl.h>
 * #include <rocket/nio.h>
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
   * Registers a function to be called upon exit.
   *
   * @param f the function to register
   */
  void atExit(void (*f)()) const;

  /**
   * Returns the classic locale. This is the locale as returned by `std::locale::classic`.
   *
   * @return a locale
   */
  const std::locale& classicLocale() const { return classicLocale_; }

  /**
   * Returns the code locale used for logging and error messages, which is `en_US.UTF-8`.
   *
   * On Linux, the locale may be configured like this:
   *
   * ```bash
   * sudo locale-gen en_US.UTF-8
   * sudo dpkg-reconfigure locales
   * ```

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

    if (status != EXIT_SUCCESS)
      exit(status);
  }

  /**
   * Exits the program.
   *
   * Depending on how #init was parametrized, this either calls `std::quick_exit` or `std::exit`.
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
   * @param quickExit if `true`, #exit calls `std::quick_exit`, otherwise it calls `std::exit`. It is
   *    recommended to set this to `true` for faster process tear-down
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
