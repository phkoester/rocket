/*
 * cl.cc
 */

 #include "cl.h"

#include "rocket/assert.h"
#include "rocket/log/log.h"
#include "rocket/str/str.h"
#include "rocket/system/terminal/terminal.h"
#include "rocket/unicode/Iterator.h"

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
    const auto& logOpts = log::internal::logOptions();
    opts_.insert(opts_.begin(), logOpts.begin(), logOpts.end());
  }

  // NOTE: Because we use string views and pointers in the maps, `opts_` may never be changed from this point

  // Validate options, populate maps

  for (const auto& opt : opts_) {
    if (opt.name == "help")
      help_ = true;
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
CommandLine::apply(const Option& opt, bool nameFlag, optional<string_view> value) {
  // Don't use `ROCKET_CHECK` here for cleaner exception messages

  if (opt.takesValue && not value) {
    ROCKET_FAIL("Missing value for option `{}`", name(opt, nameFlag));
  }

  // Usually, options not taking a value may not be assigned a value. There is one exception to this rule:
  // boolean values are allowed
  static const set<string_view> BOOL_VALUES { "0", "false", "1", "true" };
  if (not opt.takesValue && value && not BOOL_VALUES.contains(*value)) {
    ROCKET_FAIL("Option `{}` cannot take a value", name(opt, nameFlag));
  }

  try {
    opt.apply(value);
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
    ROCKET_FAIL("Option `{}`: Invalid value {:?}{}", name(opt, nameFlag), *value, expected);
  }
}

void
CommandLine::error(nio::Sink& out, i32 status) const {
  if (usage_)
    printUsage(out);
  if (help_)
    printHelp(out);
  if (status != EXIT_SUCCESS)
    process.exit(status);
}

void
CommandLine::handleException(const exception& ex, nio::Sink& out, i32 status) const {
  if (auto p = dynamic_cast<const Exception*>(&ex))
    process.error(out, 0, "{}", p->message());
  else
    process.error(out, 0, "{}", ex.what());

  if (usage_)
    printUsage(out);
  if (help_)
    printHelp(out);
  if (status != EXIT_SUCCESS)
    process.exit(status);
}

void
CommandLine::help(nio::Sink& out, bool exit) {
  ROCKET_EXPECT(help_);

  auto size = system::terminal::size(out);
  // In Windows, a `UL` isn't a `u64`
  u64 width = max(static_cast<u64>(40), size ? size->first : static_cast<u64>(80));
  bool output = params_.otherOutput;

  // Usage

  if (usage_) {
    if (output) {
      out.write('\n');
    }
    printUsage(out);
    output = true;
  }

  // Prolog

  if (params_.prolog) {
    if (output) {
      out.write('\n');
    }
    out.writeln(str::wrap(*params_.prolog, 0, width));
    output = true;
  }

  // Options

  if (not opts_.empty()) {
    if (output) {
      out.write('\n');
    }
    helpOpts(out, width);
    output = true;
  }

  // Epilog

  if (params_.epilog) {
    if (output) {
      out.write('\n');
    }
    out.writeln(str::wrap(*params_.epilog, 0, width));
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
CommandLine::helpOpts(nio::Sink& out, u64 width) const {
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

string
CommandLine::name(const Option& opt, bool nameFlag) {
  return nameFlag ? "--" + opt.name : "-" + static_cast<string>(*opt.shortName);
}

vector<string>
CommandLine::parse(const vector<string>& args, const Take& take) const {
  vector<string> ret;

  for (auto it = args.begin(), end = args.end(); it != end; ++it) {
    const auto& elem = *it; // `string`
    string_view arg = elem; // This makes `substr()` more efficient

    if (arg == "--") {
      // 1. `--` seen: Pass the rest, excluding the option-end tag, to the program and break the argument
      // loop

      ret.insert(ret.end(), it + 1, args.end());
      break;
    } else if (arg.starts_with("--")) {
      // 2. `--...` seen: Parse option by name

      arg = arg.substr(2);
      auto eq = arg.find('=');

      // Extract name, look it up in map
      string_view name = eq == string::npos ? arg : arg.substr(0, eq);
      auto mapIt = byName_.find(name);
      if (mapIt == byName_.end()) {
        ROCKET_FAIL("Unknown option `--{}`", name);
      }
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
    } else if (arg.starts_with("-") && arg != "-") {
      // 3. "-..." seen: Parse options by short name; the last one may take a value

      arg = arg.substr(1);
      auto iter = unicode::Iterator(unicode::IteratorType::Character, arg);
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
        optional<string> value;
        auto segNext = segIt + 1;
        if (segNext != segsEnd && *segNext == "=") {
          // Option is followed by `=`: Take everything after the `=` and break the character loop
          value = unicode::concat(segs, ++segNext - segsBegin);
          apply(opt, false, value);
          break;
        } else if (opt.takesValue) {
          // Option takes value: break the character loop
          if (segNext != segsEnd) {
            // Take the rest of the argument
            value = unicode::concat(segs, segNext - segsBegin);
            apply(opt, false, value);
          }
          else {
            // Take the next argument
            if (it + 1 != args.end()) {
              value = *++it;
            }
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
        ret.insert(ret.end(), it + 1, args.end());
        finish = true;
        break;
      case Store:
        // Argument was not processed, but a store was requested: Pass it to the program and continue in the
        // argument loop
        ret.push_back(elem);
        break;
      case Reject:
        // Argument was rejected: Pass the rest, including the current argument, to the program and break the
        // argument loop
        ret.insert(ret.end(), it, args.end());
        finish = true;
        break;
      }
      if (finish)
        break;
    }
  }

  return ret;
}

void
CommandLine::printHelp(nio::Sink& out) const {
  ROCKET_EXPECT(help_);

  out.println("Try `{} --help` for more information.", params_.command);
}

void
CommandLine::printUsage(nio::Sink& out) const {
  ROCKET_EXPECT(usage_);

  out.println("Usage: {} {}", params_.command, params_.usages[0]);
  for (u64 i = 1; i < params_.usages.size(); ++i) {
    out.println("  or   {} {}", params_.command, params_.usages[i]);
  }
}

void
CommandLine::validate(string_view name, bool nameFlag) {
  // Don't use `ROCKET_CHECK` here for cleaner exception messages

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
