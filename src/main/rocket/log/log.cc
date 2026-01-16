/*
 * log.cc
 */

#include "rocket/log/log.h"

#include "rocket/Process.h"
#include "rocket/enum.h"
#include "rocket/macro.h"

#include <boost/tokenizer.hpp>

#include <chrono>

using namespace rocket;
using namespace rocket::log;
using namespace std;

namespace {

void applyLog(optional<string_view>);

void applyLogFmt(optional<string_view>);

void applyLogOut(optional<string_view>);

void setLogLevel(string_view, string_view);

// Local constants ------------------------------------------------------------------------------------------

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

// `TimePoint ` ---------------------------------------------------------------------------------------------

using TimePoint = chrono::time_point<chrono::system_clock>;

// `Entry` --------------------------------------------------------------------------------------------------

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
  bool prettyFunction = true; // f, F
  u8 secondsRez = 6; // s0, s3, s6, s9
  bool sourceLocation = true; // l, L
  bool threadIds = false; // t, T
  bool utc = false; // z, Z

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

Format logFmt;

// `Out` ----------------------------------------------------------------------------------------------------

/// @NotThreadSafe
struct Out {
  inline nio::Sink& get() { return out_ ? *out_ : *fileOut_; }

  void
  set(nio::Sink& v) {
    out_ = &v;
    fileOut_ = nullptr;
  }

  void
  set(string_view v) {
    out_ = nullptr;
    // Always append to avoid data loss
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

#define NBSP "\u00A0"

// Command-line options
vector<cl::Option> clOpts {
  { &clGroup, "log", nullopt, true, "ID[=LEVEL]",
    "set logging for identifier ID to level LEVEL. ID is a known log identifier or `all`. LEVEL is `none`, "
    "`error`, `warn`, `info`, `debug`, or `trace`. If LEVEL is not supplied, `info` is assumed",
    applyLog },
  { &clGroup, "log-fmt", nullopt, true, "FMT",
    "set log format. FMT is a string of format specifiers, e.g. `fs3Z`. Valid specifiers are:\n"
    NBSP NBSP "f" NBSP NBSP NBSP "display function names\n"
    NBSP NBSP "F" NBSP NBSP NBSP "display pretty function names (*)\n"
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

/**
 * The function stack.
 *
 * @ThreadSafe
 */
thread_local vector<Entry> stack;

// Local functions ------------------------------------------------------------------------------------------

/**
 * Handles the `--log` option.
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
 * Handles the `--log-fmt` option.
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
 * Handles the `--log-out` option.
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
formatExecTime(const TimePoint& t1, const TimePoint& t2) {
  auto ns = chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count();
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
      chrono::zoned_time zt { chrono::current_zone(), ctp };
      return std::format("{:%FT%T%Ez} ", zt);
    }
  };

  if (logFmt.secondsRez == 0) {
    return formatImpl(time_point_cast<chrono::seconds>(tp));
  } else if (logFmt.secondsRez == 3) {
    return formatImpl(time_point_cast<chrono::milliseconds>(tp));
  } else if (logFmt.secondsRez == 6) {
    return formatImpl(time_point_cast<chrono::microseconds>(tp));
  } else {
    return formatImpl(time_point_cast<chrono::nanoseconds>(tp));
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
  string s = formatTimePoint(time); // Formats with a trailing space
  out.write(s);
  u64 indent = s.size();

  // Item: thread ID/name
  if (logFmt.threadIds) {
    auto threadId = this_thread::get_id();
    string s;
    auto name = ROCKET_THREAD_NAME();
    if (not name.empty()) {
      // Use thread name
      s = name;
      if (s.size() > THREAD_WIDTH) {
        s = s.substr(0, THREAD_WIDTH - 1) + "…"; // Cut name on the right side
      }
    } else {
      // Use thread ID
      s = fmt::format("{}", threadId);
      if (s.size() > THREAD_WIDTH) {
        s = "…" + s.substr(s.size() - THREAD_WIDTH + 1); // Cut ID on the left side
      }
    }
    out.print("{: <{}} ", s, THREAD_WIDTH);
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
    // XXX str::split
    boost::char_separator<char> sep("\n", nullptr, boost::keep_empty_tokens);
    bool first = true;
    for (const auto& line : boost::tokenizer(msg, sep)) {
      if (first) {
        out.print("{}\n", line);
        first = false;
      } else {
        out.print("{: <{}}{: <{}}{}\n", "", idIndent, id, indent - idIndent, line);
      }
    }
  }
}

/// @ThreadSafe
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
logBegin(LogLevel* logId, const char* function, const char* prettyFunction, const char* file, i32 line) {
  ROCKET_MUTEX_LOCK(logMutex);

  // Begin log entry will be flushed later if necessary
  string msg = logFmt.prettyFunction ? prettyFunction : function;
  if (logFmt.sourceLocation) {
    msg += fmt::format(" {}:{}", file, line);
  }
  msg += " {";
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
      string msg = "} ";
      msg += logFmt.prettyFunction ? entry.prettyFunction_ : entry.function_;
      if (logFmt.sourceLocation) {
        msg += fmt::format(" {}:{}", entry.file_, entry.line_);
      }
      auto time = chrono::system_clock::now();
      if (logFmt.execTimes) {
        msg += " [";
        msg += formatExecTime(entry.time_, time);
        msg += ']';
      }
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
