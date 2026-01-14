/**
 * @file cl.h
 *
 * Rocket CL: a command-line parser and help-text formatter.
 */

#pragma once

#include "rocket/Process.h"
#include "rocket/assert.h"
#include "rocket/nio/nio-fwd.h"
#include "rocket/str/StringConvert.h"
#include "rocket/unicode/Char.h"

#include <unordered_map>

namespace rocket::cl {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

template<typename T>
void
applyTo(T& dest, std::optional<std::string_view> arg) {
  ROCKET_CHECK(arg, arg);
  dest = str::toType<T>(*arg);
}

template<>
inline void
applyTo(bool& dest, std::optional<std::string_view> arg) {
  // For `bool` only, `arg` may be null
  dest = arg ? str::toType<bool>(*arg) : true;
}

template<typename T>
void
applyTo(std::vector<T>& dest, std::optional<std::string_view> arg) {
  ROCKET_CHECK(arg, arg);
  T val = str::toType<T>(*arg);
  dest.push_back(val);
}

} // namespace internal

// `OptionGroup` --------------------------------------------------------------------------------------------

/**
 * An option group with a title. Command-line options may be assigned a pointer to an option group. When
 * displaying the help text, options appear grouped by their groups.
 */
struct OptionGroup {
  std::string title; ///< The title.
};

// `Option` -------------------------------------------------------------------------------------------------

/**
 * A command-line option. Use the #of factory function to obtain an option and bind it to a destination. If
 * the destination is a `bool` value, the option takes no value, otherwise it does. If the destination is a
 * vector, multiple values may be supplied on the command line.
 */
struct Option {
  /**
   * Type for a function that is called to apply an option value. For `bool` values, @p v may be null. The
   * #of convenience function takes care of this all.
   */
  using Apply = std::function<void(std::optional<std::string_view> v)>;

  /**
   * Makes a new option and binds it to a destination reference.
   *
   * @param group a pointer to an option group. May be null
   * @param name the name of the option. For example, if this is `"verbose"`, the option may be chosen via
   *     `--verbose` on the command line
   * @param shortName an optional short name. For example, if this is <code>"€"</code>, the option may be
   *     chosen via `-€` on the command line
   * @param format if the option takes an argument, this parameter should briefly describe the format, e.g.
   *     `"FILE"`, `"NUM"` etc.
   * @param help a short help text. By convention, this starts with a lower-case verb and does not end with
   *     a period, e.g. "print NUM lines of leading context"
   * @param dest the destination reference that is assigned the option's value. If this is a `bool`
   *     reference, the option takes no argument, otherwise it does. If this is a `std::vector` reference,
   *     multiple values may be supplied on the command line
   * @return a new option
   */
  template<typename T>
  static inline Option
  of(
      const OptionGroup* group,
      const std::string& name,
      const std::optional<unicode::Char<char>>& shortName,
      const std::optional<std::string>& format,
      const std::optional<std::string>& help,
      T& dest) {
    return {
      group,
      name,
      shortName,
      std::is_same_v<T, bool> ? false : true, // `takesValue` is `false` for `bool`, otherwise it is true
      format,
      help,
      [&](std::optional<std::string_view> arg) { internal::applyTo(dest, arg); }
    };
  }

  const OptionGroup* group = nullptr; ///< The option group.
  std::string name; ///< The option name.
  std::optional<unicode::Char<char>> shortName; ///< The option short name.
  bool takesValue = false; ///< Option takes value?
  std::optional<std::string> format; ///< Format text.
  std::optional<std::string> help; ///< Help text.
  Apply apply; ///< Callback function that applies the option's value.
};

// `CommandLineParams` --------------------------------------------------------------------------------------

/**
 * Parameters that configure the behavior of a #rocket::cl::CommandLine.
 */
struct CommandLineParams {
  /// The name of the command. Needed to display the usage. By default, this is the process name.
  std::string command = process.name();
  /**
   * One or more usages, e.g. `{ "[OPTION]... FILE", "[OPTION]... PATTERN FILE" }`. If this is empty, no
   * usage hint is ever printed.
   */
  std::vector<std::string> usages;
  /// Prolog text to be displayed when #CommandLine#help() is called.
  std::optional<std::string> prolog;
  /// Epilog text to be displayed when #CommandLine#help() is called.
  std::optional<std::string> epilog;

  /**
   * Did another module process the command line and output something? If this is `true`, an extra empty line
   * is printed when calling #CommandLine#help.
   */
  bool otherOutput = false;
  /**
   * Include Rocket's standard options, such as logging?
   */
  bool rocketOpts = true;
};

// `CommandLine` --------------------------------------------------------------------------------------------

/**
 * A command-line parser and help-text formatter.
 */
struct CommandLine {
  /**
   * An enum for the result of the function that takes a positional argument.
   */
  enum Took {
    /// Tells the parser to do nothing and go on with the next argument.
    Accept,
    /// Tells the parser to stop and return the rest, exluding the current argument.
    Stop,
    /// Tells the parser to store the current argument for later returnal and go on with the next argument.
    Store,
    /// Tells the parser to stop and return the rest, including the current argument.
    Reject
  };

  /**
   * The function that is called as a callback when the parser sees a positional argument.
   *
   * @param arg the positional argument
   * @return a value telling the parser what to do next
   */
  using Take = std::function<Took(std::string_view arg)>;

  /**
   * @ctor
   *
   * @param opts the command-line options
   * @param params parameters that configure the parser
   */
  explicit CommandLine(const std::vector<Option>& opts = {}, const CommandLineParams& params = {});

  /**
   * To be called when the command line, in particular its positional arguments, did not satisfy the usage
   * rules.
   *
   * @param out the sink to write to
   * @param status program exit status. If this is not `EXIT_SUCCESS` (0), the program exits with this status
   */
  void error(nio::Sink& out, int status = EXIT_SERIOUS_FAILURE) const;

  /**
   * To be called when #parse threw an exception.
   *
   * @param ex the exception that was caught
   * @param out the sink to write to
   * @param status program exit status. If this is not `EXIT_SUCCESS` (0), the program exits with this status
   */
  void handleException(
      const std::exception& ex,
      nio::Sink& out,
      int status = EXIT_SERIOUS_FAILURE) const;

  /**
   * To be called when the `--help` option appeared on the command line.
   *
   * @param out the sink to write to
   * @param exit if `true`, the program exits with `EXIT_SUCCESS`, otherwise it continues to run
   */
  void help(nio::Sink& out, bool exit);

  /**
   * Parses the command-line arguments @p args, assigns values to bound destination references.
   *
   * Call this function in a try/catch block. If an exception is thrown, call #handleException.
   *
   * @param args the command-line arguments, e.g. `process.args()`
   * @param take a take function, may be null. This allows customizing how positional arguments are
   *     processed
   * @return the command-line arguments left for further processing
   * @throw #rocket::InvalidState if the command line could not be parsed successfully
   * @throw std::exception if a problem arises in an option's apply function
   * @see #Take
   * @see #Took
   */
  std::vector<std::string> parse(const std::vector<std::string>& args, const Take& take = {}) const;

private:

  static void apply(const Option& opt, bool nameFlag, std::optional<std::string_view> value);

  static std::string name(const Option& opt, bool nameFlag);

  static void validate(std::string_view name, bool nameFlag);

  std::vector<Option> opts_;
  CommandLineParams params_;
  bool usage_;
  bool help_;

  std::unordered_map<std::string_view, const Option*> byName_;
  std::unordered_map<std::string_view, const Option*> byShortName_;

  void helpOpts(nio::Sink& out, size_t width) const;

  void printHelp(nio::Sink& out) const;

  void printUsage(nio::Sink& out) const;
};

} // namespace cl

// EOF
