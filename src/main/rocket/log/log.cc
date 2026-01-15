/*
 * log.cc
 */

#include "rocket/log/log.h"

#include "rocket/Process.h"
#include "rocket/enum.h"
#include "rocket/macro.h"

#include <chrono>

using namespace rocket;
using namespace rocket::log;
using namespace std;

namespace {

void applyLog(optional<string_view>);

void applyLogFmt(optional<string_view>);

void applyLogOut(optional<string_view>);

void setLogLevel(string_view, string_view);

// `TimePoint ` ---------------------------------------------------------------------------------------------

using TimePoint = chrono::time_point<chrono::system_clock>;

// `Entry` --------------------------------------------------------------------------------------------------

struct Entry {
  LogLevel* logId_;
  const char* function_;
  const char* prettyFunction_;
  const char* file_;
  int line_;
  optional<string> begin_; // Log entry from `logBegin` that is flushed only if necessary
  const TimePoint time_;

  inline Entry(
      LogLevel* logId,
      const char* function,
      const char* prettyFunction,
      const char* file,
      int line,
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
  bool measureTimes = true; // "t": `false`, "T": `true`
  bool prettyFunction = true; // "f": `false`, "F": `true`
  int secondsRez = 6; // "s3": 3 (milliseconds), "s6": 6 (microseconds), "s9": 9 (nanoseconds)
  bool sourceLocation = true; // "l": `false`, "L": `true`
  bool utc = false; // "z": `false`, "Z": `true`

  Format(string_view s) {
    apply(s);
  }

  void apply(string_view s) {
    for (auto it = s.begin(), end = s.end(); it != end; ++it) {
      switch (*it) {
      case 'f': prettyFunction = false; break;
      case 'F': prettyFunction = true; break;
      case 'l': sourceLocation = false; break;
      case 'L': sourceLocation = true; break;
      case 's': {
        if (it == end - 1) {
          ROCKET_FAIL("Missing seconds resolution");
        }
        switch (*++it) {
        case '3': secondsRez = 3; break;
        case '6': secondsRez = 6; break;
        case '9': secondsRez = 9; break;
        default: ROCKET_FAIL("Invalid seconds resolution: {:?}", *it);
        }
        break;
      }
      case 't': measureTimes = false; break;
      case 'T': measureTimes = true; break;
      case 'z': utc = false; break;
      case 'Z': utc = true; break;
      default: ROCKET_FAIL("Invalid format specifier: {:?}", *it);
      }
    }
  }
};

Format logFmt("");

// `Out` ----------------------------------------------------------------------------------------------------

/// @NotThreadSafe
struct Out {
  inline nio::Sink& get() { return out_ ? *out_ : *fileOut_; }

  void set(nio::Sink& v) {
    out_ = &v;
    fileOut_ = nullptr;
  }

  void set(string_view v) {
    out_ = nullptr;
    // Append to avoid data loss
    fileOut_ = make_unique<nio::FileSink>(string(v), nio::FileSink::Params { .append=true });
    if (not fileOut_->good()) {
      process.error(nio::stderr, EXIT_SUCCESS, "Cannot open log file `{}`; logging to standard output instead", v);
      set(nio::stdout);
    }
  }

private:

  nio::Sink* out_ = &nio::stdout; // `stdout` or `stderr`
  unique_ptr<nio::FileSink> fileOut_; // A file sink
};

// Local variables ------------------------------------------------------------------------------------------

// Command-line-option group
cl::OptionGroup clGroup { "Logging control" };

// Command-line options
vector<cl::Option> clOpts {
  { &clGroup, "log", nullopt, true, "ID[=LEVEL]",
    "set logging for identifier ID to level LEVEL. ID is a known log identifier or `all`. LEVEL is `none`, "
    "`error`, `warn`, `info`, `debug`, or `trace`. If LEVEL is not supplied, `info` is assumed",
    applyLog },
  { &clGroup, "log-fmt", nullopt, true, "[f][F][l][L][s3][s6][s9][t][T][z][Z]",
    "set log-format options",
    applyLogFmt },
  { &clGroup, "log-out", nullopt, true, "OUT",
    "log to OUT. OUT is `stdout`, `stderr`, a file path, or a URL beginning with `file://`",
    applyLogOut },
};

// The log mutex
recursive_mutex logMutex;

// Defined log IDs
auto definedIds = rocket::makeUnorderedBimap<LogLevel*, string_view>();

// The `Out` instance
Out out;

// The function stack
thread_local vector<Entry> stack;

// Local functions ------------------------------------------------------------------------------------------

/**
 * This function handles the `--log` option.
 *
 * @ThreadSafe
 */
void
applyLog(optional<string_view> v) {
  ROCKET_EXPECT(v);

  string_view lhs, rhs;
  if (auto eq = v->find('='); eq == string::npos) {
    lhs = *v;
    rhs = "info";
  } else {
    lhs = v->substr(0, eq);
    rhs = v->substr(eq + 1);
  }

  setLogLevel(lhs, rhs);
}

/**
 * This function handles the `--log-fmt` option.
 *
 * @ThreadSafe
 */
void
applyLogFmt(optional<string_view> v) {
  ROCKET_EXPECT(v);

  ROCKET_MUTEX_LOCK(logMutex);

  logFmt.apply(*v);
}

/**
 * This function handles the `--log-out` option.
 *
 * @ThreadSafe
 */
void
applyLogOut(optional<string_view> v) {
  ROCKET_EXPECT(v);

  ROCKET_MUTEX_LOCK(logMutex);

  if (v == "stdout") {
    out.set(nio::stdout);
  } else if (v == "stderr") {
    out.set(nio::stderr);
  } else {
    if (str::beginsWith<char>(*v, "file://")) {
      *v = v->substr(7);
    }
    out.set(*v);
  }
}

/// @NotThreadSafe
string
formatTimePoint(const TimePoint& tp) {
  if (logFmt.secondsRez == 3) {
    chrono::time_point ctp = time_point_cast<chrono::milliseconds>(tp);
    if (logFmt.utc) {
      return std::format("{:%FT%TZ} ", ctp);
    } else {
      chrono::zoned_time zt { chrono::current_zone(), ctp };
      return std::format("{:%FT%T%Ez} ", zt);
    }
  } else if (logFmt.secondsRez == 6) {
    chrono::time_point ctp = time_point_cast<chrono::microseconds>(tp);
    if (logFmt.utc) {
      return std::format("{:%FT%TZ} ", ctp);
    } else {
      chrono::zoned_time zt { chrono::current_zone(), ctp };
      return std::format("{:%FT%T%Ez} ", zt);
    }
  } else {
    chrono::time_point ctp = time_point_cast<chrono::nanoseconds>(tp);
    if (logFmt.utc) {
      return std::format("{:%FT%TZ} ", ctp);
    } else {
      chrono::zoned_time zt { chrono::current_zone(), ctp };
      return std::format("{:%FT%T%Ez} ", zt);
    }
  }
}

/**
 * Flushes pending begin log entries.
 *
 * @NotThreadSafe
 */
void
logFlush(nio::Sink& out) {
  auto begin = stack.end();

  // Look for pending begin log entries
  for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
    if (it->begin_) {
      begin = it.base() - 1; // `base()` is confusing ...
    } else {
      break;
    }
  }

  // Flush them, if any
  for (auto it = begin; it != stack.end(); ++it) {
    out.write(*it->begin_);
    it->begin_= nullopt;
  }
}

/**
 * @NotThreadSafe
 */
void
logImpl(
    nio::Sink& out,
    LogLevel* logId,
    LogLevel level,
    size_t stackLevel,
    const TimePoint& time,
    string_view msg) {
  // Item: time point
  string s = formatTimePoint(time);
  out.write(s);
  size_t indentSize = s.size();

  // Item: log ID
  auto it = definedIds.left.find(logId);
  ROCKET_ASSERT(it != definedIds.left.end());
  auto id = it->second;
  if (id.size() <= 16) {
    out.print("{: <16} ", id);
  } else {
    out.print("{}… ", id.substr(0, 15));
  }
  indentSize += 17; // 16 chars + 1 space

  // Item: log level
  if (level > LogLevel::none) {
    out.write(fmt::format("[{: <5}] ", level)); // Width is 8
  } else {
    out.write("        "); // 8 spaces
  }
  indentSize += 8;

  // Item: Indent by stack level
  string indent(2 * stackLevel, ' ');
  out.write(indent);
  indentSize += indent.size();

  // Item: message
  if (auto lf = msg.find('\n'); lf == string::npos) {
    // Single-line message: just print it
    out.write(msg);
  } else {
    // Multi-line message: left-adjust
    string localMsg(msg);
    string to = "\n" + string(indentSize, ' ');
    str::replaceIn<char>(localMsg, "\n", to);
    out.write(localMsg);
  }

  // Print line feed
  out.write('\n');
}

// @ThreadSafe
void
setLogLevel(string_view id, string_view value) {
  bool all = id == "all";

  ROCKET_MUTEX_LOCK(logMutex);

  auto it = definedIds.right.end();
  if (not all) {
    it = definedIds.right.find(id);
    if (it == definedIds.right.end()) {
      throw InvalidState(fmt::format("Invalid log ID `{}`", id));
    }
  }

  LogLevel level = Enum<LogLevel>::toType(value);

  if (not all) {
    *it->second = level;
  } else {
    // "all"
    for (const auto& pair : definedIds.right) {
      *pair.second = level;
    }
  }
}

} // namespace

// `LogLevel` -----------------------------------------------------------------------------------------------

ROCKET_ENUM_DEFINE(rocket::log, LogLevel, LogLevel, (none)(error)(warn)(info)(debug)(trace));

namespace rocket::log {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

void
init() {
  // We need this in case of quick exit
  process.atExit([] {
    ROCKET_MUTEX_LOCK(logMutex);
    out.get().flush();
  }, true);
}

LogLevel
logDefine(LogLevel* logId, string_view id) {
  ROCKET_CHECK(id, id != "all", "Invalid log ID: \"all\"; this ID is reserved");
  ROCKET_MUTEX_LOCK(logMutex);
  definedIds.left.insert({ logId, id });
  return LogLevel::none;
}

/// @ThreadSafe
void
logBegin(LogLevel* logId, const char* function, const char* prettyFunction, const char* file, int line) {
  ROCKET_MUTEX_LOCK(logMutex);

  // Begin log entry will be flushed later if necessary
  string msg = string(prettyFunction) + " {";
  nio::StringSink buf;
  auto time = chrono::system_clock::now();
  logImpl(buf, logId, LogLevel::none, stack.size(), time, msg);
  stack.emplace_back(logId, function, prettyFunction, file, line, buf.str(), time);
}

/// @ThreadSafe
void
logEnd() noexcept {
  // We need to catch everything here to keep the `noexcept` promise
  try {
    // Print end log entry only if begin log entry was flushed
    const Entry& entry = stack.back();
    if (not entry.begin_) {
      ROCKET_MUTEX_LOCK(logMutex);
      string msg = "} " + string(entry.prettyFunction_);
      auto time = chrono::system_clock::now();
      // XXX Hier measure
      logImpl(out.get(), entry.logId_, LogLevel::none, stack.size() - 1, time, msg);
    }
  } catch (const exception& ex) {
    ROCKET_PROCESS_ERROR("Cannot log message: {}", what(ex));
  } catch (...) {
    ROCKET_PROCESS_ERROR("Cannot log message");
  }
  // After catching `...`, we can safely pop from the stack
  stack.pop_back();
}

/// @ThreadSafe
void
log(LogLevel level, string_view msg) {
  ROCKET_MUTEX_LOCK(logMutex);

  auto& out = ::out.get();
  logFlush(out);
  auto time = chrono::system_clock::now();
  logImpl(out, stack.back().logId_, level, stack.size(), time, msg);
}

const vector<cl::Option>& opts() { return clOpts; }

} // namespace internal

} // namespace rocket::log

// EOF
