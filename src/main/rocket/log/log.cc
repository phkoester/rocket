/*
 * log.cc
 */

#include "log.h"

#include "rocket/Process.h"
#include "rocket/enum.h"
#include "rocket/macro.h"
#include "rocket/archive/archive.h"
#include "rocket/chrono/chrono.h"
#include "rocket/str/str.h"
#include "rocket/str/escape/escape.h"
#include "rocket/system/system.h"

#include <regex>

using namespace rocket;
using namespace rocket::log;
using namespace std;

namespace fs = std::filesystem;

namespace {

using rocket::log::internal::LogSettings;

// Local constants ------------------------------------------------------------------------------------------

constexpr auto THREAD_WIDTH = 12;
constexpr auto LOG_ID_WIDTH = 16;

const map<LogLevel, string_view> LEVEL_DISPLAY {
  { LogLevel::none,  "NONE "sv },
  { LogLevel::error, "ERROR"sv },
  { LogLevel::warn,  "WARN "sv },
  { LogLevel::info,  "INFO "sv },
  { LogLevel::debug, "DEBUG"sv },
  { LogLevel::trace, "TRACE"sv },
};

// `Entry` --------------------------------------------------------------------------------------------------

/**
 * A stack entry.
 */
struct Entry {
  LogSettings* logId_;
  const char* function_;
  const char* prettyFunction_;
  const char* file_;
  i32 line_;
  optional<string> begin_; // Log entry from `logBegin` that is flushed only if necessary
  TimePoint time_;

  Entry(
    LogSettings* logId,
    const char* function,
    const char* prettyFunction,
    const char* file,
    i32 line,
    const string& begin,
    const TimePoint& time) :
    logId_(logId),
    function_(function),
    prettyFunction_(prettyFunction),
    file_(file),
    line_(line),
    begin_(begin),
    time_(time) {}
};

// `Fmt` ----------------------------------------------------------------------------------------------------

struct Fmt {
  bool usePrettyFunction = true; // f, F
  bool logStackImmediately = false; // i, I
  bool withSourceLocation = true; // l, L
  u8 secondsRez = 6; // s0, s3, s6, s9
  bool withThreadIds = false; // t, T
  bool withExecTimes = true; // x, X
  bool useUtc = false; // z, Z

  void
  set(string_view fmt) {
    for (auto it = fmt.begin(), end = fmt.end(); it != end; ++it) {
      switch (*it) {
        case 'f': usePrettyFunction = false; break;
        case 'F': usePrettyFunction = true; break;
        case 'i': logStackImmediately = false; break;
        case 'I': logStackImmediately = true; break;
        case 'l': withSourceLocation = false; break;
        case 'L': withSourceLocation = true; break;
        case 's': {
          if (it == end - 1) {
            ROCKET_FAIL("Missing seconds resolution");
          }
          switch (*++it) {
            case '0': secondsRez = 0; break;
            case '3': secondsRez = 3; break;
            case '6': secondsRez = 6; break;
            case '9': secondsRez = 9; break;
            default: ROCKET_FAIL("Invalid seconds resolution {:?}", *it);
          }
          break;
        }
        case 't': withThreadIds = false; break;
        case 'T': withThreadIds = true; break;
        case 'x': withExecTimes = false; break;
        case 'X': withExecTimes = true; break;
        case 'z': useUtc = false; break;
        case 'Z': useUtc = true; break;
        default: ROCKET_FAIL("Invalid format specifier {:?}", *it);
      }
    }
  }
};

Fmt logFmt;

// `Out` ----------------------------------------------------------------------------------------------------

/**
 * The log-output sink.
 *
 * This class is not thread-safe, so all usages must be guarded by #logMutex.
 *
 * @NotThreadSafe
 */
struct Out {
  struct ExpandResult {
    string out;
    bool utc = false;
    bool zip = false;
    optional<std::chrono::year_month_day> ymd;
  };

  // This one is publicly available; tests may need it. Throws `InvalidateState`
  static ExpandResult
  expand(string_view pattern, const fs::path& dir, string_view name, i32 pid, const TimePoint& time) {
    ExpandResult ret;
    ret.out = pattern;

    // Detect and eliminate the hints

    ret.utc = str::replaceIn(ret.out, "<utc>", "") > 0;
    ret.zip = str::replaceIn(ret.out, "<zip>", "") > 0;

    // Find out if we have a stable directory and a name with a date

    const fs::path patternPath(ret.out);
    const auto patternDir = patternPath.parent_path();
    const auto patternName = patternPath.filename();

    const string patternDirStr = patternDir.string();
    // `<dir>` and `<name>` are assumed to be stable, while `<date>` and `<pid>` are not
    const bool dirStable = not patternDirStr.contains("<date>") && not patternDirStr.contains("<pid>");
    const bool nameHasDate = patternName.string().contains("<date>");

    // If present, expand `<date>`

    const bool date = ret.out.contains("<date>");
    if (date) {
      // The functions `utcYmd` and `localYmd` must be static and stateless
      ret.ymd = ret.utc ? utcYmd(time) : localYmd(time);
      str::replaceIn(ret.out, {
        { "<date>", fmt::format("{}", *ret.ymd) },
      });
    }

    // Expand the remaining placeholders

    str::replaceIn(ret.out, {
      { "<dir>", dir.string() },
      { "<name>", name },
      { "<pid>", fmt::format("{}", pid) }
    });

    // Do sanity checks

    if (ret.out.empty()) {
      ROCKET_FAIL("Empty pattern");
    }
    // A pattern like `some/path/<date>-<pid>/name.log` works fine; however, in this case where the
    // directory part of the pattern is unstable, we don’t support zipping older log files
    if (ret.zip && not dirStable) {
      ROCKET_FAIL("Can only zip older log files in the same directory");
    }
    // If the pattern is something like `some/path/name-<date>.log`, we can easily look for older zip files
    if (ret.zip && not nameHasDate) {
      ROCKET_FAIL("Can only zip older log files if a date is present in the name part of the pattern");
    }

    return ret;
  }

  void flushOnExit() {
    if (out_ != nullptr) {
      out_->flush();
    } else if (fileOut_) {
      fileOut_->flush();
    }
  }

  nio::Sink& get(const TimePoint& time);

  void set(string_view pattern, const TimePoint& time);

private:

  static fs::path
  expandDir() {
    return process().invocationPath().parent_path();
  }

  static string
  expandName() {
    return process().name();
  }

  static std::chrono::year_month_day
  localYmd(const TimePoint& time) {
    auto localTime = rocket::chrono::zoned_time(rocket::chrono::current_zone(), time);
    auto localDays = std::chrono::floor<std::chrono::days>(localTime.get_local_time());
    return std::chrono::year_month_day(localDays); // NOLINT
  }

  /**
   * Converts the file-name part of a log-output pattern to a regex pattern that is suitable for finding
   * sibling log files in the same directory.
   *
   * This is needed to zip older log files.
   *
   * @param name the file-name part of a log-output pattern; must not contain any file sepatators
   * @return the regex
   */
  static string
  nameRegex(const fs::path& name) {
    ROCKET_ASSERT(not name.has_parent_path(), "File-name pattern must not contain any file separators");

    // Expand `<dir>` and `<name>` to their real values, eliminate the hints `<utc>` and `<zip>`
    string ret(name.string());
    str::replaceIn(ret, {
      { "<dir>", expandDir().string() },
      { "<name>", expandName() },
      { "<utc>", "" },
      { "<zip>", "" }
    });

    // Escape for regex
    ret = str::escape::escapeRegex(ret);

    // Replace `<date>` and `<pid>` with regex patterns
    str::replaceIn(ret, {
      { "<date>", "(\\d{4}-\\d{2}-\\d{2})" }, // `YYYY-MM-DD`, and make this a group
      { "<pid>", "\\d+" }, // One or more digits
    });

    return ret;
  }

  static std::chrono::year_month_day
  utcYmd(const TimePoint& time) {
    auto utcDays = std::chrono::floor<std::chrono::days>(time);
    return std::chrono::year_month_day(utcDays); // NOLINT
  }

  nio::Sink* out_ = &nio::out; // `stdout` or `stderr`
  unique_ptr<nio::FileSink> fileOut_; // a file sink

  string pattern_;
  bool utc_ = false;
  optional<std::chrono::year_month_day> ymd_;
  bool zip_ = false;

  void reset() {
    set(nio::out, "stdout");
  }

  void
  set(nio::Sink& out, string_view pattern) {
    out_ = &out;
    fileOut_ = nullptr;

    pattern_ = pattern;
    utc_ = false;
    ymd_ = {};
    zip_ = false;
  }

  void zipOlderFiles(string_view pattern, const fs::path& current);
};

nio::Sink&
Out::get(const TimePoint& time) {
  if (fileOut_ && ymd_) {
    // Writing to a file, with a date in the pattern: has the date in the pattern changed?
    auto newYmd = utc_ ? utcYmd(time) : localYmd(time);
    if (newYmd != *ymd_) {
      // Date has changed: reassign the pattern, open new log file
      set(pattern_, time);
    }
  }

  return out_ != nullptr ? *out_ : *fileOut_;
}

void
Out::set(string_view val, const TimePoint& time) {
  out_ = nullptr;

  if (val == "-" || val == "stdout") {
    set(nio::out, "stdout");
    return;
  } else if (val == "stderr") {
    set(nio::err, "stderr");
    return;
  }

  // `val` is a pattern

  fs::path path;
  try {
    auto result = expand(val, expandDir(), expandName(), process().id(), time);
    path = result.out;

    pattern_ = val;
    utc_ = result.utc;
    ymd_ = result.ymd;
    zip_ = result.zip;
  } catch (const Exception& ex) {
    process().error(nio::err, 0,
      "Cannot apply log-output pattern `{}`; logging to standard output instead: {}",
      val, ex.message());
    reset();
    return;
  } catch (const exception&) {
    process().error(nio::err, 0, "Cannot apply log-output pattern `{}`; logging to standard output instead", val);
    reset();
    return;
  }

  if (not path.empty()) {
    try {
      fileOut_ = make_unique<nio::FileSink>(path, true); // Append to the file
    } catch (const exception&) {
      process().error(nio::err, 0, "Cannot open log file `{}`; logging to standard output instead", path);
      reset();
      return;
    }
  }

  // Zip older files
  if (zip_) {
    zipOlderFiles(val, path);
  }
}

void
Out::zipOlderFiles(string_view pattern, const fs::path& current) {
  string namePattern = fs::path(pattern).filename().string();
  string nameRegex = Out::nameRegex(namePattern);
  std::regex nameRe(nameRegex);

  auto currentDir = current.parent_path();
  if (currentDir.empty()) {
    currentDir = fs::absolute(current).parent_path();
  }

  ROCKET_ASSERT(ymd_, "A date must be present");
  string currentDate = fmt::format("{}", *ymd_);

  error_code ec;
  fs::directory_iterator it(currentDir, fs::directory_options::skip_permission_denied, ec);
  if (ec) {
    process().error(nio::err, 0, "Cannot scan directory `{}`: {}", currentDir, ec.message());
    return;
  }
  for (; it != fs::directory_iterator(); it.increment(ec)) {
    if (ec) {
      break;
    }
    if (not it->is_regular_file(ec) || ec) {
      continue;
    }

    auto entry = it->path();
    string entryName = entry.filename().string();
    smatch match;
    if (regex_match(entryName, match, nameRe)) {
      // Group 1 is the directory-entry date, `YYYY-MM-DD`
      const string entryDate = match[1].str();
      // Using `YYYY-MM-DD` for the date part of the name, we can compare the strings directly
      if (entryDate < currentDate) {
        // The entry date is older: zip the file
        process().info(nio::err, "Zipping log file `{}` ...", entry);
        const auto gz = entry.string() + ".gz";
        try {
          archive::gzip(entry, gz);
          error_code ec;
          fs::remove(entry, ec);
          if (ec) {
            process().error(nio::err, 0, "Cannot remove log file `{}`: {}", entry, ec.message());
          }
        } catch (const Exception& ex) {
          process().error(nio::err, 0, "Cannot zip log file `{}`: {}", entry, ex.message());
        } catch (const exception& ex) {
          process().error(nio::err, 0, "Cannot zip log file `{}`: {}", entry, ex.what());
        }
      }
    }
  }
}

// Local constants ------------------------------------------------------------------------------------------

/// Command-line-option group.
const cl::OptionGroup logging { "Logging control" };

bool applyLog(optional<string_view> val);
bool applyLogFmt(optional<string_view> val);
bool applyLogOut(optional<string_view> val);

#define NBSP "\u00A0"

/// Command-line options.
const vector<cl::Option> CL_OPTIONS {
  cl::Option::of(cl::OptionType::Custom, {
    .description="set logging for identifier ID to level LEVEL",
    .format="ID[.SUBSTRING][=LEVEL]",
    .group=&logging,
    .minOccurs=0,
    .name="log",
    .takesValue=true,
    .verboseDescription=
      "set logging for identifier ID to level LEVEL. ID is a known log ID or `all`.\n"
      NBSP NBSP "If an optional substring is supplied, the log level is only applied to functions that "
      "contain the substring.\n"
      NBSP NBSP "LEVEL is `none`, `error`, `warn`, `info`, `debug`, or `trace`. If LEVEL is not supplied, "
      "`info` is assumed",
  }, applyLog),
  cl::Option::of(cl::OptionType::Custom, {
    .description="set log format",
    .format="FMT",
    .group=&logging,
    .minOccurs=0,
    .name="log-fmt",
    .takesValue=true,
    .verboseDescription=
      "set log format. FMT is a string of format specifiers, e.g., `fs3Z`. Valid specifiers are:\n"
      NBSP NBSP "f" NBSP NBSP NBSP "display function names\n"
      NBSP NBSP "F" NBSP NBSP NBSP "display pretty function names (*)\n"
      NBSP NBSP "i" NBSP NBSP NBSP "do not log function stack immediately (*)\n"
      NBSP NBSP "I" NBSP NBSP NBSP "log function stack immediately\n"
      NBSP NBSP "l" NBSP NBSP NBSP "do not display source location\n"
      NBSP NBSP "L" NBSP NBSP NBSP "display source location (*)\n"
      NBSP NBSP "s0"     NBSP NBSP "display time with seconds\n"
      NBSP NBSP "s3"     NBSP NBSP "display time with milliseconds\n"
      NBSP NBSP "s6"     NBSP NBSP "display time with microseconds (*)\n"
      NBSP NBSP "s9"     NBSP NBSP "display time with nanoseconds\n"
      NBSP NBSP "t" NBSP NBSP NBSP "do not display thread IDs/names (*)\n"
      NBSP NBSP "T" NBSP NBSP NBSP "display thread IDs/names\n"
      NBSP NBSP "x" NBSP NBSP NBSP "do not display function execution times\n"
      NBSP NBSP "X" NBSP NBSP NBSP "display function execution times (*)\n"
      NBSP NBSP "z" NBSP NBSP NBSP "display local time (*)\n"
      NBSP NBSP "Z" NBSP NBSP NBSP "display UTC time\n"
      "An asterisk (*) indicates that the setting is enabled by default",
  }, applyLogFmt),
  cl::Option::of(cl::OptionType::Custom, {
    .description="log to system device or file",
    .format="OUT",
    .group=&logging,
    .minOccurs=0,
    .name="log-out",
    .takesValue=true,
    .verboseDescription=
      "log to system device or file. If OUT is `-` or `stdout`, log messages are written to standard "
      "output, which is the default. If OUT is `stderr`, log messages are written to standard error. "
      "Otherwise, OUT is a PATTERN. Examples: `<name>.log` or `<name>-<date>.log<zip>`. Inside PATTERN, "
      "these placeholders are available:\n"
      NBSP NBSP "<date>"     NBSP NBSP "expands to the current date\n"
      NBSP NBSP "<dir>" NBSP NBSP NBSP "expands to the parent directory of the executable\n"
      NBSP NBSP "<name>"     NBSP NBSP "expands to the name of the process\n"
      NBSP NBSP "<pid>" NBSP NBSP NBSP "expands to the process ID (PID)\n"
      NBSP NBSP "<utc>" NBSP NBSP NBSP "serves as a hint to use the UTC date rather than the local date\n"
      NBSP NBSP "<zip>" NBSP NBSP NBSP "serves as a hint to zip older log files",
  }, applyLogOut),
};

// Local variables ------------------------------------------------------------------------------------------

// Defined log IDs
UnorderedBimap<LogSettings*, string_view>&
definedIds() {
  static auto ids = rocket::makeUnorderedBimap<LogSettings*, string_view>();
  return ids;
}

// The log mutex
recursive_mutex&
logMutex() {
  static recursive_mutex mutex;
  return mutex;
}

// The `Out` instance
Out logOut;

/**
 * The function stack.
 *
 * @ThreadSafe
 */
thread_local vector<Entry> logStack;

// Local functions ------------------------------------------------------------------------------------------

void setLogFmt(string_view val);
void setLogLevel(string_view id, string_view val);
void setLogOut(string_view val);

/**
 * Handles the `--log` option.
 *
 * @ThreadSafe
 */
bool
applyLog(optional<string_view> val) {
  ROCKET_EXPECT(val);

  string_view lhs, rhs;
  if (auto eq = val->find('='); eq == string::npos) {
    lhs = *val;
    rhs = "info";
  } else {
    lhs = val->substr(0, eq);
    rhs = val->substr(eq + 1);
  }

  // Use public API from here
  setLogLevel(lhs, rhs);
  return true;
}

/**
 * Handles the `--log-fmt` option.
 *
 * @ThreadSafe
 */
bool
applyLogFmt(optional<string_view> val) {
  ROCKET_EXPECT(val);

  // Use public API from here
  setLogFmt(*val);
  return true;
}

/**
 * Handles the `--log-out` option.
 *
 * @ThreadSafe
 */
bool
applyLogOut(optional<string_view> val) {
  ROCKET_EXPECT(val);

  // Use public API from here
  setLogOut(*val);
  return true;
}

// @ThreadSafe
string
formatExecTime(const TimePoint& t1, const TimePoint& t2) {
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
  if (ns < 1'000) {
    // Display in nanoseconds
    return fmt::format("{} ns", ns);
  }
  if (ns < 1'000'000) {
    // Display in microseconds
    return fmt::format("{}.{:0>3} µs", ns / 1'000, ns % 1'000);
  }
  if (ns < 1'000'000'000) {
    // Display in milliseconds
    const auto µs = ns / 1'000;
    return fmt::format("{}.{:0>3} ms", µs / 1'000, µs % 1'000);
  }
  // Display in seconds
  const auto ms = ns / 1'000'000;
  return fmt::format("{}.{:0>3} s", ms / 1'000, ms % 1'000);
}

// @NotThreadSafe
string
formatTimePoint(const TimePoint& tp) {
  const auto formatImpl = [](const auto& ctp) { // ctp = cast time point
    if (logFmt.useUtc) {
      return fmt::format("{:%FT%TZ} ", ctp);
    }
    rocket::chrono::zoned_time zt { rocket::chrono::current_zone(), ctp };
    return fmt::format("{:%FT%T%Ez} ", zt);
  };

  if (logFmt.secondsRez == 0) {
    return formatImpl(time_point_cast<std::chrono::seconds>(tp));
  }
  if (logFmt.secondsRez == 3) {
    return formatImpl(time_point_cast<std::chrono::milliseconds>(tp));
  }
  if (logFmt.secondsRez == 6) {
    return formatImpl(time_point_cast<std::chrono::microseconds>(tp));
  }
  return formatImpl(time_point_cast<std::chrono::nanoseconds>(tp));
}

/**
 * Flushes pending begin log entries.
 *
 * @NotThreadSafe
 */
void
logFlush(nio::Sink& out) {
  auto begin = logStack.end();

  // Look for pending begin log entries
  for (auto it = logStack.rbegin(); it != logStack.rend(); ++it) {
    if (it->begin_) {
      begin = it.base() - 1; // `base()` is confusing ...
    } else {
      break;
    }
  }

  // Flush them, if any
  for (auto it = begin; it != logStack.end(); ++it) {
    out.write(*it->begin_);
    it->begin_= nullopt;
  }
}

// @NotThreadSafe
void
logImpl(
  nio::Sink& out,
  LogSettings* logId,
  LogLevel level,
  u64 stackLevel,
  const TimePoint& time,
  string_view msg) {
  // Item: time point
  const string str = formatTimePoint(time); // Formats with a trailing space
  out.write(str);
  u64 indent = str.size();

  // Item: thread ID/name
  if (logFmt.withThreadIds) {
    auto threadId = this_thread::get_id();
    string str;
    auto name = ROCKET_THREAD_NAME();
    if (not name.empty()) {
      // Use thread name
      str = name;
      if (str.size() > THREAD_WIDTH) {
        str = str.substr(0, THREAD_WIDTH - 1) + "…"; // Cut name on the right side
      }
    } else {
      // Use thread ID
      str = fmt::format("{}", threadId);
      if (str.size() > THREAD_WIDTH) {
        str = "…" + str.substr(str.size() - THREAD_WIDTH + 1); // Cut ID on the left side
      }
    }
    out.print("{: <{}} ", str, THREAD_WIDTH);
    indent += THREAD_WIDTH + 1;
  }

  // Item: log ID
  auto it = definedIds().left.find(logId);
  ROCKET_ASSERT(it != definedIds().left.end());
  auto id = string(it->second);
  if (id.size() > LOG_ID_WIDTH) {
    id = id.substr(0, LOG_ID_WIDTH - 1) + "…"; // Cut ID on the right side
  }
  out.print("{: <{}} ", id, LOG_ID_WIDTH);
  u64 idIndent = indent; // We need this for multi-line messages later
  indent += LOG_ID_WIDTH + 1;

  // Item: log level
  if (level > LogLevel::none) {
    out.write(LEVEL_DISPLAY.at(level));
    out.write(' '); // Width is 6
  } else {
    out.write("      "); // 6 spaces
  }
  indent += 6;

  // Item: Indent by stack level
  out.print("{: <{}}", "", 2 * stackLevel);
  indent += 2 * stackLevel;

  // Item: message
  if (auto lf = msg.find('\n'); lf == string::npos) {
    // Single-line message: just print it
    out.print("{}\n", msg);
  } else {
    // Multi-line message: left-adjust, repeat the log ID for each line
    bool first = true;
    for (auto line : str::lines(msg)) {
      if (first) {
        // Print first line, just like above
        out.print("{}\n", line);
        first = false;
      } else {
        // Print continuing lines, repeat the log ID for each line
        out.print("{: <{}}{: <{}}{}\n", "", idIndent, id, indent - idIndent, line);
      }
    }
  }
}

// @ThreadSafe
void
setLogFmt(string_view val) {
  ROCKET_MUTEX_LOCK(logMutex());

  logFmt.set(val);
}

/**
 * Sets the log level for the log ID @p id to @p val.
 *
 * If @p id is `"all"`, the log level is set for all log IDs.
 *
 * @ThreadSafe
 *
 * @param id the log ID with an optional substring in the form `ID[.SUBSTRING]`, or `"all"`
 * @param val the log level
 */
void
setLogLevel(string_view id, string_view val) {
  ROCKET_MUTEX_LOCK(logMutex());

  string substring;
  const auto dot = id.find('.');
  if (dot != string::npos) {
    substring = id.substr(dot + 1);
    id = id.substr(0, dot);
  }

  if (id.empty()) {
    ROCKET_FAIL("Missing log ID");
  }
  const bool all = id == "all";
  if (all && not substring.empty()) {
    ROCKET_FAIL("Log ID `all` cannot be used with a substring");
  }

  const auto result = Enum<LogLevel>::toType(val, true);
  if (not result) {
    throw InvalidState(result.error().message);
  }
  const auto [_, level] = result.value();

  if (not all) {
    auto it = definedIds().right.find(id);
    if (it == definedIds().right.end()) {
      ROCKET_FAIL("Unknown log ID `{}`", id);
    }
    if (substring.empty()) {
      it->second->level_ = level;
    } else {
      it->second->setSubstringLevel(substring, level);
    }
  } else {
    // "all"
    for (const auto& pair : definedIds().right) {
      pair.second->level_ = level;
    }
  }
}

/**
 * Set the log output.
 *
 * @ThreadSafe
 *
 * @param val `"-"`, `"stdout"`, or `"stderr"`, or a log-output pattern
 */
void
setLogOut(string_view val) {
  ROCKET_MUTEX_LOCK(logMutex());

  logOut.set(val, rocket::chrono::now());
}

} // namespace

// `LogLevel` -----------------------------------------------------------------------------------------------

ROCKET_ENUM_DEFINE(rocket::log, LogLevel, LogLevel, (none)(error)(warn)(info)(debug)(trace));

namespace rocket::log {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

// `Log` ....................................................................................................

/// @ThreadSafe
unique_ptr<Log>
makeLog(
  LogSettings* logId,
  const char* function,
  const char* prettyFunction,
  const char* file,
  i32 line) {
  ROCKET_MUTEX_LOCK(logMutex());
  if (not logId->active()) {
    return {};
  }
  return make_unique<Log>(logId, function, prettyFunction, file, line);
}

// `LogSettings` ............................................................................................

LogLevel
LogSettings::level(const char* function, const char* prettyFunction) const {
  if (substringLevels_.empty()) {
    return level_;
  }
  for (const auto& [substring, level] : substringLevels_) {
    if (strstr(function, substring.c_str()) != nullptr ||
        strstr(prettyFunction, substring.c_str()) != nullptr) {
      return level;
    }
  }
  return level_;
}

void
LogSettings::setSubstringLevel(const string& substring, LogLevel level) {
  const auto it = substringLevels_.find(substring);
  if (it != substringLevels_.end()) {
    substringLevels_.erase(it);
  }
  if (level <= LogLevel::none) {
    return;
  }
  substringLevels_.emplace(substring, level);
}

// Functions ................................................................................................

/// @ThreadSafe
void
log(LogLevel level, string_view msg) {
  ROCKET_MUTEX_LOCK(logMutex());

  auto time = chrono::now<Clock>();
  auto& out = logOut.get(time);
  logFlush(out);
  logImpl(out, logStack.back().logId_, level, logStack.size(), time, msg);
}

/// @ThreadSafe
void
logBegin(LogSettings* logId, const char* function, const char* prettyFunction, const char* file, i32 line) {
  ROCKET_MUTEX_LOCK(logMutex());

  // Begin log entry will be flushed later if necessary
  string msg = logFmt.usePrettyFunction ? prettyFunction : function;
  if (logFmt.withSourceLocation) {
    msg += fmt::format(" {}:{}", file, line);
  }
  msg += " {";
  nio::StringSink buf;
  auto time = chrono::now<Clock>();
  logImpl(buf, logId, LogLevel::none, logStack.size(), time, msg);
  logStack.emplace_back(logId, function, prettyFunction, file, line, buf.str(), time);

  // If requested, log function stack immediately
  if (logFmt.logStackImmediately) {
    logFlush(logOut.get(time));
  }
}

/// @ThreadSafe
LogSettings
logDefine(LogSettings* logId, string_view id) {
  ROCKET_CHECK(id, id != "all", "Invalid log ID `all`; this ID is reserved");

  ROCKET_MUTEX_LOCK(logMutex());

  definedIds().left.insert({ logId, id });
  return {};
}

/// @ThreadSafe
void
logEnd() {
  // We need to catch anything here, because this is called in a destructor
  try {
    // Print end log entry only if begin log entry was flushed
    const Entry& entry = logStack.back();
    if (not entry.begin_) {
      ROCKET_MUTEX_LOCK(logMutex());
      string msg = "} ";
      msg += logFmt.usePrettyFunction ? entry.prettyFunction_ : entry.function_;
      if (logFmt.withSourceLocation) {
        msg += fmt::format(" {}:{}", entry.file_, entry.line_);
      }
      auto time = chrono::now<Clock>();
      if (logFmt.withExecTimes) {
        msg += " [";
        msg += formatExecTime(entry.time_, time);
        msg += ']';
      }
      logImpl(logOut.get(time), entry.logId_, LogLevel::none, logStack.size() - 1, time, msg);
    }
  } catch (const exception& ex) {
    ROCKET_PROCESS_ERROR(0, "Cannot log message: {}", what(ex));
  } catch (...) {
    ROCKET_PROCESS_ERROR(0, "Cannot log message");
  }
  // After catching anything, we can safely pop from the stack
  logStack.pop_back();
}

/// @ThreadSafe
void
logInit() {
  // We need this in case of quick exit
  process().atExit([] {
    ROCKET_MUTEX_LOCK(logMutex());
    logOut.flushOnExit();
  }, true);

  // Apply default log options from environment variable `ROCKET_LOG`
  vector<string> args;
  const string val = system::env::get<string>("ROCKET_LOG").value_or("");
  try {
    args = system::makeArgs(val);
  } catch (const Exception& ex) {
    ROCKET_PROCESS_ERROR(EXIT_SERIOUS_FAILURE, "Cannot parse `ROCKET_LOG`, which is {:?}: {}", val, ex.message());
  }
  if (not args.empty()) {
    cl::CommandLine cl(CL_OPTIONS, {}, { .rocketOpts = false });
    cl.parse(args);
  }
}

/// @ThreadSafe
const vector<cl::Option>& logOptions() { return CL_OPTIONS; }

} // namespace internal

// Functions ------------------------------------------------------------------------------------------------

// Needs no mutex log, calls static functions only
string
expandLogOut(string_view val, const fs::path& dir, string_view name, i32 pid, const TimePoint& time) {
  if (val == "-" || val == "stdout" || val == "stderr") {
    return string(val);
  }
  return Out::expand(val, dir, name, pid, time).out;
}

} // namespace rocket::log

// EOF
