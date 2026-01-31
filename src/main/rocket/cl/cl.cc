/*
 * cl.cc
 */

 #include "cl.h"

#include "rocket/assert.h"
#include "rocket/literal.h"
#include "rocket/log/log.h"
#include "rocket/str/str.h"
#include "rocket/system/terminal/terminal.h"
#include "rocket/unicode/Iterator.h"

using namespace std;

namespace rocket::cl {

// #CommandLine ---------------------------------------------------------------------------------------------

CommandLine::CommandLine(
  const vector<Option>& opts,
  const vector<Parameter>& params,
  const CommandLineConfig& config) :
  opts_(opts),
  params_(params),
  config_(config),
  hasUsage_(not config.usages.empty()),
  hasHelpOpt_(false) {
  // If requested, prepend Rocket options

  if (config.rocketOpts) {
    const auto& logOpts = log::internal::logOptions();
    opts_.insert(opts_.begin(), logOpts.begin(), logOpts.end());
  }

  // NOTE: Because we use string views and pointers in the maps, #opts_ may never be changed from this point

  // Validate options, populate maps

  for (const auto& opt : opts_) {
    if (opt.name == "help") {
      hasHelpOpt_ = true;
    }
    validate(opt.name, true);
    auto pair = byName_.emplace(opt.name, &opt);
    ROCKET_CHECK(opts, pair.second, "Duplicate option `{}`", name(opt, true));
    if (opt.shortName) {
      string shortName = static_cast<string>(*opt.shortName);
      validate(shortName, false);
      auto pair = byShortName_.emplace(*opt.shortName, &opt); // cppcheck-suppress shadowVariable
      ROCKET_CHECK(opts, pair.second, "Duplicate option `{}`", name(opt, false));
    }
  }
}

void
CommandLine::applyOpt(const Option& opt, bool nameFlag, const optional<string>& value) {
  // Don't use #ROCKET_CHECK here for cleaner exception messages

  if (opt.takesValue && not value) {
    ROCKET_FAIL("Missing value for option `{}`", name(opt, nameFlag));
  }
  // Usually, options not taking a value may not be assigned a value. There is one exception to this rule:
  // boolean values are allowed
  static const set<string_view> BOOL_VALUES { "0", "false", "1", "true" };
  if (not opt.takesValue && value && not BOOL_VALUES.contains(*value)) {
    ROCKET_FAIL("Option `{}` cannot take a value", name(opt, nameFlag));
  }
  string useValue = value.value_or("true");

  try {
    if (opt.allowedValues && not opt.allowedValues->contains(useValue)) {
      ROCKET_FAIL("Invalid value `{}`", value);
    }
    opt.apply(useValue);
    if (opt.name == "help") {
      parserState_.seenHelp = true;
    }
    parserState_.seenOpts.insert(&opt);
  } catch (const Exception& ex) {
    string expected;
    if (opt.format) {
      expected = fmt::format("; expected {}", *opt.format);
    }
    ROCKET_FAIL("Option `{}`: {}{}", name(opt, nameFlag), ex.message(), expected);
  } catch (const exception& ex) {
    string expected;
    if (opt.format) {
      expected = fmt::format("; expected {}", *opt.format);
    }
    ROCKET_FAIL("Option `{}`: Invalid value {:?}{}", name(opt, nameFlag), useValue, expected);
  }
}

void
CommandLine::applyParam(const Parameter& param, const string& value) {
  try {
    if (param.allowedValues && not param.allowedValues->contains(value)) {
      ROCKET_FAIL("Invalid value `{}`", value);
    }
    param.apply(value);
    parserState_.seenParams.insert(&param);
  } catch (const Exception& ex) {
    string expected;
    if (param.format) {
      expected = fmt::format("; expected {}", *param.format);
    }
    ROCKET_FAIL("Parameter {}: {}{}", param.name, ex.message(), expected);
  } catch (const exception& ex) {
    string expected;
    if (param.format) {
      expected = fmt::format("; expected {}", *param.format);
    }
    ROCKET_FAIL("Parameter {}: Invalid value {:?}{}", param.name, value, expected);
  }
}

void
CommandLine::handleException(const exception& ex, nio::Sink& out, i32 status) const {
  if (auto p = dynamic_cast<const Exception*>(&ex)) {
    process.error(out, 0, "{}", p->message());
  } else {
    process.error(out, 0, "{}", ex.what());
  }

  if (hasUsage_) {
    printUsage(out);
  }
  if (hasHelpOpt_) {
    printTryHelp(out);
  }
  if (status != 0) {
    process.exit(status);
  }
}

string
CommandLine::name(const Option& opt, bool nameFlag) {
  return nameFlag ? "--" + opt.name : "-" + static_cast<string>(*opt.shortName);
}

bool
CommandLine::parse(const vector<string>& args, nio::Sink& out, nio::Sink& err, bool exit) {
  try {
    parserState_ = ParserState();

    u64 paramIndex = 0;
    u64 paramOccurs = 0;
    bool consumeOpts = false;

    for (auto it = args.begin(), end = args.end(); it != end; ++it) {
      string arg = *it;

      if (not consumeOpts && arg == "--") {
        // 1. `--` seen: Recognize all remaining arguments as positional arguments, continue in loop
        // loop

        consumeOpts = true;
        continue;
      } else if (not consumeOpts && arg.starts_with("--")) {
        // 2. `--...` seen: Parse option by name

        arg = arg.substr(2);
        auto eq = arg.find('=');

        // Extract name, look it up in map
        string name = eq == string::npos ? arg : arg.substr(0, eq);
        auto mapIt = byName_.find(name);
        if (mapIt == byName_.end()) {
          ROCKET_FAIL("Unknown option `--{}`", name);
        }
        const Option& opt = *mapIt->second;

        // Obtain value, if any
        optional<string> value;
        if (eq != string::npos) {
          // Take everything after the `=`
          value = arg.substr(eq + 1);
        } else if (opt.takesValue) {
          // Take the next argument
          if (it + 1 != args.end())
            value = *++it;
        }

        // Apply option
        applyOpt(opt, true, value);
      } else if (not consumeOpts && arg.starts_with("-") && arg != "-") {
        // 3. "-..." seen: Parse options by short name; the last one may take a value

        arg = arg.substr(1);
        auto iter = unicode::Iterator<char>(unicode::IteratorType::Character, arg);
        auto segs = iter.nextSegments();
        auto segsBegin = segs.begin();
        auto segsEnd = segs.end();
        for (auto segIt = segsBegin; segIt != segsEnd; ++segIt) {
          // Extract short name, look it up in map
          auto seg = *segIt;
          auto mapIt = byShortName_.find(seg);
          if (mapIt == byShortName_.end()) {
            ROCKET_FAIL("Unknown option `-{}`", seg);
          }
          const Option& opt = *mapIt->second;

          // Obtain value, if any, apply option
          auto segNext = segIt + 1;
          if (segNext != segsEnd && *segNext == "=") {
            // Option is followed by `=`: Take everything after the `=` and break the character loop
            string value = unicode::concat(segs, ++segNext - segsBegin);
            applyOpt(opt, false, value);
            break;
          } else if (opt.takesValue) {
            // Option takes value: break the character loop
            if (segNext != segsEnd) {
              // Take the rest of the argument
              string value = unicode::concat(segs, segNext - segsBegin);
              applyOpt(opt, false, value);
            }
            else {
              optional<string> value;
              // Take the next argument, if any
              if (it + 1 != args.end()) {
                value = *++it;
              }
              applyOpt(opt, false, value);
            }
            break;
          } else {
            // No value: Apply, continue in code-point loop
            applyOpt(opt, false, nullopt);
          }
        }
      } else {
        // 4. Not an option, but a positional argument: assign

        if (paramIndex >= params_.size()) {
          ROCKET_FAIL("Extraneous argument `{}`", arg);
        }
        const Parameter* param = &params_[paramIndex];
        if (paramOccurs == param->maxOccurs) {
          // Argument has reached its maximum number of occurrences: advance to next argument
          ++paramIndex;
          paramOccurs = 0;
          if (paramIndex >= params_.size()) {
            ROCKET_FAIL("Extraneous argument `{}`", arg);
          }
          param = &params_[paramIndex];
        }
        applyParam(*param, arg);
        ++paramOccurs;
        if (param->consumeOpts) {
          consumeOpts = true;
        }
      }
    }

    // Help?
    if (parserState_.seenHelp) {
      printHelp(out, exit);
      return false;
    }

    // Have we seen all required options?
    for (const auto& opt : opts_) {
      if (opt.required && not parserState_.seenOpts.contains(&opt)) {
        ROCKET_FAIL("Missing required option `{}`", name(opt, true));
      }
    }

    // Have we seen all required parameters?
    for (const auto& param : params_) {
      if (param.required && not parserState_.seenParams.contains(&param)) {
        ROCKET_FAIL("Missing required argument for parameter {}", param.name);
      }
    }

    return true;
  }
  catch (const exception& ex) {
    handleException(ex, err, exit ? EXIT_SERIOUS_FAILURE : 0);
    return false;
  }
}

void
CommandLine::printHelp(nio::Sink& out, bool exit) {
  ROCKET_EXPECT(hasHelpOpt_);

  auto size = system::terminal::size(out);
  u64 width = max(40_u64, size ? size->first : 80_u64);
  bool output = config_.otherOutput;

  // Usage

  if (hasUsage_) {
    if (output) {
      out.write('\n');
    }
    printUsage(out);
    output = true;
  }

  // Prolog

  if (config_.prolog) {
    if (output) {
      out.write('\n');
    }
    out.writeln(str::wrap(*config_.prolog, 0, width));
    output = true;
  }

  // Parameters

  if (not params_.empty()) {
    if (output) {
      out.write('\n');
    }
    printHelpParams(out, width);
    output = true;
  }

  // Options

  if (not opts_.empty()) {
    if (output) {
      out.write('\n');
    }
    printHelpOpts(out, width);
    output = true;
  }

  // Epilog

  if (config_.epilog) {
    if (output) {
      out.write('\n');
    }
    out.writeln(str::wrap(*config_.epilog, 0, width));
  }

  if (exit) {
    process.exit(EXIT_SUCCESS);
  }
}

/**
 * Option groups appear in the order they are seen. However, no group or group with an empty title comes
 * first. Within the groups, options appear in the order they are seen.
 */
void
CommandLine::printHelpOpts(nio::Sink& out, u64 width) const {
  // Collect groups and options therein

  unordered_map<const OptionGroup*, vector<const Option*>> options;
  vector<const OptionGroup*> groups;

  OptionGroup null;
  options.emplace(&null, vector<const Option*>());
  groups.push_back(&null);

  for (const auto& opt : opts_) {
    if (not opt.group || opt.group->title.empty())
      options.find(&null)->second.push_back(&opt);
    else {
      if (auto it = options.find(opt.group); it == options.end()) {
        options.emplace(opt.group, vector<const Option*> { &opt });
        groups.push_back(opt.group);
      } else
        it->second.push_back(&opt);
    }
  }

  // Loop though groups

  bool output = false;

  for (const auto* group : groups) {
    const auto& opts = options.find(group)->second;
    if (opts.empty())
      continue;
    if (output) {
      out.write('\n');
    }
    out.println("{}:\n", group->title);
    output = true;

    // Loop through options

    for (const auto* opt : opts) {
      out.write("  ");
      if (opt->shortName) {
        out.print("-{}, ", static_cast<string>(*opt->shortName));
      } else {
        out.write("    ");
      }
      out.print("--{}", opt->name);
      if (opt->required) {
        out.write(" (required)");
      }
      if (opt->format) {
        out.print(" {}", *opt->format);
      }
      out.write('\n');
      if (opt->help) {
        out.writeln(str::wrap(*opt->help, 10, width));
      }
    }
  }
}

/**
 * Parameters appear in the order they are declared.
 */
void
CommandLine::printHelpParams(nio::Sink& out, u64 width) const {
  // Loop though parameters

  out.writeln("Parameters:\n");

  for (const auto& param : params_) {
    out.write("  ");
    out.write(param.name);
    if (param.required) {
      out.write(" (required)");
    }
    if (param.format) {
      out.print(" {}", *param.format);
    }
    out.write('\n');
    if (param.help) {
      out.writeln(str::wrap(*param.help, 10, width));
    }
  }
}

void
CommandLine::printTryHelp(nio::Sink& out) const {
  ROCKET_EXPECT(hasHelpOpt_);

  out.println("Try `{} --help` for more information.", config_.command);
}

void
CommandLine::printUsage(nio::Sink& out) const {
  ROCKET_EXPECT(hasUsage_);

  out.println("Usage: {} {}", config_.command, config_.usages[0]);
  for (u64 i = 1; i < config_.usages.size(); ++i) {
    out.println("  or   {} {}", config_.command, config_.usages[i]);
  }
}

void
CommandLine::validate(string_view name, bool nameFlag) {
  // Don't use #ROCKET_CHECK here for cleaner exception messages

  const char* what = nameFlag ? "option name" : "option short name";
  if (name.empty()) {
    ROCKET_FAIL("{} may not be empty", str::capitalize(what));
  }
  if (name.starts_with("-") || name.find_first_of(" =") != string::npos) {
    ROCKET_FAIL("Invalid {} {:?}", what, name);
  }
}

} // namespace rocket::cl

// EOF
