/*
 * log.cc
 */

#include "codec-rocket-decl.h"
#include "codec-std-decl.h"
#include "codec-rocket.h"
#include "codec-std.h"

#include "log.h"

#include "Process.h"
#include "enum.h"
#include "macro.h"

#include <chrono>

using namespace rocket;
using namespace rocket::log;
using namespace std;

namespace {

void applyLog(optional<string_view>);

void applyLogOut(optional<string_view>);

void logFlush(nio::Sink& sink);

void setLogLevel(string_view, string_view);

// `Entry` --------------------------------------------------------------------------------------------------

struct Entry {
  inline Entry(LogLevel* logId, const char* func, string&& begin) :
      logId_(logId), func_(func), begin_(std::move(begin)) {}

  LogLevel* logId_;
  const char* func_;
  optional<string> begin_; // Log entry from `logBegin` that is flushed only if necessary
};

// `Out` ----------------------------------------------------------------------------------------------------

struct Out {
  inline nio::Sink& get() { return sink_ ? *sink_ : *p_; }

  void set(nio::Sink& v) {
    sink_ = &v;
    p_ = nullptr;
  }

  void set(string_view v) {
    sink_ = nullptr;
    // Append to avoid data loss
    p_ = make_unique<nio::FileSink>(string(v), nio::FileSink::Params { .append=true });
    if (not p_->good()) {
      process.error(nio::stderr, EXIT_SUCCESS, "Cannot open log file `{}`; logging to standard output instead", v);
      set(nio::stdout);
    }
  }

private:

  nio::Sink* sink_ = &nio::stdout; // `stdout` or `stderr`
  unique_ptr<nio::FileSink> p_; // A file sink
};

// Local variables1 -----------------------------------------------------------------------------------------

// Command-line-option group
cl::OptionGroup clGroup { "Logging control" };

// Command-line options
vector<cl::Option> clOpts {
  { &clGroup, "log", nullopt, true, "ID[=LEVEL]",
    "set logging for identifier ID to level LEVEL. ID is a known log identifier or `all`. LEVEL is `none`, "
    "`error`, `warn`, `info`, `debug`, or `trace`. If LEVEL is not supplied, `info` is assumed",
    applyLog },
  { &clGroup, "log-out", nullopt, true, "OUT",
    "log to OUT. OUT is `stdout`, `stderr`, a file path, or a URL beginning with `file://`",
    applyLogOut }
};

// Defined log IDs
auto definedIds = rocket::boost::bimap::UnorderedBimap<LogLevel*, string_view>::of();
mutex definedIdsMutex;

// The `Out` instance
Out out;
recursive_mutex outMutex;

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
 * This function handles the `--log-out` option.
 *
 * @ThreadSafe
 */
void
applyLogOut(optional<string_view> v) {
  ROCKET_EXPECT(v);

  ROCKET_LOCK(outMutex);

  if (v == "stdout") {
    out.set(nio::stdout);
  } else if (v == "stderr") {
    out.set(nio::stderr);
  } else {
    if (strings::beginsWith<char>(*v, "file://")) {
      *v = v->substr(7);
    }
    out.set(*v);
  }
}

/**
 * Flushes pending begin log entries.
 */
void
logFlush(nio::Sink& sink) {
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
    sink.write(*it->begin_);
    it->begin_= nullopt;
  }
}

/**
 * @ThreadSafe
 */
void
logImpl(nio::Sink& sink, LogLevel* logId, LogLevel level, size_t stackLevel, string_view msg) {
  // Item: time point in ISO-8601, current time zone, with microseconds
  chrono::time_point tp = time_point_cast<chrono::microseconds>(chrono::system_clock::now());
  chrono::zoned_time zt { chrono::current_zone(), tp };
  string s = std::format("{:%FT%T%Ez} ", zt);
  sink.write(s);
  size_t indentSize = s.size();

  // Item: caller level
  if (level > LogLevel::none) {
    sink.write(fmt::format("[{: <5}] ", level)); // Width is 8
  } else {
    sink.write("        "); // 8 spaces
  }
  indentSize += 8;

  // Item: stack level
  string indent(2 * stackLevel, ' ');
  sink.write(indent);
  indentSize += indent.size();

  // Item: log ID
  {
    ROCKET_LOCK(definedIdsMutex);
    auto it = definedIds.left.find(logId);
    ROCKET_ASSERT(it != definedIds.left.end());
    sink.print("{} ", it->second);
    indentSize += it->second.size() + 1;
  }

  // Item: message
  if (auto lf = msg.find('\n'); lf == string::npos) {
    // Single-line message: just print it
    sink.write(msg);
  } else {
    // Multi-line message: left-adjust
    string localMsg(msg);
    string to = "\n" + string(indentSize, ' ');
    strings::replaceIn<char>(localMsg, "\n", to);
    sink.write(localMsg);
  }

  // Print line feed
  sink.write("\n");
}

// @ThreadSafe
void
setLogLevel(string_view id, string_view value) {
  bool all = id == "all";

  ROCKET_LOCK(definedIdsMutex);

  auto it = definedIds.right.end();
  if (not all) {
    it = definedIds.right.find(id);
    if (it == definedIds.right.end()) {
      except::throwInvalidState(ROCKET_EXCEPT_SL, "Invalid log ID `{}`", id);
    }
  }

  auto is = io::is(value);
  LogLevel level;
  is >> level;
  if (is.fail() || io::tellg(is) != value.size()) {
    except::throwInvalidState(ROCKET_EXCEPT_SL, "Invalid log level `{}`", value);
  }

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

namespace rocket::log {

ROCKET_ENUM_DEFINE(LogLevel, LogLevel, (none)(error)(warn)(info)(debug)(trace));

} // namespace rocket::log

ROCKET_ENUM_DEFINE_FMT_FORMATTER(rocket::log, LogLevel, LogLevel);

namespace rocket::log {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

void init() {
  // We need this in case of quick exit
  process.atExit([] {
    ROCKET_LOCK(outMutex);
    out.get().flush();
  });
}

LogLevel
logDefine(LogLevel* logId, string_view id) {
  ROCKET_CHECK(id, id != "all", "Invalid log ID: \"all\"; this ID is reserved");
  ROCKET_LOCK(definedIdsMutex);
  definedIds.left.insert({ logId, id });
  return LogLevel::none;
}

void
logBegin(LogLevel* logId, const char* func) {
  string buf;
  nio::StringSink sink(buf);
  // Begin log entry will be flushed later if necessary
  logImpl(sink, logId, LogLevel::none, stack.size(), S << func << " {");
  stack.emplace_back(logId, func, std::move(buf));
}

void
logEnd() noexcept {
  // We need to catch everything here to keep the `noexcept` promise
  try {
    // Print end log entry only if begin log entry was flushed
    const Entry& entry = stack.back();
    if (not entry.begin_) {
      ROCKET_LOCK(outMutex);
      logImpl(out.get(), entry.logId_, LogLevel::none, stack.size() - 1, S << "} " << entry.func_);
    }
  } catch (const exception& ex) {
    ROCKET_PROCESS_ERROR("Cannot log message: {}", except::what(ex));
  } catch (...) {
    ROCKET_PROCESS_ERROR("Cannot log message");
  }
  // After catching '...', we can safely pop from the stack
  stack.pop_back();
}

void
log(LogLevel level, const exception& ex) {
  ROCKET_LOCK(outMutex);

  auto& out = ::out.get();
  logFlush(out);
  ostringstream os;
  except::printException(os, ex);
  auto s = os.str();
  string_view msg(s.begin(), s.end() - 1); // Strip '\n'
  logImpl(out, stack.back().logId_, level, stack.size(), msg);
}

void
log(LogLevel level, exception_ptr ptr) {
  ROCKET_LOCK(outMutex);

  auto& out = ::out.get();
  logFlush(out);
  ostringstream os;
  except::printException(os, ptr);
  auto s = os.str();
  string_view msg(s.begin(), s.end() - 1); // Strip '\n'
  logImpl(out, stack.back().logId_, level, stack.size(), msg);
}

void
log(LogLevel level, string_view msg) {
  ROCKET_LOCK(outMutex);

  auto& out = ::out.get();
  logFlush(out);
  logImpl(out, stack.back().logId_, level, stack.size(), msg);
}

const vector<cl::Option>& opts() { return clOpts; }

} // namespace internal

} // namespace rocket::log

// EOF
