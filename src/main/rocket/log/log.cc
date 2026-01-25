/*
 * log.cc
 */

#include "rocket/log/log.h"

#include "rocket/Process.h"
#include "rocket/enum.h"
#include "rocket/macro.h"
#include "rocket/chrono/chrono.h"
#include "rocket/str/str.h"
#include "rocket/system/system.h"

#include <boost/algorithm/string.hpp>

using namespace rocket;
using namespace rocket::log;
using namespace std;

namespace {

// Local constants ------------------------------------------------------------------------------------------

const string ROCKET_LOG_FMT = system::env::get<string>("ROCKET_LOG_FMT").value_or("");

constexpr auto THREAD_WIDTH = 12;
constexpr auto LOG_ID_WIDTH = 16;

const unordered_map<LogLevel, string_view> LEVEL_DISPLAY {
  { LogLevel::none,  "NONE "sv },
  { LogLevel::error, "ERROR"sv },
  { LogLevel::warn,  "WARN "sv },
  { LogLevel::info,  "INFO "sv },
  { LogLevel::debug, "DEBUG"sv },
  { LogLevel::trace, "TRACE"sv },
};

// `Entry` --------------------------------------------------------------------------------------------------

using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock>;

/**
 * A stack entry.
 */
struct Entry {
  LogLevel* logId_;
  const char* function_;
  const char* prettyFunction_;
  const char* file_;
  i32 line_;
  optional<string> begin_; // Log entry from `logBegin` that is flushed only if necessary
  const TimePoint time_;

  inline Entry(
    LogLevel* logId,
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

// `Format` -------------------------------------------------------------------------------------------------

struct Format {
  bool execTimes = true; // x, X
  bool immediate = false; // i, I
  bool prettyFunction = true; // f, F
  u8 secondsRez = 6; // s0, s3, s6, s9
  bool sourceLocation = true; // l, L
  bool threadIds = false; // t, T
  bool utc = false; // z, Z

  constexpr Format(string_view fmt) {
    set(fmt);
  }

  void
  set(string_view fmt) {
    for (auto it = fmt.begin(), end = fmt.end(); it != end; ++it) {
      switch (*it) {
      case 'f': prettyFunction = false; break;
      case 'F': prettyFunction = true; break;
      case 'i': immediate = false; break;
      case 'I': immediate = true; break;
      case 'l': sourceLocation = false; break;
      case 'L': sourceLocation = true; break;
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
      case 't': threadIds = false; break;
      case 'T': threadIds = true; break;
      case 'x': execTimes = false; break;
      case 'X': execTimes = true; break;
      case 'z': utc = false; break;
      case 'Z': utc = true; break;
      default: ROCKET_FAIL("Invalid format specifier {:?}", *it);
      }
    }
  }
};

Format logFmt(ROCKET_LOG_FMT);

// `Out` ----------------------------------------------------------------------------------------------------

/// @NotThreadSafe
struct Out {
  void flushOnExit() {
    if (out_) {
      out_->flush();
    } else if (fileOut_) {
      fileOut_->flush();
    }
  }

  nio::Sink& get(const TimePoint& time);

  void
  set(nio::Sink& out) {
    out_ = &out;
    fileOut_ = nullptr;
  }

  void setPattern(string_view pattern, const TimePoint& time);

private:

  nio::Sink* out_ = &nio::out; // `stdout` or `stderr`
  unique_ptr<nio::FileSink> fileOut_; // A file sink

  string pattern_;
  bool utc_ = false;
  optional<std::chrono::year_month_day> ymd_;
  bool zip_ = false;

  string expand(string_view pattern, const TimePoint& time, bool update);

  std::chrono::year_month_day
  localYmd(const TimePoint& time) const {
    auto localTime = std::chrono::zoned_time(std::chrono::current_zone(), time);
    auto localDays = std::chrono::floor<std::chrono::days>(localTime.get_local_time());
    return std::chrono::year_month_day(localDays);
  }

  std::chrono::year_month_day
  utcYmd(const TimePoint& time) const {
    auto utcDays = std::chrono::floor<std::chrono::days>(time);
    return std::chrono::year_month_day(utcDays);
  }

  void zipYesterday(const TimePoint& time);
};

nio::Sink&
Out::get(const TimePoint& time) {
  if (fileOut_ && ymd_) {
    // Writing to a file: has the date in the pattern changed?
    auto newYmd = utc_ ? utcYmd(time) : localYmd(time);
    if (newYmd != *ymd_) {
      // Date has changed: reassign the pattern, open new log file
      setPattern(pattern_, time);
    }
  }

  return out_ ? *out_ : *fileOut_;
}

string
Out::expand(string_view pattern, const TimePoint& time, bool update) {
  // Prepare the values

  auto localYmd = this->localYmd(time);
  auto localDate = std::format("{}", localYmd);
  auto utcYmd = this->utcYmd(time);
  auto utcDate = std::format("{}", utcYmd);

  auto dir = filesystem::path(process.invocationName()).parent_path().string();
  auto pid = fmt::format("{}", getpid());
  const auto& name = process.name();

  // Expand the pattern

  string ret(pattern);

  auto replaceAll = [](string& str, string_view from, string_view to) -> bool {
    string old(str);
    boost::replace_all(str, from, to);
    return str != old;
  };

  // Start with `@[utc]`
  string_view date;
  boost::replace_all(ret, "@[utc]", "");
  bool utcFound = replaceAll(ret, "@[utc]", "");
  if (utcFound) {
    date = utcDate;
  } else {
    date = localDate;
  }

  // Now do the rest
  bool dateFound = replaceAll(ret, "@[date]", date);
  replaceAll(ret, "@[dir]", dir);
  replaceAll(ret, "@[name]", name);
  replaceAll(ret, "@[pid]", pid);
  bool zipFound = replaceAll(ret, "@[zip]", "");

  // Do sanity checks

  if (zipFound && not dateFound) {
    ROCKET_FAIL("Can only zip yesterday’s log file if a date is present in the pattern");
  }
  if (ret.find("@[") != NPOS) {
    ROCKET_FAIL("Invalid pattern");
  }

  // If requested, update members

  if (update) {
    pattern_ = pattern;
    utc_ = utcFound;
    ymd_ = nullopt;
    if (dateFound) {
      ymd_ = utc_ ? utcYmd : localYmd;
    }
    zip_ = zipFound;
  }

  return ret;
}

void
Out::setPattern(string_view pattern, const TimePoint& time) {
  out_ = nullptr;

  auto path = expand(pattern, time, true);
  // Always append to avoid data loss
  fileOut_ = make_unique<nio::FileSink>(path, nio::FileSink::Params { .append=true });
  if (not fileOut_->good()) {
    process.error(nio::err, 0, "Cannot open log file `{}`; logging to standard output instead", path);
    set(nio::out);
  }

  // If zipping, check if yesterday's log file exists and zip it
  if (zip_) {
    zipYesterday(time);
  }
}

void
Out::zipYesterday(const TimePoint& time) {
  string expanded = expand(pattern_, time - 24h, false);
  filesystem::path path(expanded);
  if (filesystem::exists(path)) {
    system::exec( { "gzip", "-5f", path.string() } );
  }
}

// Local constants ------------------------------------------------------------------------------------------

/// Command-line-option group.
const cl::OptionGroup CL_GROUP { "Logging control" };

void applyLog(optional<string_view>);
void applyLogFmt(optional<string_view>);
void applyLogOut(optional<string_view>);

#define NBSP "\u00A0"

/// Command-line options.
const vector<cl::Option> CL_OPTIONS {
  { &CL_GROUP, "log", nullopt, true, "ID[=LEVEL]",
    "set logging for identifier ID to level LEVEL. ID is a known log identifier or `all`. LEVEL is `none`, "
    "`error`, `warn`, `info`, `debug`, or `trace`. If LEVEL is not supplied, `info` is assumed",
    applyLog },
  { &CL_GROUP, "log-fmt", nullopt, true, "FMT",
    "set log format. FMT is a string of format specifiers, e.g. `fs3Z`. Valid specifiers are:\n"
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
    applyLogFmt },
  { &CL_GROUP, "log-out", nullopt, true, "OUT",
    "log to system device or file. If OUT is `-` or `stdout`, log messages are written to standard output, "
    "which is the default. If OUT is `stderr`, log messages are written to standard error. Otherwise, OUT "
    "is a PATTERN. Examples: `@[name].log`, `@[name]-@[date].log@[zip]`. Inside PATTERN, these placeholders "
    "are available:\n"
    NBSP NBSP "@[date]"     NBSP NBSP NBSP "expands to the current date\n"
    NBSP NBSP "@[dir]" NBSP NBSP NBSP NBSP "expands to the parent directory of the executable\n"
    NBSP NBSP "@[name]"     NBSP NBSP NBSP "expands to the name of the process\n"
    NBSP NBSP "@[pid]" NBSP NBSP NBSP NBSP "expands to the process ID (PID)\n"
    NBSP NBSP "@[utc]"           NBSP NBSP "expands to nothing and uses UTC date rather than local date\n"
    NBSP NBSP "@[zip]" NBSP NBSP NBSP NBSP "expands to nothing and zips yesterday’s log file",
    applyLogOut },
};

// Local variables ------------------------------------------------------------------------------------------

// The log mutex
recursive_mutex logMutex;

// Defined log IDs
auto definedIds = rocket::makeUnorderedBimap<LogLevel*, string_view>();

// The `Out` instance
Out logOut;

/**
 * The function stack.
 *
 * @ThreadSafe
 */
thread_local vector<Entry> logStack;

// Local functions ------------------------------------------------------------------------------------------

/**
 * Handles the `--log` option.
 *
 * @ThreadSafe
 */
void
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
}

/**
 * Handles the `--log-fmt` option.
 *
 * @ThreadSafe
 */
void
applyLogFmt(optional<string_view> val) {
  ROCKET_EXPECT(val);

  // Use public API from here
  setLogFmt(*val);
}

/**
 * Handles the `--log-out` option.
 *
 * @ThreadSafe
 */
void
applyLogOut(optional<string_view> val) {
  ROCKET_EXPECT(val);

  // Use public API from here
  setLogOut(*val);
}

/// @ThreadSafe
string
formatExecTime(const TimePoint& t1, const TimePoint& t2) {
  auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
  if (ns < 1'000) {
    // Display in nanoseconds
    return fmt::format("{} ns", ns);
  } else if (ns < 1'000'000) {
    // Display in microseconds
    return fmt::format("{}.{:0>3} µs", ns / 1'000, ns % 1'000);
  } else if (ns < 1'000'000'000) {
    // Display in milliseconds
    auto µs = ns / 1'000;
    return fmt::format("{}.{:0>3} ms", µs / 1'000, µs % 1'000);
  } else {
    // Display in seconds
    auto ms = ns / 1'000'000;
    return fmt::format("{}.{:0>3} s", ms / 1'000, ms % 1'000);
  }
}

/// @NotThreadSafe
string
formatTimePoint(const TimePoint& tp) {
  auto formatImpl = [](const auto& ctp) {
    // Note we're using `std::format` here because `fmt::format` doesn't support `chrono::zoned_time`
    if (logFmt.utc) {
      return std::format("{:%FT%TZ} ", ctp);
    } else {
      std::chrono::zoned_time zt { std::chrono::current_zone(), ctp };
      return std::format("{:%FT%T%Ez} ", zt);
    }
  };

  if (logFmt.secondsRez == 0) {
    return formatImpl(time_point_cast<std::chrono::seconds>(tp));
  } else if (logFmt.secondsRez == 3) {
    return formatImpl(time_point_cast<std::chrono::milliseconds>(tp));
  } else if (logFmt.secondsRez == 6) {
    return formatImpl(time_point_cast<std::chrono::microseconds>(tp));
  } else {
    return formatImpl(time_point_cast<std::chrono::nanoseconds>(tp));
  }
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

/// @NotThreadSafe
void
logImpl(
  nio::Sink& out,
  LogLevel* logId,
  LogLevel level,
  u64 stackLevel,
  const TimePoint& time,
  string_view msg) {
  // Item: time point
  string str = formatTimePoint(time); // Formats with a trailing space
  out.write(str);
  u64 indent = str.size();

  // Item: thread ID/name
  if (logFmt.threadIds) {
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
  auto it = definedIds.left.find(logId);
  ROCKET_ASSERT(it != definedIds.left.end());
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
    for (auto line : str::split<char>(msg, "\n")) {
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

} // namespace

// `LogLevel` -----------------------------------------------------------------------------------------------

ROCKET_ENUM_DEFINE(rocket::log, LogLevel, LogLevel, (none)(error)(warn)(info)(debug)(trace));

namespace rocket::log {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

/// @ThreadSafe
void
log(LogLevel level, string_view msg) {
  ROCKET_MUTEX_LOCK(logMutex);

  auto time = chrono::now<Clock>();
  auto& out = logOut.get(time);
  logFlush(out);
  logImpl(out, logStack.back().logId_, level, logStack.size(), time, msg);
}

/// @ThreadSafe
void
logBegin(LogLevel* logId, const char* function, const char* prettyFunction, const char* file, i32 line) {
  ROCKET_MUTEX_LOCK(logMutex);

  // Begin log entry will be flushed later if necessary
  string msg = logFmt.prettyFunction ? prettyFunction : function;
  if (logFmt.sourceLocation) {
    msg += fmt::format(" {}:{}", file, line);
  }
  msg += " {";
  nio::StringSink buf;
  auto time = chrono::now<Clock>();
  logImpl(buf, logId, LogLevel::none, logStack.size(), time, msg);
  logStack.emplace_back(logId, function, prettyFunction, file, line, buf.str(), time);

  // If requested, log function stack immediately
  if (logFmt.immediate) {
    logFlush(logOut.get(time));
  }
}

/// @ThreadSafe
LogLevel
logDefine(LogLevel* logId, string_view id) {
  ROCKET_CHECK(id, id != "all", "Invalid log ID \"all\"; this ID is reserved");

  ROCKET_MUTEX_LOCK(logMutex);

  definedIds.left.insert({ logId, id });
  return LogLevel::none;
}

/// @ThreadSafe
void
logEnd() noexcept {
  // We need to catch anything here to keep the `noexcept` promise
  try {
    // Print end log entry only if begin log entry was flushed
    const Entry& entry = logStack.back();
    if (not entry.begin_) {
      ROCKET_MUTEX_LOCK(logMutex);
      string msg = "} ";
      msg += logFmt.prettyFunction ? entry.prettyFunction_ : entry.function_;
      if (logFmt.sourceLocation) {
        msg += fmt::format(" {}:{}", entry.file_, entry.line_);
      }
      auto time = chrono::now<Clock>();
      if (logFmt.execTimes) {
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
  process.atExit([] {
    ROCKET_MUTEX_LOCK(logMutex);
    logOut.flushOnExit();
  }, true);
}

/// @ThreadSafe
const vector<cl::Option>& logOptions() { return CL_OPTIONS; }

} // namespace internal

// Functions ------------------------------------------------------------------------------------------------

/// @ThreadSafe
void
setLogFmt(string_view val) {
  ROCKET_MUTEX_LOCK(logMutex);

  logFmt.set(val);
}

/// @ThreadSafe
void
setLogLevel(string_view id, string_view val) {
  bool all = id == "all";

  ROCKET_MUTEX_LOCK(logMutex);

  auto it = definedIds.right.end();
  if (not all) {
    it = definedIds.right.find(id);
    if (it == definedIds.right.end()) {
      ROCKET_FAIL("Invalid log ID `{}`", id);
    }
  }

  LogLevel level = Enum<LogLevel>::toType(val);

  if (not all) {
    *it->second = level;
  } else {
    // "all"
    for (const auto& pair : definedIds.right) {
      *pair.second = level;
    }
  }
}

/// @ThreadSafe
void
setLogOut(string_view val) {
  ROCKET_MUTEX_LOCK(logMutex);

  if (val == "-" || val == "stdout") {
    logOut.set(nio::out);
  } else if (val == "stderr") {
    logOut.set(nio::err);
  } else {
    logOut.setPattern(val, chrono::now<Clock>());
  }
}

} // namespace rocket::log

// EOF
