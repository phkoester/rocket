/**
 * @file Process.h
 *
 * The central Rocket process class.
 */

#pragma once

#include <iosfwd>
#include <locale>
#include <optional>
#include <string>
#include <vector>

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Calls #rocket::Process::error. This may be used even if the process isn't initialized yet.
 *
 * @param msg The error message
 */
#define ROCKET_PROCESS_ERROR(msg) { \
  ::std::ostringstream os; \
  os << __FILE__ << ':' << __LINE__ << ": " << msg; \
  ::rocket::process.error(::std::cerr, os.str(), EXIT_SUCCESS); \
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
 *
 * #include <iostream>
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
 *       cl.handleException(ex);
 *     }
 *     cout << "This is " << process.name() << '\n';
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
   * Outputs an error message.
   *
   * This function may be called even if the process isn't initialized yet.
   *
   * @param os the output stream, usually `std::cerr`
   * @param msg the error messsage
   * @param status the exit status. If not `EXIT_SUCCESS` (0), then #exit is called
   */
  void error(std::ostream& os, std::string_view msg, int status = EXIT_FAILURE) const;

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
      int argc, char** argv, std::optional<std::string_view> name = std::nullopt, bool quickExit = true);

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
   * Outputs a warning.
   *
   * This function may be called even if the process isn't initialized yet.
   
   * @param os the output stream, usually `std::cerr`
   * @param msg the warning message
   */
  void warn(std::ostream& os, std::string_view msg) const;

private:

  int argc_ = 0;
  char** argv_ = nullptr;
  std::string name_;
  bool quickExit_ = true;

  std::vector<std::string> args_;
  bool inited_ = false;
  std::locale oldLocale_;
};

/**
 * The Process singleton.
 */
extern Process process;

} // namespace rocket

// EOF
