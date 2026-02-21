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

/// The type of a #rocket::cl::Option: either custom or one of the predefined types with a special meaning.
enum class OptionType { Custom, Help, Verbose, Version };

// #OptionConfig --------------------------------------------------------------------------------------------

/// Configuration for #rocket::cl::Option.
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
   * A pointer to an option group.
   *
   * May be null.
   */
  const OptionGroup* group = nullptr;
  /**
   * Maximum number of occurrences.
   */
  u64 maxOccurs = NPOS;
  /**
   * Minimum number of occurrences.
   *
   * If null, this will be auto-configured.
   */
  std::optional<u64> minOccurs = {};
  /**
   * The option name.
   *
   * For example, if this is `"verbose"`, the option may be chosen via `--verbose` on the command line.
   */
  std::string name;
  /**
   * The option's short name.
   *
   * For instance, if this is `"⨁"`, the option may be chosen via `-⨁` on the command line.
   */
  std::optional<unicode::Character<char>> shortName = {};
  /**
   * Whether the option takes a value.
   *
   * If null, this will be auto-configured.
   */
  std::optional<bool> takesValue = {};
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

  /**
   * Low-level factory function that makes a new option.
   *
   * Usually, you should use one of the following convenience functions instead.
   *
   * @param type the type of the option
   * @param config the configuration
   * @param apply the function that applies the option value
   * @return a new option
   */
  static Option
  of(OptionType type, const OptionConfig& config, Apply apply) {
    return {
      .apply=apply,
      .choices=config.choices,
      .description=config.description,
      .format=config.format,
      .group=config.group,
      .maxOccurs=config.maxOccurs,
      .minOccurs=config.minOccurs.value_or(0),
      .name=config.name,
      .shortName=config.shortName,
      .takesValue=config.takesValue.value_or(false),
      .type=type,
      .verboseDescription=config.verboseDescription
    };
  }

  /**
   * Convenience factory function that makes a new custom option and binds it to a destination reference.
   *
   * @tparam T the type of the destination reference. If this is a #std::optional reference, the option
   *   is optional, otherwise it is required. If this is a #std::vector reference, the option can consume
   *   multiple values from the command line
   * @param config the configuration
   * @param out the destination reference that is assigned the option value
   * @return a new option
   */
  template<typename T>
  static Option
  custom(const OptionConfig& config, T& out) {
    Apply apply;
    using ValueType = internal::ValueType<T>;
    if constexpr (IsInteger<ValueType>) {
      if (config.takesValue == false) { // Sic!
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

    // Auto-configure #minOccurs
    if constexpr (not IsOptional<T>) {
      if (not config.minOccurs) {
        configCopy.minOccurs = 1;
      }
    }

    // Auto-configure #takesValue
    if constexpr (std::is_same_v<ValueType, bool>) {
      configCopy.takesValue = false;
    } else {
      if (not config.takesValue) {
        configCopy.takesValue = true;
      }
    }

    return of(OptionType::Custom, configCopy, apply);
  }

  /**
   * Convenience factory function that makes a new help option and binds it to a destination reference.
   *
   * When providing a help option, consider providing a verbose option and verbose descriptions for options
   * and parameters as well.
   *
   * @param group the option group, may be null
   * @param out the destination reference that is assigned the option value
   * @return a new option
   */
  static Option
  help(const OptionGroup* group, std::optional<bool>& out) {
    OptionConfig config {
      .description="display this help text and exit",
      .group=group,
      .name="help",
      .shortName=unicode::Character<char>("?")
    };
    Apply apply = [&](std::string_view val) { return internal::applyTo(out, val); };
    return of(OptionType::Help, config, apply);
  }

  /**
   * Convenience factory function that makes a new verbose option and binds it to a destination reference.
   *
   * @param group the option group, may be null
   * @param out the destination reference that is assigned the option value
   * @return a new option
   */
  static Option
  verbose(const OptionGroup* group, std::optional<bool>& out) {
    OptionConfig config {
      .description="produce verbose output",
      .group=group,
      .name="verbose",
      .shortName=unicode::Character<char>("v")
    };
    Apply apply = [&](std::string_view val) { return internal::applyTo(out, val); };
    return of(OptionType::Verbose, config, apply);
  }

  /**
   * Convenience factory function that makes a new verbose option and binds it to a destination reference.
   *
   * Each occurrence of the option in the command line increases the level of verbosity by one. `-v` sets the
   * level to 1, `-vv` sets it to 2, and so on.
   *
   * @param group the option group, may be null
   * @param maxOccurs the maximum number of occurrences
   * @param out the destination reference that is assigned the option value
   * @return a new option
   */
  static Option
  verbose(const OptionGroup* group, u64 maxOccurs, std::optional<u64>& out) {
    ROCKET_CHECK(maxOccurs, maxOccurs > 1);

    OptionConfig config {
      .description="increase level of verbosity",
      .group=group,
      .maxOccurs=maxOccurs,
      .name="verbose",
      .shortName=unicode::Character<char>("v"),
      .takesValue=false,
      .verboseDescription=fmt::format("increase level of verbosity (may be supplied up to {} times)", maxOccurs)
    };
    Apply apply = [&](std::string_view val) { return internal::applyToInteger(out, val); };
    return of(OptionType::Verbose, config, apply);
  }

  /**
   * Convenience factory function that makes a new version option and binds it to a destination reference.
   *
   * @param group the option group, may be null
   * @param out the destination reference that is assigned the option value
   * @return a new option
   */
  static Option
  version(const OptionGroup* group, std::optional<bool>& out) {
    OptionConfig config {
      .description="display version information and exit",
      .group=group,
      .name="version"
    };
    Apply apply = [&](std::string_view val) { return internal::applyTo(out, val); };
    return of(OptionType::Version, config, apply);
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

// #ParameterConfig -----------------------------------------------------------------------------------------

/// Configuration for #rocket::cl::Parameter.
struct ParameterConfig {
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
    using ValueType = internal::ValueType<T>;

    return {
      name,
      std::nullopt, // #choices
      IsVector<ValueType> ? NPOS : 1, // #maxOccurs
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
    using ValueType = internal::ValueType<T>;

    Parameter ret {
      name,
      std::nullopt, // #choices
      IsVector<ValueType> ? NPOS : 1, // #maxOccurs
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

  /// @cond undocumented
  // XXX alphabetisch
  std::string name;
  std::optional<std::set<std::string>> choices;
  u64 maxOccurs = 1;
  u64 minOccurs = 1;
  bool consumeOpts = false;
  std::optional<std::string> format;
  std::optional<std::string> description;
  std::optional<std::string> verboseDescription;
  Apply apply;
  /// @endcond
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
    std::map<const Parameter*, u64> seenParams;

    bool help = false; ///< Available only after `eval()`
    bool verbose = false; ///< Available only after `eval()`
    bool version = false; ///< Available only after `eval()`

    u64 countOpt(const Option& opt) const;

    u64 countParam(const Parameter& param) const;

    /// Initializes #help, #verbose, and #version
    void eval();

    void updateOpt(const Option& opt, bool flag);

    void updateParam(const Parameter& param);
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

  void printHelpParams(nio::Sink& out, bool verbose, u64 width) const;

  void printTryHelp(nio::Sink& out) const;

  void printUsage(nio::Sink& out) const;

  void printVersion(nio::Sink& out, bool exit);
};

} // namespace rocket::cl

// EOF
