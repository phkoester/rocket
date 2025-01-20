/*
 * cl.cc
 */

#include "codec-std-decl.h"
#include "codec-std.h"

#include "S.h"
#include "assert.h"
#include "cl.h"
#include "codec.h"
#include "except.h"
#include "log.h"
#include "quote.h"
#include "strings.h"
#include "terminal.h"
#include "unicode-iterator.h"

using namespace std;

namespace rocket::cl {

// `CommandLine` --------------------------------------------------------------------------------------------

CommandLine::CommandLine(const vector<Option>& opts, const CommandLineParams& params) :
    opts_(opts),
    params_(params),
    usage_(not params.usages.empty()),
    help_(false) {
  // If requested, prepend Rocket options

  if (params.rocketOpts) {
    const auto& logOpts = log::internal::opts();
    opts_.insert(opts_.begin(), logOpts.begin(), logOpts.end());
  }

  // NOTE: Because we use string views and pointers in the maps, `opts_` may never be changed from this point

  // Validate options, populate maps
  
  for (const auto& opt : opts_) {
    if (opt.name == "help")
      help_ = true;
    validate(opt.name, true);
    auto pair = byName_.emplace(opt.name, &opt);
    ROCKET_CHECK(opts, pair.second, S << "Duplicate option " << ROCKET_QUOTE_BT(name(opt, true)));
    if (opt.shortName) {
      string shortName = static_cast<string>(*opt.shortName);
      validate(shortName, false);
      auto pair = byShortName_.emplace(*opt.shortName, &opt); // cppcheck-suppress shadowVariable
      ROCKET_CHECK(opts, pair.second, S << "Duplicate option " << ROCKET_QUOTE_BT(name(opt, false)));
    }
  }
}

void
CommandLine::apply(const Option& opt, bool nameFlag, optional<string_view> value) {
  if (opt.takesValue && not value)
    throw except::InvalidState(S << "Missing value for option " << ROCKET_QUOTE_BT(name(opt, nameFlag)));
  
  // Usually, options not taking a value may not be assigned a value. There is one exception to this rule:
  // boolean values are allowed
  if (not opt.takesValue && value && not codec::Symbols::Strings::Bool.contains(*value))
    throw except::InvalidState(S << "Option " << ROCKET_QUOTE_BT(name(opt, nameFlag)) << " cannot take a value");

  try {
    opt.apply(value);
  } catch (const exception& ex) {
    string msg = S << "Option " << ROCKET_QUOTE_BT(name(opt, nameFlag)) << ": Invalid value " << *value;
    if (opt.format)
      msg += "; expected " + *opt.format;
    throw except::InvalidState(msg);
  }
}

void
CommandLine::error(ostream& err, int status) const {
  if (usage_)
    printUsage(err);
  if (help_)
    printHelp(err);
  if (status != EXIT_SUCCESS)
    process.exit(status);
}

void
CommandLine::handleException(const exception& ex, ostream& err, int status) const {
  if (auto p = dynamic_cast<const except::Exception*>(&ex))
    process.error(err, p->message(), EXIT_SUCCESS);
  else
    process.error(err, ex.what(), EXIT_SUCCESS);
  
  if (usage_)
    printUsage(err);
  if (help_)
    printHelp(err);
  if (status != EXIT_SUCCESS)
    process.exit(status);
}

void
CommandLine::help(ostream& out, bool exit) {
  ROCKET_EXPECT(help_);

  auto size = terminal::size(out);
  size_t width = max(40UL, size ? size->first : 80UL);
  bool output = params_.otherOutput;
  
  // Usage

  if (usage_) {
    if (output)
      out << '\n';
    printUsage(out);
    output = true;
  }

  // Prolog

  if (params_.prolog) {
    if (output)
      out << '\n';
    out << text::wrap(*params_.prolog, { .width=width }) << '\n';
    output = true;
  }

  // Options

  if (not opts_.empty()) {
    if (output)
      out << '\n';
    helpOpts(out, width);
    output = true;
  }

  // Epilog

  if (params_.epilog) {
    if (output)
      out << '\n';
    out << text::wrap(*params_.epilog, { .width=width }) << '\n';
  }

  if (exit)
    process.exit(EXIT_SUCCESS);
}

/**
 * Option groups appear in the order they are seen. However, no group or group with an empty title comes
 * first. Within the groups, options appear in the order they are seen.
 */
void
CommandLine::helpOpts(ostream& os, size_t width) const {
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
  text::WrapParams wrapParams { .leftIndent=10, .width=width };

  for (const auto* group : groups) {
    const auto& opts = options.find(group)->second;
    if (opts.empty())
      continue;
    if (output)
      os << '\n';
    os << group->title << ":\n\n";
    output = true;

    // Loop through options

    for (const auto* opt : opts) {
      os << "  ";
      if (opt->shortName)
        os << '-' << static_cast<string>(*opt->shortName) << ", ";
      else
        os << "    ";
      os << "--" << opt->name;
      if (opt->format)
        os << " " << *opt->format;
      os << '\n';
      if (opt->help)
        os << text::wrap(*opt->help, wrapParams) << '\n';
    }
  }
}

string
CommandLine::name(const Option& opt, bool nameFlag) {
  return nameFlag ? "--" + opt.name : "-" + static_cast<string>(*opt.shortName);
}

vector<string>
CommandLine::parse(const vector<string>& args, const Take& take) const {
  vector<string> result;
  
  for (auto it = args.begin(), end = args.end(); it != end; ++it) {
    const auto& elem = *it; // `string`
    string_view arg = elem; // This makes `substr()` more efficient

    if (arg == "--") {
      // 1. `--` seen: Pass the rest, excluding the option-end tag, to the program and break the argument
      // loop
      
      result.insert(result.end(), it + 1, args.end());
      break;
    } else if (strings::beginsWith<char>(arg, "--")) {
      // 2. `--...` seen: Parse option by name

      arg = arg.substr(2);
      auto eq = arg.find('=');
      
      // Extract name, look it up in map
      string_view name = eq == string::npos ? arg : arg.substr(0, eq);
      auto mapIt = byName_.find(name);
      if (mapIt == byName_.end())
        throw except::InvalidState(S << "Unknown option " << ROCKET_QUOTE_BT("--" + string(name)));
      const Option& opt = *mapIt->second;

      // Obtain value, if any
      optional<string_view> value;
      if (eq != string::npos) {
        // Take everything after the `=`
        value = arg.substr(eq + 1);
      } else if (opt.takesValue) {
        // Take the next argument
        if (it + 1 != args.end())
          value = *++it;
      }

      // Apply option
      apply(opt, true, value);
    } else if (strings::beginsWith<char>(arg, "-") && arg != "-") {
      // 3. "-..." seen: Parse options by short name; the last one may take a value

      arg = arg.substr(1);
      auto cpIt = unicode::CodePointIterator<char>(arg);
      auto cpEnd = unicode::CodePointIterator<char>(arg, arg.size());
      for (; cpIt != cpEnd; ++cpIt) {
        // Extract short name, look it up in map
        auto cp = *cpIt;
        auto mapIt = byShortName_.find(cp);
        if (mapIt == byShortName_.end())
          throw except::InvalidState(S << "Unknown option " << ROCKET_QUOTE_BT("-" + static_cast<string>(cp)));
        const Option& opt = *mapIt->second;

        // Obtain value, if any, apply option
        optional<string> value;
        auto cpItNext = cpIt + 1;
        if (cpItNext != cpEnd && *cpItNext == U'=') {
          // Option is followed by `=`: Take everything after the `=` and break the code-point loop
          value = arg.substr((++cpItNext).position());
          apply(opt, false, value);
          break;
        } else if (opt.takesValue) {
          // Option takes value: break the code-point loop
          if (cpItNext != cpEnd) {
            // Take the rest of the argument
            value = arg.substr(cpItNext.position());
            apply(opt, false, value);
          }
          else {
            // Take the next argument
            if (it + 1 != args.end())
              value = *++it;
            apply(opt, false, value);
          }
          break;
        } else {
          // No value: Apply, continue in code-point loop
          apply(opt, false, value);
        }
      }
    } else {
      // 4. Not an option, but a positional argument: Let the program take the argument
      
      bool finish = false;
      auto took = take ? take(arg) : Store;
      switch (took) {
      case Accept:
        // Argument was accepted and consumed: nothing to do, continue in argument loop
        break;
      case Stop:
        // Argument was accepted and consumed, but a stop was requested: Pass the rest, excluding the current
        // argument, to the program and break the argument loop
        result.insert(result.end(), it + 1, args.end());
        finish = true;
        break;
      case Store:
        // Argument was not processed, but a store was requested: Pass it to the program and continue in the
        // argument loop
        result.push_back(elem);
        break;
      case Reject:
        // Argument was rejected: Pass the rest, including the current argument, to the program and break the
        // argument loop
        result.insert(result.end(), it, args.end());
        finish = true;
        break;
      }
      if (finish)
        break;
    }
  }
  
  return result;
}

void
CommandLine::printHelp(ostream& os) const {
  ROCKET_EXPECT(help_);

  os << "Try `" << params_.command << " --help` for more information.\n";
}

void
CommandLine::printUsage(ostream& os) const {
  ROCKET_EXPECT(usage_);
  
  os << "Usage: " << params_.command << ' ' << params_.usages[0] << '\n';
  for (size_t i = 1; i < params_.usages.size(); ++i)
    os << "  or   " << params_.command << ' ' << params_.usages[i] << '\n';
}

void
CommandLine::validate(string_view name, bool nameFlag) {
  const char* what = nameFlag ? "option name" : "option short name";
  if (name.empty())
    throw except::InvalidState(S << raw(strings::capitalize(what)) << " may not be empty");
  if (strings::beginsWith<char>(name, "-") || name.find_first_of(" =") != string::npos)
    throw except::InvalidState(S << "Invalid " << what << " " << name);
}

} // namespace rocket::cl

// EOF
