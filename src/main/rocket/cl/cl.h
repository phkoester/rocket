/**
 * @file cl.h
 *
 * Rocket CL: a command-line parser and help-text formatter.
 */

#pragma once

#include "rocket/Process.h"
#include "rocket/type-traits.h"
#include "rocket/nio/nio-fwd.h"
#include "rocket/str/StringConvert.h"
#include "rocket/unicode/Character.h"

#include <set>
#include <unordered_map>

namespace rocket::cl {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

// #ValueType ...............................................................................................

template<typename T>
struct ValueType {
  using Type = T;
};

template<typename T>
struct ValueType<std::optional<T>> {
  using Type = T;
};

// #applyTo .................................................................................................

template<typename T>
inline void
applyTo(T& dest, std::string_view val) {
  dest = str::toType<T>(val);
}

template<typename T>
inline void
applyTo(std::vector<T>& dest, std::string_view val) {
  dest.push_back(str::toType<T>(val));
}

template<typename T>
inline void
applyTo(std::optional<T>& dest, std::string_view val) {
  dest = str::toType<T>(val);
}

template<typename T>
inline void
applyTo(std::optional<std::vector<T>>& dest, std::string_view val) {
  if (not dest) {
    dest = std::vector<T>();
  }
  dest->push_back(str::toType<T>(val));
}

} // namespace internal

// #Parameter -----------------------------------------------------------------------------------------------

/// Positional command-line parameters.
struct Parameter {
  /// Type for a function that is called to apply an argument.
  using Apply = std::function<void(std::string_view val)>;

  /**
   * Convenience function that makes a new parameter and binds it to a destination reference.
   *
   * @tparam T the type of the destination reference. If this is a #std::optional reference, the parameter
   *   is optional, otherwise it is required. If this is a #std::vector reference, the parameter can consume
   *   multiple arguments from the command line
   * @param name the name of the parameter, e.g. `"FILE"`. By conention, this is in all-caps and matches
   *   the usage line
   * @param format this parameter should briefly describe the format, e.g.
   *   `"FILE"`, `"NUMBER"` etc.
   * @param help a short help text. By convention, this starts with a lower-case letter and does not end with
   *   a period, e.g. `"path to input file"`
   * @param dest the destination reference that is assigned the argument
   * @return a new parameter
   */
  template<typename T>
  static inline Parameter
  of(
    const std::string& name,
    const std::optional<std::string>& format,
    const std::optional<std::string>& help,
    T& dest) {
    return {
      name,
      std::nullopt,
      IsVector<typename internal::ValueType<T>::Type> ? NPOS : 1, // #maxOccurs
      false, // #consumeOptions
      not IsOptional<T>, // #required
      format,
      help,
      [&](std::string_view val) { internal::applyTo(dest, val); }
    };
  }

  /**
   * Convenience function that makes a new parameter and binds it to a destination reference.
   *
   * @tparam T the type of the destination reference. If this is a #std::optional reference, the parameter
   *   is optional, otherwise it is required. If this is a #std::vector reference, the parameter can consume
   *   multiple arguments from the command line
   * @param name the name of the parameter, e.g. `"FILE"`. By conention, this is in all-caps and matches
   *   the usage line
   * @param allowedValues a set of allowed values
   * @param format this parameter should briefly describe the format, e.g.
   *   `"FILE"`, `"NUMBER"` etc.
   * @param help a short help text. By convention, this starts with a lower-case letter and does not end with
   *   a period, e.g. `"path to input file"`
   * @param dest the destination reference that is assigned the argument
   * @return a new parameter
   */
  template<typename T>
  static inline Parameter
  of(
    const std::string& name,
    const std::set<typename internal::ValueType<T>::Type>& allowedValues,
    const std::optional<std::string>& format,
    const std::optional<std::string>& help,
    T& dest) {
    Parameter ret {
      name,
      std::nullopt,
      IsVector<typename internal::ValueType<T>::Type> ? NPOS : 1, // #maxOccurs
      false, // #consumeOptions
      not IsOptional<T>, // #required
      format,
      help,
      [&](std::string_view val) { internal::applyTo(dest, val); }
    };
    std::set<std::string> strings;
    for (const auto& val : allowedValues) {
      strings.insert(str::toString(val));
    }
    ret.allowedValues = strings;
    return ret;
  }

  std::string name; ///< The parameter name.
  std::optional<std::set<std::string>> allowedValues; ///< Allowed values.
  u64 maxOccurs = 1; ///< Maximum number of occurrences.
  bool consumeOptions = false; ///< Whether options shall be consumed after this parameter.
  bool required = false; ///< Required?
  std::optional<std::string> format; ///< Format text.
  std::optional<std::string> help; ///< Help text.
  Apply apply; ///< Callback function that applies the argument.
};

// #OptionGroup ---------------------------------------------------------------------------------------------

/**
 * An option group with a title.
 *
 * Command-line options may be assigned a pointer to an option group. When displaying the help text, options
 * appear grouped by their groups.
 */
struct OptionGroup {
  std::string title; ///< The title.
};

// #Option --------------------------------------------------------------------------------------------------

/// Command-line options.
struct Option {
  /// Type for a function that is called to apply an option value.
  using Apply = std::function<void(std::string_view val)>;

  /**
   * Convenience function that makes a new help option.
   *
   * @param group a pointer to an option group. May be null
   * @param dest a reference to an optional `bool` value that will be set to `true` if the help option is
   *   supplied
   * @return a new help option
   */
  static inline Option
  helpOf(
    const OptionGroup* group,
    std::optional<bool>& dest) {
    return of(
      group,
      "help",
      unicode::CharacterView<char>("?"),
      std::nullopt,
      "display this help text and exit",
      dest);
  }

  /**
   * Convenience function that makes a new option and binds it to a destination reference.
   *
   * @tparam T the type of the destination reference. If this is a #std::optional reference, this option is
   *   optional, otherwise it is required. If this is a `bool` reference, the option takes no argument,
   *   otherwise it does. If this is a #std::vector reference, multiple values may be supplied on the command
   *   line
   * @param group a pointer to an option group. May be null
   * @param name the name of the option. For example, if this is `"verbose"`, the option may be chosen via
   *   `--verbose` on the command line
   * @param shortName an optional short name. For example, if this is <code>"€"</code>, the option may be
   *   chosen via `-€` on the command line
   * @param format if the option takes an argument, this parameter should briefly describe the format, e.g.
   *   `"FILE"`, `"NUM"` etc.
   * @param help a short help text. By convention, this starts with a lower-case verb and does not end with
   *   a period, e.g. `"print NUM lines of leading context"`
   * @param dest the destination reference that is assigned the option's value
   * @return a new option
   */
  template<typename T>
  static inline Option
  of(
    const OptionGroup* group,
    const std::string& name,
    const std::optional<unicode::CharacterView<char>>& shortName,
    const std::optional<std::string>& format,
    const std::optional<std::string>& help,
    T& dest) {
    return {
      group,
      name,
      shortName.transform([](auto val) { return unicode::Character<char>(val); }),
      // #takesValue is `false` for `bool`, otherwise it is `true`
      std::is_same_v<typename internal::ValueType<T>::Type, bool> ? false : true,
      not IsOptional<T>, // #required
      format,
      help,
      [&](std::string_view val) { internal::applyTo(dest, val); }
    };
  }

  const OptionGroup* group = nullptr; ///< The option group.
  std::string name; ///< The option name.
  std::optional<unicode::Character<char>> shortName; ///< The option short name.
  bool takesValue = false; ///< Option takes value?
  bool required = false; ///< Required?
  std::optional<std::string> format; ///< Format text.
  std::optional<std::string> help; ///< Help text.
  Apply apply; ///< Callback function that applies the argument.
};

// #CommandLineParams ---------------------------------------------------------------------------------------

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
  /// Prolog text to be displayed when the `--help` option is supplied.
  std::optional<std::string> prolog = std::nullopt;
  /// Epilog text to be displayed when the `--help` option is supplied.
  std::optional<std::string> epilog = std::nullopt;

  /**
   * Did another module process the command line and output something? If this is `true`, an extra empty line
   * is printed when calling #CommandLine#help.
   */
  bool otherOutput = false;
  /**
   * Include Rocket's standard options, such as for logging control?
   */
  bool rocketOpts = true;
};

// #CommandLine ---------------------------------------------------------------------------------------------

/**
 * A command-line parser and help-text formatter.
 */
struct CommandLine {
  /**
   * @ctor
   *
   * @param opts the command-line options
   * @param params parameters that configure the parser
   */
  CommandLine(
    const std::vector<Option>& opts = {},
    const std::vector<Parameter>& args = {},
    const CommandLineParams& params = {});

  /**
   * To be called when the command line did not satisfy the usage rules.
   *
   * @param out the sink to write to
   * @param status program exit status. If this is not 0, the program exits with this status
   */
  // XXX private
  void error(nio::Sink& out, i32 status = EXIT_SERIOUS_FAILURE) const;

  /**
   * To be called when #parse threw an exception.
   *
   * @param ex the exception that was caught
   * @param out the sink to write to
   * @param status program exit status. If this is not 0, the program exits with this status
   */
  // XXX private
  void handleException(
    const std::exception& ex,
    nio::Sink& out,
    i32 status = EXIT_SERIOUS_FAILURE) const;

  /**
   * To be called when the `--help` option appeared on the command line.
   *
   * @param out the sink to write to
   * @param exit if `true`, the program exits with `EXIT_SUCCESS`, otherwise it continues to run
   */
  // XXX private
  void help(nio::Sink& out, bool exit);

  /**
   * Parses the command-line arguments @p args, assigns values to bound destination references.
   *
   * @param args the command-line arguments, e.g. `process.args()`
   * @param out the sink to write standard output to
   * @param err the sink to write error output to
   * @param exit if `true`, the program exits if the command line cannot be parsed successfully
   * @return whether the application should continue after parsing
   */
  bool parse(
    const std::vector<std::string>& args,
    nio::Sink& out = nio::out,
    nio::Sink& err = nio::err,
    bool exit = true);

private:

  struct ParserState {
    bool seenHelp = false;
    std::set<const Option*> seenOpts;
    std::set<const Parameter*> seenParams;
  };

  static std::string name(const Option& opt, bool nameFlag);

  static void validate(std::string_view name, bool nameFlag);

  std::vector<Option> opts_;
  std::vector<Parameter> params_;
  CommandLineParams config_;
  bool hasUsage_;
  bool hasHelpOpt_;
  std::unordered_map<std::string_view, const Option*> byName_;
  std::unordered_map<std::string_view, const Option*> byShortName_;

  ParserState parserState_;

  void applyOpt(const Option& opt, bool nameFlag, const std::optional<std::string>& value);

  void applyParam(const Parameter& param, const std::string& value);

  void helpOpts(nio::Sink& out, u64 width) const;

  void printHelp(nio::Sink& out) const;

  void printUsage(nio::Sink& out) const;
};

} // namespace cl

// EOF
