/**
 * @file Process.h
 *
 * The central Rocket process class.
 */

#pragma once

#include "rocket/rocket.h"
#include "rocket/nio/nio.h"

#include <locale>
#include <optional>
#include <string>
#include <thread>
#include <vector>

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Calls #rocket::Process::error.
 *
 * This may be used even if the process isn't initialized yet.
 *
 * Usage: `ROCKET_PROCESS_ERROR(status, fmt, [args]...])`
 */
#define ROCKET_PROCESS_ERROR(status, fmt, ...) { \
  ::rocket::nio::StringSink msg; \
  msg.print("{}:{}: ", ROCKET_SRC_FILE, __LINE__); \
  msg.print( \
      fmt \
      __VA_OPT__(,) __VA_ARGS__); \
  ::rocket::process.error(::rocket::nio::err, status, "{}", msg.str()); \
}

/**
 * Calls #rocket::Process::info.
 *
 * This may be used even if the process isn't initialized yet.
 *
 * Usage: `ROCKET_PROCESS_INFO(fmt, [args]...])`
 */
 #define ROCKET_PROCESS_INFO(fmt, ...) { \
  ::rocket::nio::StringSink msg; \
  msg.print("{}:{}: ", ROCKET_SRC_FILE, __LINE__); \
  msg.print( \
      fmt \
      __VA_OPT__(,) __VA_ARGS__); \
  ::rocket::process.info(::rocket::nio::out, "{}", msg.str()); \
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
  msg.print("{}:{}: ", ROCKET_SRC_FILE, __LINE__); \
  msg.print( \
      fmt \
      __VA_OPT__(,) __VA_ARGS__); \
  ::rocket::process.warn(::rocket::nio::err, "{}", msg.str()); \
}

/**
 * Sets or returns the name of the current thread.
 *
 * The rocket logging API considers the thread name for logging.
 *
 * Usage: `ROCKET_THREAD_NAME([name])`
 *
 * @return the current thread name
 */
#define ROCKET_THREAD_NAME(...) ::rocket::internal::setThreadName(__VA_ARGS__)

namespace rocket {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

const std::string& setThreadName(std::string_view name = "");

} // namespace internal

// Constants ------------------------------------------------------------------------------------------------

/**
 * An exit value that may be used when the program cannot start, e.g. due to a a bad command line, a bad
 * environment or missing ressources.
 *
 * Once the program started operating properly, `EXIT_FAILURE` (1) may be used to indicate a problem.
 */
constexpr i32 EXIT_SERIOUS_FAILURE = 2;

/// The ID of the main thread.
extern const std::thread::id MAIN_THREAD_ID;

// #Process -------------------------------------------------------------------------------------------------

/**
 * A central Rocket class to be used in `main`.
 *
 * You don't ever create a #Process object. To access the singleton, use #rocket::process.
 *
 * @ThreadSafe
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
 *
 * i32
 * main(i32 argc, char** argv) {
 *   process.init(argc, argv, "my-program");
 *
 *   cl::CommandLine cl;
 *   cl.parse(process.args());
 *
 *   nio::out.println("This is {}", process.name());
 *
 *   process.exit(EXIT_SUCCESS);
 * }
 * ```
 */
struct Process {
  /**
   * Registers a function to be called upon exit and quick exit, and even possibly on abnormal termination.
   *
   * @param fn the function to register
   * @param callOnTerminate whether to call the function even on abnormal termination
   */
  static void atExit(std::function<void()>&& fn, bool callOnTerminate = false);

  /**
   * Returns the command this program was started with.
   *
   * @return the invocation name
   */
  [[nodiscard]] static const std::string& invocationName();

  /**
   * Returns the file-name portion of the command this program was started with.
   *
   * @return the invocation short name
   */
  [[nodiscard]] static const std::string& invocationShortName();

  /**
   * Returns the command line.
   *
   * Must be called after #init.
   *
   * @return the command line, as separate strings. `argv[0]` is not included
   */
  [[nodiscard]] const std::vector<std::string>& args() const { return args_; }

  /**
   * Returns the classic locale. This is the locale as returned by #std::locale::classic.
   *
   * Must be called after #init.
   *
   * @return a locale
   */
  [[nodiscard]] const std::locale& classicLocale() const { return classicLocale_; }

  /**
   * Returns the code locale used for logging and error messages, which is `en_US.UTF-8`.
   *
   * This is also the default locale for unit tests and benchmarks.
   *
   * Must be called after #init.
   *
   * On Linux, the locale may be configured like this:
   *
   * ```bash
   * $ sudo locale-gen en_US.UTF-8
   * ```
   *
   * @return a locale
   */
  [[nodiscard]] const std::locale& codeLocale() const { return codeLocale_; }

  /**
   * Outputs an error message.
   *
   * May be called before #init.
   *
   * @param out the sink to write to, usually `rocket::nio::err`
   * @param status the exit status. If not 0, a fatal error is issued and #exit is called
   * @param fmt the format string
   * @param args the format arguments
   */
  template<typename... T>
  void
  error(nio::Sink& out, i32 status, fmt::format_string<T...> fmt, T&&... args) const { // NOLINT(*-recursion)
    out.write(autoName());
    out.write(": ");
    const char* label = status != 0 ? "fatal error: " : "error: ";
    if (isatty(out.handle())) {
      out.print(fg(fmt::color::red) | fmt::emphasis::bold, "{}", label);
    } else {
      out.write(label);
    }
    out.println(fmt, std::forward<T>(args)...);

    if (status != 0) {
      exit(status, true);
    }
  }

  /**
   * Exits the program.
   *
   * Must be called after #init.
   *
   * Depending on how #init was parametrized, this function either calls #std::exit or #std::quick_exit.
   * The environment variables `ROCKET_EXIT` and `ROCKET_QUICK_EXIT` overrule the parametrized setting.
   *
   * @param status the exit status
   */
  [[noreturn]] void exit(i32 status) const { exit(status, false); }

  /**
   * Outputs an information message.
   *
   * May be called before #init.
   *
   * @param out the sink to write to, usually `rocket::nio::err`
   * @param fmt the format string
   * @param args the format arguments
   */
  template<typename... T>
  void
  info(nio::Sink& out, fmt::format_string<T...> fmt, T&&... args) const {
    out.write(autoName());
    out.write(": ");
    if (isatty(out.handle())) {
      out.print(fg(fmt::color::cyan) | fmt::emphasis::bold, "note: ");
    } else {
      out.write("note: ");
    }
    out.println(fmt, std::forward<T>(args)...);
  }

  /**
   * Initializes the process.
   *
   * @attention This function must be called by any Rocket program; see the example in the documentation of
   * this class.
   *
   * @MainThread
   *
   * @param argc `argc` from `main`
   * @param argv `argv` from `main`
   * @param name the name of the program. If this is null, the file name from `argv[0]` is used. It is
   *    recommended to give the program a proper name independent from `argv[0]`
   * @param locale the locale to use for the process. If this is null, the locale from the environment is
   *    used
   * @param quickExit if `true`, #exit calls #std::quick_exit, otherwise it calls #std::exit
   */
  void init(
      i32 argc,
      char** argv,
      std::optional<std::string_view> name = std::nullopt,
      std::optional<std::locale> locale = std::nullopt,
      bool quickExit = true) const;

  /**
   * Returns the locale this process picked in #init.
   *
   * Must be called after #init.
   *
   * @return a locale
   */
  [[nodiscard]] const std::locale& initLocale() const { return initLocale_; }

  /**
   * Returns the name of the process.
   *
   * Must be called after #init.
   *
   * @return the name of the process
   */
  [[nodiscard]] const std::string& name() const;

  /**
   * Returns the system locale. This is the locale that was returned by the first call of
   * #std::locale::global.
   *
   * Must be called after #init.
   *
   * @return a locale
   */
  [[nodiscard]] const std::locale& systemLocale() const { return systemLocale_; }

  /**
   * Outputs a warning.
   *
   * May be called before #init.
   *
   * @param out the sink to write to, usually `rocket::nio::err`
   * @param fmt the format string
   * @param args the format arguments
   */
  template<typename... T>
  void
  warn(nio::Sink& out, fmt::format_string<T...> fmt, T&&... args) const {
    out.write(autoName());
    out.write(": ");
    if (isatty(out.handle())) {
      out.print(fg(fmt::color::yellow) | fmt::emphasis::bold, "warning: ");
    } else {
      out.write("warning: ");
    }
    out.println(fmt, std::forward<T>(args)...);
  }

private:

  mutable i32 argc_ = 0;
  mutable char** argv_ = nullptr;
  mutable std::string name_;
  mutable bool quickExit_ = true;

  mutable std::vector<std::string> args_;
  mutable bool inited_ = false;

  std::locale classicLocale_ = std::locale::classic();
  std::locale codeLocale_ = std::locale("en_US.UTF-8");
  mutable std::locale initLocale_;
  mutable std::locale systemLocale_;

  Process() = default;

  [[nodiscard]] const std::string& autoName() const;

  [[noreturn]] void exit(i32 status, bool allowUninited) const;

  friend Process makeProcess__();
};

/// The #rocket::Process singleton.
ROCKET_PUBLIC extern const Process process;

} // namespace rocket

// EOF
