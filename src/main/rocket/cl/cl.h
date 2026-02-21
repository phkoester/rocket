/**
 * @file cl.h
 *
 * Rocket CL: a command-line parser and help-text formatter.
 */

#pragma once

#include "rocket/Process.h"
#include "rocket/type-traits.h"
#include "rocket/nio/nio-fwd.h"
#include "rocket/str/str.h"
#include "rocket/str/StringConvert.h"
#include "rocket/unicode/Character.h"

#include <fmt/std.h>

#include <map>
#include <set>

namespace rocket::cl {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

// #ValueType ...............................................................................................

template<typename T>
struct ValueTypeImpl {
  using Type = T;
};

template<typename T>
struct ValueTypeImpl<std::optional<T>> {
  using Type = ValueTypeImpl<T>::Type;
};

template<typename T>
using ValueType = ValueTypeImpl<T>::Type;

// #applyTo .................................................................................................

inline bool
applyTo(bool& out, std::string_view val) {
  out = str::toType<bool>(val);
  return out;
}

inline bool
applyTo(std::optional<bool>& out, std::string_view val) {
  out = str::toType<bool>(val);
  return *out;
}

template<typename T>
inline bool
applyTo(T& out, std::string_view val) {
  out = str::toType<T>(val);
  return true;
}

template<typename T>
inline bool
applyTo(std::optional<T>& out, std::string_view val) {
  out = str::toType<T>(val);
  return true;
}

template<typename T>
inline bool
applyTo(std::vector<T>& out, std::string_view val) {
  out.push_back(str::toType<T>(val));
  return true;
}

template<typename T>
inline bool
applyTo(std::optional<std::vector<T>>& out, std::string_view val) {
  if (not out) {
    out = std::vector<T>();
  }
  out->push_back(str::toType<T>(val));
  return true;
}

template<typename I> requires IsInteger<I>
inline bool
applyToInteger(I& out, std::string_view val) {
  bool flag = str::toType<bool>(val);
  if (flag) {
    ++out;
  } else if (out > 0) {
    --out;
  }
  return flag;
}

template<typename I> requires IsInteger<I>
inline bool
applyToInteger(std::optional<I>& out, std::string_view val) {
  if (not out) {
    out = I();
  }
  return applyToInteger(*out, val);
}

} // namespace internal

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

// #OptionType ----------------------------------------------------------------------------------------------

enum class OptionType { Custom, Help, Verbose, Version };

// #OptionConfig --------------------------------------------------------------------------------------------

struct OptionConfig {
  /**
   * A set of allowed values.
   */
  std::optional<std::set<std::string>> choices = {};
  /**
   * Short description.
   *
   * By convention, this starts with a lower-case verb and does not end with a period, e.g.
   * `"print NUM lines of leading context"`.
   */
  std::optional<std::string> description = {};
  /**
   * Short format description.
   *
   * If the option takes a value, this parameter should briefly describe the format, e.g.
   * <code>"file"</code>, <code>"number"</code>, or <code>"`red`, `green`, or `blue`"</code>.
   */
  std::optional<std::string> format = {};
  /**
   * A pointer to an option group. May be null.
   */
  const OptionGroup* group = nullptr;
   /**
   * Maximum number of occurrences.
   */
  u64 maxOccurs = NPOS;
  /**
   * Minimum number of occurrences.
   */
  u64 minOccurs = 0;
  /**
   * The option name.
   *
   * For example, if this is `"verbose"`, the option may be chosen via `--verbose` on the command line.
   */
  std::string name;
  /**
   * An optional short name.
   *
   * For instance, if this is `"€"`, the option may be chosen via `-€` on the command line.
   */
  std::optional<unicode::Character<char>> shortName = {};
  /**
   * An optional verbose description.
   *
   * A verbose description that is displayed when verbose help is requested.
   */
  std::optional<std::string> verboseDescription = {};
};

// #Option --------------------------------------------------------------------------------------------------

/// Command-line options.
struct Option {
  /// Type for a function that is called to apply an option value.
  using Apply = std::function<bool(std::string_view val)>;

  static Option
  of(OptionType type, const OptionConfig& config, bool takesValue, Apply apply) {
    return {
      .apply=apply,
      .choices=config.choices,
      .description=config.description,
      .format=config.format,
      .group=config.group,
      .maxOccurs=config.maxOccurs,
      .minOccurs=config.minOccurs,
      .name=config.name,
      .shortName=config.shortName,
      .takesValue=takesValue,
      .type=type,
      .verboseDescription=config.verboseDescription
    };
  }

  template<typename T>
  static Option
  custom(const OptionConfig& config, T& out) {
    bool takesValue = true;
    Apply apply;
    using ValueType = internal::ValueType<T>;
    if constexpr (std::is_same_v<ValueType, bool>) {
      takesValue = false;
    }
    if constexpr (IsInteger<ValueType>) {
      if (config.maxOccurs > 1) {
        takesValue = false;
        apply = [&](std::string_view val) { return internal::applyToInteger(out, val); };
      } else {
        apply = [&](std::string_view val) { return internal::applyTo(out, val); };
      }
    } else {
      apply = [&](std::string_view val) { return internal::applyTo(out, val); };
    }

    OptionConfig configCopy(config);

    // If no format is provided, but choices are, generate a format string from the choices
    if (config.choices && not config.format) {
      std::set<std::string> quoted;
      for (const auto& val : *config.choices) {
        quoted.insert(fmt::format("`{}`", val));
      }
      configCopy.format = str::join(quoted.begin(), quoted.end(), ", ", " or ", ", or ");
    }

    configCopy.minOccurs = IsOptional<T> ? 0 : 1;

    return of(OptionType::Custom, configCopy, takesValue, apply);
  }

  static Option
  help(const OptionGroup* group, std::optional<bool>& out) {
    OptionConfig config {
      .description="display this help text and exit",
      .group=group,
      .name="help",
      .shortName=unicode::Character<char>("?")
    };
    Apply apply = [&](std::string_view val) { return internal::applyTo(out, val); };
    return of(OptionType::Help, config, false, apply);
  }

  static Option
  verbose(const OptionGroup* group, std::optional<bool>& out) {
    OptionConfig config {
      .description="produce verbose output",
      .group=group,
      .name="verbose",
      .shortName=unicode::Character<char>("v")
    };
    Apply apply = [&](std::string_view val) { return internal::applyTo(out, val); };
    return of(OptionType::Verbose, config, false, apply);
  }

  static Option
  verbose(const OptionGroup* group, std::optional<u64>& out, u64 maxOccurs) {
    ROCKET_CHECK(maxOccurs, maxOccurs > 1);

    OptionConfig config {
      .description="produce verbose output",
      .group=group,
      .maxOccurs=maxOccurs,
      .name="verbose",
      .shortName=unicode::Character<char>("v")
    };
    Apply apply = [&](std::string_view val) { return internal::applyToInteger(out, val); };
    return of(OptionType::Verbose, config, false, apply);
  }

  static Option
  version(const OptionGroup* group, std::optional<bool>& out) {
    OptionConfig config {
      .description="display version information and exit",
      .group=group,
      .name="version"
    };
    Apply apply = [&](std::string_view val) { return internal::applyTo(out, val); };
    return of(OptionType::Version, config, false, apply);
  }

  /// @cond undocumented
  Apply apply;
  std::optional<std::set<std::string>> choices;
  std::optional<std::string> description;
  std::optional<std::string> format;
  const OptionGroup* group = nullptr;
  u64 maxOccurs = NPOS;
  u64 minOccurs = 0;
  std::string name;
  std::optional<unicode::Character<char>> shortName;
  bool takesValue = false;
  OptionType type = OptionType::Custom;
  std::optional<std::string> verboseDescription;
  /// @endcond
};

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
   * @param name the name of the parameter, e.g. `"FILE"`. By convention, this is in all-caps and matches
   *   the usage line
   * @param format this parameter should briefly describe the format, e.g. <code>"file"</code>,
   *   <code>"number"</code>, or <code>"`red`, `green`, or `blue`"</code>
   * @param description a short description text. By convention, this starts with a lower-case letter and
   *   does not end with a period, e.g. `"the input file"`
   * @param out the destination reference that is assigned the argument
   * @return a new parameter
   */
  template<typename T>
  static Parameter
  of(
    const std::string& name,
    const std::optional<std::string>& format,
    const std::optional<std::string>& description,
    T& out) {
    return {
      name,
      std::nullopt, // #choices
      1, // #maxOccurs
      IsOptional<T> ? 0 : 1, // #minOccurs
      false, // #consumeOpts
      format,
      description,
      {},
      [&](std::string_view val) { internal::applyTo(out, val); }
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
   * @param choices a set of allowed values
   * @param description a short description text. By convention, this starts with a lower-case letter and
   *   does not end with a period, e.g. `"the input file"`
   * @param out the destination reference that is assigned the argument
   * @return a new parameter
   */
  template<typename T>
  static Parameter
  of(
    const std::string& name,
    const std::set<typename internal::ValueType<T>>& choices,
    const std::optional<std::string>& description,
    T& out) {
    Parameter ret {
      name,
      std::nullopt, // #choices
      IsVector<typename internal::ValueType<T>> ? NPOS : 1, // #maxOccurs
      IsOptional<T> ? 0 : 1, // #minOccurs
      false, // #consumeOpts
      std::nullopt, // #format
      description,
      {},
      [&](std::string_view val) { internal::applyTo(out, val); }
    };

    std::set<std::string> strings;
    for (const auto& val : choices) {
      strings.insert(str::toString(val));
    }
    ret.choices = strings;

    std::set<std::string> quotedStrings;
    for (const auto& val : strings) {
      quotedStrings.insert(fmt::format("`{}`", val));
    }
    ret.format = str::join(quotedStrings.begin(), quotedStrings.end(), ", ", " or ", ", or");

    return ret;
  }

  std::string name; ///< The parameter name.
  std::optional<std::set<std::string>> choices; ///< Allowed values.
  u64 maxOccurs = 1; ///< Maximum number of occurrences.
  u64 minOccurs = 1; ///< Minimum number of occurrences.
  bool consumeOpts = false; ///< Whether options shall be consumed after this parameter.
  std::optional<std::string> format; ///< Format text.
  std::optional<std::string> description; ///< Description text.
  std::optional<std::string> verboseDescription; ///< Verbose description text.
  Apply apply; ///< Callback function that applies the argument.
};

// #CommandLineConfig ---------------------------------------------------------------------------------------

/**
 * Parameters that configure the behavior of a #rocket::cl::CommandLine.
 */
struct CommandLineConfig {
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
   * is printed before the help text.
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
   * @param params the command-line parameters
   * @param config the configuration
   */
  explicit CommandLine(
    const std::vector<Option>& opts = {},
    const std::vector<Parameter>& params = {},
    const CommandLineConfig& config = {});

  /**
   * Parses the command-line arguments @p args, assigns values to bound destination references.
   *
   * @param args the command-line arguments, e.g. `process.args()`
   * @param out the sink to write standard output to
   * @param err the sink to write error output to
   * @param exit if `true`, the program exits on help, version, or parse failure
   * @return whether the application should continue after parsing. On help, version, or parse failure, the
   *   function returns `false`. If @p exit is `true`, the return value may be ignored
   */
  bool parse(
    const std::vector<std::string>& args,
    nio::Sink& out = nio::out,
    nio::Sink& err = nio::err,
    bool exit = true);

private:

  struct ParserState {
    std::map<const Option*, u64> seenOpts;
    std::set<const Parameter*> seenParams;

    bool help = false; ///< Available only after `eval()`
    bool verbose = false; ///< Available only after `eval()`
    bool version = false; ///< Available only after `eval()`

    void eval();

    void updateOpt(const Option& opt, bool flag);
  };

  static std::string name(const Option& opt, bool nameFlag);

  static void validate(std::string_view name, bool nameFlag);

  std::vector<Option> opts_;
  std::vector<Parameter> params_;
  CommandLineConfig config_;
  bool hasUsage_ = false;
  bool hasHelpOpt_ = false;
  bool hasVerboseDescriptions_ = false;
  std::map<std::string_view, const Option*> byName_;
  std::map<std::string_view, const Option*> byShortName_;

  ParserState parserState_;

  void applyOpt(const Option& opt, bool nameFlag, const std::optional<std::string>& value);

  void applyParam(const Parameter& param, const std::string& value);

  void handleException(const std::exception& ex, nio::Sink& out, i32 status) const;

  void printHelp(nio::Sink& out, bool verbose, bool exit);

  void printHelpOpts(nio::Sink& out, bool verbose, u64 width) const;

  void printHelpParams(nio::Sink& out, u64 width) const;

  void printTryHelp(nio::Sink& out) const;

  void printUsage(nio::Sink& out) const;

  void printVersion(nio::Sink& out, bool exit);
};

} // namespace rocket::cl

// EOF
