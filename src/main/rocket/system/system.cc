/*
 * system.cc
 */

#include "system.h"

#include "rocket/InputFailure.h"
#include "rocket/assert.h"
#include "rocket/scope.h"

#ifdef ROCKET_OS_WINDOWS
  #include "rocket/windows.h"
  #include <thread>
#else
  #include <fcntl.h>
  #include <poll.h>
  #include <spawn.h>
  #include <sys/wait.h>
  #include <unistd.h>
#endif

#include <array>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <optional>
#include <system_error>
#include <utility>

using namespace rocket;
using namespace rocket::system;
using namespace std;

// Global variables -----------------------------------------------------------------------------------------

#ifndef ROCKET_OS_WINDOWS
extern char** environ;
#endif

namespace {

#ifdef ROCKET_OS_WINDOWS

// `ConsoleModeGuard` ---------------------------------------------------------------------------------------

/**
 * A guard that restores the console mode of `stdin` and `stdout`.
 */
struct ConsoleModeGuard {
  ConsoleModeGuard() {
    for (auto& entry : entries_) {
      entry.handle = CreateFileA(
        entry.name,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);
      if (entry.handle == INVALID_HANDLE_VALUE) {
        continue; // No console attached
      }
      DWORD mode = 0;
      if (GetConsoleMode(entry.handle, &mode) != 0) {
        entry.mode = mode;
      }
    }
  }

  ConsoleModeGuard(const ConsoleModeGuard&) = delete;

  ~ConsoleModeGuard() {
    for (const auto& entry : entries_) {
      if (entry.handle == INVALID_HANDLE_VALUE) {
        continue;
      }
      if (entry.mode) {
        SetConsoleMode(entry.handle, *entry.mode);
      }
      CloseHandle(entry.handle);
    }
  }

  ConsoleModeGuard& operator=(const ConsoleModeGuard&) = delete;

private:

  struct Entry {
    const char* name;
    HANDLE handle = INVALID_HANDLE_VALUE;
    optional<DWORD> mode = {};
  };

  array<Entry, 2> entries_ { {
    { .name="CONIN$" },  // `stdin`
    { .name="CONOUT$" }, // `stdout` and `stderr` (same screen buffer)
  } };
};

// `Handle` -------------------------------------------------------------------------------------------------

/**
 * Windows: A handle that closes itself.
 */
struct Handle {
  Handle() = default;

  explicit Handle(HANDLE handle) : handle_(handle) {}

  Handle(const Handle&) = delete;

  Handle(Handle&& rhs) noexcept : handle_(exchange(rhs.handle_, nullptr)) {}

  ~Handle() { reset(); }

  Handle& operator=(const Handle&) = delete;

  Handle&
  operator=(Handle&& rhs) noexcept {
    if (this != &rhs) {
      reset();
      handle_ = exchange(rhs.handle_, nullptr);
    }
    return *this;
  }

  [[nodiscard]] HANDLE get() const { return handle_; }

  void
  reset() {
    if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(handle_);
    }
    handle_ = nullptr;
  }

private:

  HANDLE handle_ = nullptr;
};

#else

// `Fd` -----------------------------------------------------------------------------------------------------

/**
 * Linux, macOS: A file descriptor that closes itself.
 */
class Fd {
public:

  Fd() = default;

  explicit Fd(i32 fd) : fd_(fd) {}

  Fd(const Fd&) = delete;

  Fd(Fd&& rhs) noexcept : fd_(exchange(rhs.fd_, -1)) {}

  ~Fd() { reset(); }

  Fd& operator=(const Fd&) = delete;

  Fd&
  operator=(Fd&& rhs) noexcept {
    if (this != &rhs) {
      reset();
      fd_ = exchange(rhs.fd_, -1);
    }
    return *this;
  }

  explicit operator bool() const { return fd_ >= 0; }

  [[nodiscard]] i32 get() const { return fd_; }

  void
  reset() {
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
  }

private:

  i32 fd_ = -1;
};

#endif // ROCKET_OS_WINDOWS

// Local functions ------------------------------------------------------------------------------------------

recursive_mutex&
envMutex() {
  static recursive_mutex mutex;
  return mutex;
}

#ifdef ROCKET_OS_WINDOWS

/**
 * Windows: Returns the error of the last failed system call.
 */
error_code
lastError() {
  return { static_cast<i32>(GetLastError()), system_category() };
}

/**
 * Windows: Returns the path of `cmd.exe` in the system directory.
 *
 * Taking it from there rather than searching for it keeps a `cmd.exe` in the current directory from being
 * run.
 */
error_code
cmdPath(string& path) {
  array<char, MAX_PATH> buf {};
  const UINT n = GetSystemDirectoryA(buf.data(), static_cast<UINT>(buf.size()));
  if (n == 0) {
    return lastError();
  }
  if (n >= buf.size()) {
    return make_error_code(errc::filename_too_long);
  }
  path.assign(buf.data(), n).append("\\cmd.exe");
  return {};
}

/**
 * Windows: Returns whether @p program names a batch file.
 *
 * Windows ignores trailing dots and spaces in file names, so `a.bat. ` names a batch file as well.
 */
bool
isBatchFile(string_view program) {
  while (not program.empty() && (program.back() == '.' || program.back() == ' ')) {
    program.remove_suffix(1);
  }
  if (program.size() < 4) {
    return false;
  }
  string ext(program.substr(program.size() - 4));
  ranges::transform(ext, ext.begin(), [](char c) { return static_cast<char>(tolower(static_cast<u8>(c))); });
  return ext == ".bat" || ext == ".cmd";
}

/**
 * Windows: Creates a pipe whose write end a command can inherit, but not its read end.
 */
error_code
makePipe(Handle& readEnd, Handle& writeEnd) {
  SECURITY_ATTRIBUTES sa {
    .nLength = static_cast<DWORD>(sizeof(SECURITY_ATTRIBUTES)),
    .lpSecurityDescriptor = nullptr,
    .bInheritHandle = TRUE,
  };
  HANDLE r = nullptr;
  HANDLE w = nullptr;
  if (CreatePipe(&r, &w, &sa, 0) == 0) {
    return lastError();
  }
  readEnd = Handle(r);
  writeEnd = Handle(w);
  if (SetHandleInformation(readEnd.get(), HANDLE_FLAG_INHERIT, 0) == 0) {
    return lastError();
  }
  return {};
}

/**
 * Windows: Creates an inheritable handle for the stdin of a command.
 *
 * This is a duplicate of the own stdin, so the command reads from the same source as `popen` would let it.
 * Without an own stdin, as in a GUI application, it is the null device.
 */
error_code
makeStdin(Handle& handle) {
  const HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
  if (in != nullptr && in != INVALID_HANDLE_VALUE) {
    HANDLE dup = nullptr;
    const HANDLE self = GetCurrentProcess();
    if (DuplicateHandle(self, in, self, &dup, 0, TRUE, DUPLICATE_SAME_ACCESS) != 0) {
      handle = Handle(dup);
      return {};
    }
  }

  SECURITY_ATTRIBUTES sa {
    .nLength = static_cast<DWORD>(sizeof(SECURITY_ATTRIBUTES)),
    .lpSecurityDescriptor = nullptr,
    .bInheritHandle = TRUE,
  };
  const HANDLE nul = CreateFileA(
    "NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, nullptr);
  if (nul == INVALID_HANDLE_VALUE) {
    return lastError();
  }
  handle = Handle(nul);
  return {};
}

/**
 * Windows: Quotes an argument such that the C runtime of the called program reads it back unchanged.
 *
 * Backslashes are only special in front of a quotation mark: there, they must be doubled. This includes the
 * closing quotation mark. A quotation mark inside the argument is escaped with one more backslash.
 */
string
quoteArg(string_view arg) {
  string ret = "\"";
  u64 backslashes = 0;

  for (const char c : arg) {
    if (c == '\\') {
      ++backslashes; // Decide how to write them when we see what follows
      continue;
    }
    ret.append(c == '"' ? 2 * backslashes + 1 : backslashes, '\\');
    ret.push_back(c);
    backslashes = 0;
  }

  ret.append(2 * backslashes, '\\');
  ret.push_back('"');
  return ret;
}

/**
 * Windows: Converts arguments to a command line for `CreateProcess` that passes each argument literally.
 *
 * No shell is involved, so only the quoting rules of the C runtime apply. They differ for the program, the
 * first argument: it ends at the next quotation mark, and backslashes have no special meaning in it. It is
 * therefore just enclosed in quotation marks, and may not contain one itself.
 *
 * A batch file is refused: `CreateProcess` would hand it to `cmd.exe`, which applies its own rules to the
 * arguments. No quoting is known to make them safe there.
 */
string
makeCl(const vector<string_view>& args) {
  string ret;

  for (u64 i = 0; i < args.size(); ++i) {
    const string_view arg = args[i];
    ROCKET_CHECK(args, not arg.contains('\0'), "May not contain NUL characters");

    if (i == 0) {
      ROCKET_CHECK(args, not arg.contains('"'), "Program may not contain quotation marks on Windows");
      ROCKET_CHECK(args, not isBatchFile(arg), "Program may not be a batch file on Windows");
      ret.push_back('"');
      ret.append(arg);
      ret.push_back('"');
    } else {
      ret.push_back(' ');
      ret.append(quoteArg(arg));
    }
  }

  return ret;
}

/**
 * Windows: Reads from @p pipe until the writing side closes it, and closes it then.
 *
 * On a read error, the pipe is closed as well, so the command fails to write rather than block.
 */
error_code
readPipe(Handle& pipe, string& out) {
  array<char, 4'096> buf {};
  for (;;) {
    DWORD n = 0;
    if (ReadFile(pipe.get(), buf.data(), static_cast<DWORD>(buf.size()), &n, nullptr) == 0) {
      const DWORD error = GetLastError();
      pipe.reset();
      if (error == ERROR_BROKEN_PIPE) {
        return {}; // The writing side is closed: this is the end of the output
      }
      return { static_cast<i32>(error), system_category() };
    }
    // A read of 0 bytes is not the end: the command may have written 0 bytes
    out.append(buf.data(), n);
  }
}

/**
 * Windows: Runs a command line and captures its output.
 *
 * @param app the full path of the program to run, or null to take the program from @p cl and search
 *   for it
 * @param cl the command line, starting with the program
 * @param combined whether to capture the output to stderr together with that to stdout
 */
ExecResult
execImpl(const char* app, string cl, bool combined) {
  ExecResult ret;

  // Auto-restore console modes that the command might compromise
  const ConsoleModeGuard cmg;

  Handle in;
  Handle outRead;
  Handle outWrite;
  Handle errRead;
  Handle errWrite;
  if (auto ec = makeStdin(in); ec) {
    ret.error = ec;
    return ret;
  }
  if (auto ec = makePipe(outRead, outWrite); ec) {
    ret.error = ec;
    return ret;
  }
  if (not combined) {
    if (auto ec = makePipe(errRead, errWrite); ec) {
      ret.error = ec;
      return ret;
    }
  }

  // Let the command inherit these handles only. Without the list, it would inherit all inheritable handles,
  // including the pipe ends of commands that other threads start meanwhile. They would not see the end of
  // the output until this command exits
  vector<HANDLE> handles { in.get(), outWrite.get() };
  if (not combined) {
    handles.push_back(errWrite.get());
  }

  SIZE_T size = 0;
  InitializeProcThreadAttributeList(nullptr, 1, 0, &size); // Fails, but tells the size
  vector<std::byte> attrsBuf(size);
  auto* attrs = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrsBuf.data());
  if (InitializeProcThreadAttributeList(attrs, 1, 0, &size) == 0) {
    ret.error = lastError();
    return ret;
  }
  ROCKET_SCOPE_EXIT([&] { DeleteProcThreadAttributeList(attrs); });
  if (UpdateProcThreadAttribute(
    attrs,
    0,
    PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
    handles.data(),
    handles.size() * sizeof(HANDLE),
    nullptr,
    nullptr) == 0) {
    ret.error = lastError();
    return ret;
  }

  STARTUPINFOEXA si {};
  si.StartupInfo.cb = static_cast<DWORD>(sizeof(si));
  si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
  si.StartupInfo.hStdInput = in.get();
  si.StartupInfo.hStdOutput = outWrite.get();
  si.StartupInfo.hStdError = combined ? outWrite.get() : errWrite.get();
  si.lpAttributeList = attrs;

  PROCESS_INFORMATION pi {};
  {
    // The command inherits the environment, which may not change meanwhile
    ROCKET_MUTEX_LOCK(envMutex());
    if (CreateProcessA(
      app,
      cl.data(),
      nullptr,
      nullptr,
      TRUE,
      EXTENDED_STARTUPINFO_PRESENT,
      nullptr,
      nullptr,
      &si.StartupInfo,
      &pi) == 0) {
      ret.error = lastError();
      return ret;
    }
  }
  const Handle process(pi.hProcess);
  CloseHandle(pi.hThread);

  // Close the own copies of the handles the command uses. Only then is the end of its output seen
  in.reset();
  outWrite.reset();
  errWrite.reset();

  // Read both pipes at the same time. Otherwise, the command could block writing to one of them, while we
  // wait for the other one. Anonymous pipes cannot be read asynchronously, so stderr is read by a thread
  error_code errError;
  jthread errReader;
  if (not combined) {
    try {
      errReader = jthread([&] { errError = readPipe(errRead, ret.err); });
    } catch (const system_error& ex) {
      errError = ex.code();
      errRead.reset(); // The command fails to write to stderr rather than block
    }
  }
  const error_code outError = readPipe(outRead, ret.out);
  if (errReader.joinable()) {
    errReader.join();
  }
  ret.error = outError ? outError : errError;

  if (WaitForSingleObject(process.get(), INFINITE) == WAIT_FAILED) {
    if (not ret.error) {
      ret.error = lastError();
    }
    return ret;
  }
  DWORD code = 0;
  if (GetExitCodeProcess(process.get(), &code) == 0) {
    if (not ret.error) {
      ret.error = lastError();
    }
    return ret;
  }
  ret.status = static_cast<i32>(code);

  return ret;
}

#else

/**
 * Linux, macOS: Returns the error of the last failed system call.
 */
error_code
lastError() {
  return { errno, system_category() };
}

/**
 * Linux, macOS: Creates a pipe whose ends are closed when a command is executed.
 *
 * The caller must hold `envMutex()` until it has started the command, see `execImpl`.
 */
error_code
makePipe(Fd& readEnd, Fd& writeEnd) {
  array<i32, 2> fds {};

#ifdef ROCKET_OS_MACOS
  // macOS has no `pipe2`, so the flag is set separately
  if (pipe(fds.data()) != 0) {
    return lastError();
  }
  readEnd = Fd(fds[0]);
  writeEnd = Fd(fds[1]);
  for (const i32 fd : fds) {
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
      return lastError();
    }
  }
#else
  if (pipe2(fds.data(), O_CLOEXEC) != 0) {
    return lastError();
  }
  readEnd = Fd(fds[0]);
  writeEnd = Fd(fds[1]);
#endif

  // With stdin, stdout, or stderr closed, an end may get one of their numbers. Then, redirecting stdout and
  // stderr of the command could overwrite it before it is used. Move it out of the way
  for (Fd* fd : { &readEnd, &writeEnd }) {
    if (fd->get() <= STDERR_FILENO) {
      const i32 newFd = fcntl(fd->get(), F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
      if (newFd == -1) {
        return lastError();
      }
      *fd = Fd(newFd);
    }
  }

  return {};
}

/**
 * Linux, macOS: Reads from the pipes @p out and @p err until the writing side closes them, and closes them
 * then.
 *
 * Both pipes are read at the same time. Otherwise, the command could block writing to one of them, while we
 * wait for the other one. A pipe that is not open is skipped.
 */
error_code
readPipes(Fd& out, string& outStr, Fd& err, string& errStr) {
  array<char, 4'096> buf {};

  while (out || err) {
    array<pollfd, 2> fds {};
    array<pair<Fd*, string*>, 2> targets {};
    nfds_t n = 0;
    for (auto [fd, str] : { pair { &out, &outStr }, pair { &err, &errStr } }) {
      if (*fd) {
        fds[n] = { .fd = fd->get(), .events = POLLIN, .revents = 0 };
        targets[n] = { fd, str };
        ++n;
      }
    }

    if (poll(fds.data(), n, -1) == -1) {
      if (errno == EINTR) {
        continue;
      }
      return lastError();
    }

    for (nfds_t i = 0; i < n; ++i) {
      if (fds[i].revents == 0) {
        continue;
      }
      const ssize_t count = read(fds[i].fd, buf.data(), buf.size());
      if (count > 0) {
        targets[i].second->append(buf.data(), static_cast<size_t>(count));
      } else if (count == 0) {
        targets[i].first->reset(); // The writing side is closed: this is the end of the output
      } else if (errno != EINTR) {
        return lastError();
      }
    }
  }

  return {};
}

/**
 * Linux, macOS: Runs a command and captures its output.
 *
 * @param path the path of the program to run, or `nullptr` to search for `args[0]` in the `PATH`
 * @param args the arguments, starting with the program name
 * @param combined whether to capture the output to stderr together with that to stdout
 */
ExecResult
execImpl(const char* path, vector<string> args, bool combined) {
  ExecResult ret;

  vector<char*> argv;
  argv.reserve(args.size() + 1);
  for (auto& arg : args) {
    argv.push_back(arg.data());
  }
  argv.push_back(nullptr);

  Fd outRead;
  Fd outWrite;
  Fd errRead;
  Fd errWrite;
  pid_t pid = 0;

  {
    // The command inherits the environment, which may not change meanwhile. On macOS, holding the lock from
    // creating the pipes on also keeps commands started by other threads from inheriting the pipe ends
    // before they are marked to be closed. They would not see the end of the output until those commands
    // exit
    ROCKET_MUTEX_LOCK(envMutex());

    if (auto ec = makePipe(outRead, outWrite); ec) {
      ret.error = ec;
      return ret;
    }
    if (not combined) {
      if (auto ec = makePipe(errRead, errWrite); ec) {
        ret.error = ec;
        return ret;
      }
    }

    posix_spawn_file_actions_t actions;
    if (const i32 rc = posix_spawn_file_actions_init(&actions); rc != 0) {
      ret.error = { rc, system_category() };
      return ret;
    }
    ROCKET_SCOPE_EXIT([&] { posix_spawn_file_actions_destroy(&actions); });

    const i32 errFd = combined ? outWrite.get() : errWrite.get();
    for (auto [from, to] : { pair { outWrite.get(), STDOUT_FILENO }, pair { errFd, STDERR_FILENO } }) {
      if (const i32 rc = posix_spawn_file_actions_adddup2(&actions, from, to); rc != 0) {
        ret.error = { rc, system_category() };
        return ret;
      }
    }

    const i32 rc = path
      ? posix_spawn(&pid, path, &actions, nullptr, argv.data(), environ)
      : posix_spawnp(&pid, argv.front(), &actions, nullptr, argv.data(), environ);
    if (rc != 0) {
      ret.error = { rc, system_category() };
      return ret;
    }
  }

  // Close the own copies of the ends the command writes to. Only then is the end of its output seen
  outWrite.reset();
  errWrite.reset();

  ret.error = readPipes(outRead, ret.out, errRead, ret.err);

  // After a read error, close the pipes, so the command fails to write rather than block
  outRead.reset();
  errRead.reset();

  i32 status = 0;
  while (waitpid(pid, &status, 0) == -1) {
    if (errno != EINTR) {
      if (not ret.error) {
        ret.error = lastError();
      }
      return ret;
    }
  }
  if (WIFEXITED(status)) {
    ret.status = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    ret.signal = WTERMSIG(status);
  }

  return ret;
}

#endif

} // namespace

namespace rocket::system {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

optional<string>
getImpl(std::string_view name) {
  ROCKET_MUTEX_LOCK(envMutex());

  const string nameStr(name);

#ifdef ROCKET_OS_WINDOWS
  size_t size = 0;
  getenv_s(&size, nullptr, 0, nameStr.c_str());
  if (size == 0) {
    return nullopt;
  }
  string ret(size - 1, '\0');
  getenv_s(&size, ret.data(), size, nameStr.c_str());
  return ret;
#else
  const char* p = getenv(nameStr.c_str()); // NOLINT(concurrency-*)
  if (p == nullptr) {
    return nullopt;
  }
  return p;
#endif
}

void
setImpl(std::string_view name, const optional<string>& val, bool replace) {
  ROCKET_CHECK(val, val || replace);

  ROCKET_MUTEX_LOCK(envMutex());

  const string nameStr(name);

#ifdef ROCKET_OS_WINDOWS
  if (val && not replace && env::get<string>(name)) {
    return;
  }
  // Set/unset
  _putenv_s(nameStr.c_str(), val ? val->c_str() : "");
#else
  if (val) {
    // Set
    setenv(nameStr.c_str(), val->c_str(), replace ? 1 : 0); // NOLINT(concurrency-*)
  } else {
    // Unset
    unsetenv(const_cast<char*>(nameStr.c_str())); // NOLINT(concurrency-*)
  }
#endif
}

} // namespace internal

// Functions ------------------------------------------------------------------------------------------------

#ifdef ROCKET_OS_WINDOWS

// Windows
ExecResult
exec(const string& cl, bool combined) {
  ROCKET_CHECK(cl, not cl.contains('\0'), "May not contain NUL characters");

  string app;
  if (const auto ec = cmdPath(app); ec) {
    ExecResult ret;
    ret.error = ec;
    return ret;
  }
  // This is how `_popen` runs a command line
  return execImpl(app.c_str(), "cmd.exe /c " + cl, combined);
}

// Windows
ExecResult
exec(const vector<string_view>& args, bool combined) {
  ROCKET_CHECK(args, not args.empty(), "May not be empty");
  ROCKET_CHECK(args, not args.front().empty(), "Program may not be empty");

  return execImpl(nullptr, makeCl(args), combined);
}

#else

// Linux, macOS
ExecResult
exec(const string& cl, bool combined) {
  ROCKET_CHECK(cl, not cl.contains('\0'), "May not contain NUL characters");

  // This is how POSIX.1-2024 specifies `popen`. `--` ends the options of the shell, in case `cl` starts with
  // `-` or `+`
  return execImpl("/bin/sh", { "sh", "-c", "--", cl }, combined);
}

// Linux, macOS
ExecResult
exec(const vector<string_view>& args, bool combined) {
  ROCKET_CHECK(args, not args.empty(), "May not be empty");
  ROCKET_CHECK(args, not args.front().empty(), "Program may not be empty");

  vector<string> argv;
  argv.reserve(args.size());
  for (const string_view arg : args) {
    ROCKET_CHECK(args, not arg.contains('\0'), "May not contain NUL characters");
    argv.emplace_back(arg);
  }
  return execImpl(nullptr, std::move(argv), combined);
}

#endif

vector<string>
makeArgs(string_view cl) { // NOLINT(*-complexity)
  vector<string> ret;

  enum State : u8 { NORMAL, SINGLE, DOUBLE };
  State state = NORMAL;
  string arg;
  bool inArg = false; // An argument can be in progress and still be empty, as with `''`

  for (size_t i = 0, size = cl.size(); i < size; ++i) {
    const char c = cl[i];
    optional<char> next;
    if (i < size - 1) {
      next = cl[i + 1];
    }

    switch (state) {
      case NORMAL:
        if (isspace(static_cast<u8>(c)) != 0) {
          if (inArg) {
            ret.push_back(arg);
            arg.clear();
            inArg = false;
          }
          break;
        }
        inArg = true;
        if (c == '\'') {
          state = SINGLE;
        } else if (c == '"') {
          state = DOUBLE;
        } else if (c == '\\') {
          if (not next) {
            throw InputFailure(i, "Invalid escape sequence");
          }
          arg.push_back(*next);
          ++i;
        } else {
          arg.push_back(c);
        }
        break;
      case SINGLE:
        if (c == '\'') {
          state = NORMAL;
        } else {
          arg.push_back(c);
        }
        break;
      case DOUBLE:
        if (c == '"') {
          state = NORMAL;
        } else if (c == '\\') {
          if (not next) {
            throw InputFailure(i, "Invalid escape sequence");
          }
          arg.push_back(*next);
          ++i;
        } else {
          arg.push_back(c);
        }
        break;
    }
  }

  if (state == SINGLE) {
    throw InputFailure(cl.size(), "Unterminated single quote");
  }
  if (state == DOUBLE) {
    throw InputFailure(cl.size(), "Unterminated double quote");
  }

  if (inArg) {
    ret.push_back(arg);
  }

  return ret;
}

namespace env {

// Environment ----------------------------------------------------------------------------------------------

// Functions ................................................................................................

unordered_map<string, string>
get() {
  ROCKET_MUTEX_LOCK(envMutex());

  unordered_map<string, string> ret;

#ifdef ROCKET_OS_WINDOWS
  // The C runtime builds `_environ` from the process environment block and leaves out the hidden per-drive
  // entries such as `=C:=C:\path`. Unlike `GetEnvironmentStrings`, which returns the raw block, it therefore
  // holds only ordinary variables
  char** p = _environ; // NOLINT
#else
  char** p = environ; // NOLINT
#endif

  for (; p != nullptr && *p != nullptr; ++p) {
    const string_view entry(*p);
    string_view name, val;
    auto eq = entry.find('=');
    if (eq == string_view::npos) {
      name = entry;
    } else {
      name = entry.substr(0, eq);
      val = entry.substr(eq + 1);
    }
    // A variable has a nonempty name. Skip anything else, in case a hidden entry gets through after all
    if (name.empty()) {
      continue;
    }
    ret.emplace(name, val);
  }

  return ret;
}

} // namespace env

} // namespace rocket::system

// EOF
