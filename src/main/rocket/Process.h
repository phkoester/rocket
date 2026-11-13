/**
 * @file Process.h
 *
 * The central Rocket process class.
 */

#pragma once

#include "rocket/io/io.h"
#include "rocket/nio/nio.h"

#include <filesystem>
#include <functional>
#include <locale>
#include <optional>
#include <string>
#include <thread>
#include <vector>

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Calls #rocket::Process::error.
 *
 * This may be used even if the process isn’t initialized yet.
 *
 * Usage: `ROCKET_PROCESS_ERROR(status, fmt, [args]...])`
 */
#define ROCKET_PROCESS_ERROR(status, fmt, ...) { \
  ::rocket::nio::StringSink msg; \
  msg.print("{}:{}: ", ROCKET_SRC_FILE, __LINE__); \
  msg.print(fmt __VA_OPT__(,) __VA_ARGS__); \
  ::rocket::process().error(::rocket::nio::err, status, "{}", msg.str()); \
}

/**
 * Calls #rocket::Process::info.
 *
 * This may be used even if the process isn’t initialized yet.
 *
 * Usage: `ROCKET_PROCESS_INFO(fmt, [args]...])`
 */
 #define ROCKET_PROCESS_INFO(fmt, ...) { \
  ::rocket::nio::StringSink msg; \
  msg.print("{}:{}: ", ROCKET_SRC_FILE, __LINE__); \
  msg.print(fmt __VA_OPT__(,) __VA_ARGS__); \
  ::rocket::process().info(::rocket::nio::out, "{}", msg.str()); \
}

/**
 * Calls #rocket::Process::warn.
 *
 * This may be used even if the process isn’t initialized yet.
 *
 * Usage: `ROCKET_PROCESS_WARN(fmt, [args]...])`
 */
 #define ROCKET_PROCESS_WARN(fmt, ...) { \
  ::rocket::nio::StringSink msg; \
  msg.print("{}:{}: ", ROCKET_SRC_FILE, __LINE__); \
  msg.print(fmt __VA_OPT__(,) __VA_ARGS__); \
  ::rocket::process().warn(::rocket::nio::err, "{}", msg.str()); \
}

/**
 * Sets or returns the name of the current thread.
 *
 * The Rocket logging API considers the thread name for logging.
 *
 * Usage: `ROCKET_THREAD_NAME([name])`
 *
 * @return the current thread name
 */
#define ROCKET_THREAD_NAME(...) ::rocket::internal::process::setThreadName(__VA_ARGS__)

namespace rocket {

// Internal -------------------------------------------------------------------------------------------------

namespace internal::process {

/**
 * Returns the locale selected by the environment.
 *
 * The environment may name a locale that is not installed, which must not keep a program from starting.
 *
 * @param err the sink to write a warning to if the locale is not available
 * @return the locale from the environment, or the classic locale if it is not available
 */
std::locale localeFromEnvironment(nio::Sink& err);

const std::string& setThreadName(std::string_view name = "");

/**
 * Creates a locale by name.
 *
 * @param name the name of the locale
 * @return the locale, or null if it is not available on this system
 */
std::optional<std::locale> tryLocale(const std::string& name);

} // namespace internal::process

// Constants ------------------------------------------------------------------------------------------------

/**
 * An exit value that may be used when the program cannot start, e.g., due to a bad command line, a bad
 * environment or missing resources.
 *
 * Once the program started operating properly, `EXIT_FAILURE` (1) may be used to indicate a problem.
 */
constexpr i32 EXIT_SERIOUS_FAILURE = 2;

/// The ID of the main thread.
extern const std::thread::id MAIN_THREAD_ID;

// `Process` ------------------------------------------------------------------------------------------------

/**
 * A central Rocket class to be used in `main`.
 *
 * You don’t ever create a #Process object. To access the singleton, use #rocket::process.
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
 *   process().init(argc, argv, "my-program");
 *   cl::CommandLine cl;
 *   cl.parse(process().args());
 *   nio::out.println("This is {}", process().name());
 *   process().exit(EXIT_SUCCESS);
 * }
 * ```
 */
struct Process {
  /**
   * Returns the command line.
   *
   * Must be called after #init.
   *
   * @return the command line, as separate strings. `argv[0]` is not included
   */
  [[nodiscard]] const std::vector<std::string>& args() const;

  /**
   * Registers a function to be called upon exit and quick exit, and even possibly on abnormal termination.
   *
   * May be called before #init.
   *
   * @param fn the function to register
   * @param callOnTerminate whether to call the function even on abnormal termination
   */
  void atExit(std::function<void()>&& fn, bool callOnTerminate = false) const;

  /**
   * Returns the code locale used for logging and error messages, which is `en_US.UTF-8`, or the classic
   * locale if `en_US.UTF-8` is not available on this system.
   *
   * This is also the default locale for Rocket.Test unit tests.
   *
   * May be called before #init.
   *
   * On Linux, the locale may be configured like this:
   *
   * ```shell
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
    if (auto handle  = out.handle(); handle != -1 && ROCKET_ISATTY(handle)) {
      out.print(fg(fmt::color::red) | fmt::emphasis::bold, "{}", label);
    } else {
      out.write(label);
    }
    out.println(fmt, std::forward<T>(args)...);

    if (status != 0) {
      exit(status);
    }
  }

  /**
   * Exits the program.
   *
   * May be called before #init.
   *
   * Depending on how #init was parameterized, this function either calls #std::exit or #std::quick_exit.
   * The environment variables `ROCKET_EXIT` and `ROCKET_QUICK_EXIT` may overrule this setting.
   *
   * @param status the exit status
   */
  [[noreturn]] void exit(i32 status) const;

  /**
   * Returns the process ID (PID).
   *
   * May be called before #init.
   *
   * @return the process ID (PID)
   */
  [[nodiscard]] u32 id() const;

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
    if (auto handle = out.handle(); handle != -1 && ROCKET_ISATTY(handle)) {
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
   * @param name the name of the program
   * @param locale the locale to use for the process. If this is null, the locale from the environment is
   *    used
   * @param quickExit if `true`, #exit calls #std::quick_exit, otherwise it calls #std::exit. The environment
   *    variables `ROCKET_EXIT` and `ROCKET_QUICK_EXIT` may override this setting
   */
  void init(
    i32 argc,
    char** argv,
    std::string_view name,
    std::optional<std::locale> locale = std::nullopt,
    bool quickExit = true) const;

  /**
   * Returns the command the process was started with.
   *
   * If the command is not available, for example because `argv` was empty, this is `"program"`.
   *
   * May be called before #init.
   *
   * @return the invocation path
   */
  [[nodiscard]] const std::filesystem::path& invocationPath() const;

  /**
   * Returns the file-stem portion of the command the process was started with.
   *
   * If the file stem is not available, for example because `argv` was empty, this is `"program"`.
   *
   * May be called before #init.
   *
   * @return the file-stem portion of the invocation path
   */
  [[nodiscard]] const std::filesystem::path& invocationStem() const;

  /**
   * Returns the locale the process picked in #init.
   *
   * Must be called after #init.
   *
   * @return a locale
   */
  [[nodiscard]] const std::locale& initLocale() const;

  /**
   * Returns the name of the process.
   *
   * Must be called after #init.
   *
   * @return the name of the process
   */
  [[nodiscard]] const std::string& name() const;

  /**
   * Returns the system locale.
   *
   * This is the global locale that was active by the time the process was constructed.
   *
   * May be called before #init.
   *
   * @return the system locale
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
    if (auto handle = out.handle(); handle != -1 && ROCKET_ISATTY(handle)) {
      out.print(fg(fmt::color::yellow) | fmt::emphasis::bold, "warning: ");
    } else {
      out.write("warning: ");
    }
    out.println(fmt, std::forward<T>(args)...);
  }

private:

  mutable std::filesystem::path invocationPath_;
  mutable std::filesystem::path invocationStem_;

  mutable i32 argc_ = 0;
  mutable char** argv_ = nullptr;
  mutable std::string name_;
  mutable bool quickExit_ = true;
  mutable std::vector<std::string> args_;
  mutable bool inited_ = false;

  // Default-constructed locale: a copy of the current global locale
  std::locale systemLocale_;
  std::locale codeLocale_ = internal::process::tryLocale("en_US.UTF-8").value_or(std::locale::classic());
  mutable std::locale initLocale_;

  Process() = default;

  [[nodiscard]] std::string autoName() const;

  friend const Process& process();
};

/**
 * Returns a reference to the current process.
 *
 * @return a reference to the current process.
 */
[[nodiscard]] const Process& process();

} // namespace rocket

// EOF
