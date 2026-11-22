// io.cc

#include "rocket/io/io.h"

#include "rocket/assert.h"

#ifdef ROCKET_OS_WINDOWS
  #include "rocket/windows/windows.h"
  #include <share.h> // `_SH_DENYNO`
#else
  #include <sys/stat.h>
  #include <sys/types.h> // `off_t`
#endif

#include <cerrno>
#include <iostream>
#include <string>
#include <string_view>

using namespace rocket;
using namespace std;

namespace rocket::io {

// Functions working with `FILE` pointers -------------------------------------------------------------------

namespace file {

FILE*
open(const filesystem::path& path, std::string_view modes) {
#ifdef ROCKET_OS_WINDOWS
  // `_wfsopen` takes the path as a wide string, so paths with characters outside the ANSI code page work.
  // `_SH_DENYNO` allows other processes to read and write the file, as on POSIX systems. (`fopen_s` would
  // deny write access to others.) The modes are ASCII, so a plain widening suffices
  const wstring wmodes(modes.begin(), modes.end());
  return _wfsopen(path.c_str(), wmodes.c_str(), _SH_DENYNO);
#else
  return fopen(path.c_str(), string(modes).c_str()); // NOLINT
#endif
}

bool
seek(FILE* file, i64 offset, i32 origin) {
  ROCKET_CHECK(file, file != nullptr);

#ifdef ROCKET_OS_WINDOWS
  const auto result = _fseeki64(file, offset, origin);
#else
  if constexpr (sizeof(off_t) < sizeof(i64)) {
    // 32-bit `off_t`: build with `_FILE_OFFSET_BITS=64` to support large files
    if (offset != static_cast<off_t>(offset)) {
      errno = EOVERFLOW;
      return false;
    }
  }
  const auto result = fseeko(file, static_cast<off_t>(offset), origin);
#endif
  return result == 0;
}

bool
seekable(FILE* file) {
  ROCKET_CHECK(file, file != nullptr);

  const i32 fd = ROCKET_FILENO(file);
#ifdef ROCKET_OS_WINDOWS
  // `_fileno` returns -2 for a standard stream that isn’t attached to anything, e.g., in a GUI process
  if (fd < 0) {
    return false;
  }
  // `_get_osfhandle` returns -2 under the same conditions, which is also the pseudo-handle of the current
  // thread, so it must not be passed on
  const auto handle = reinterpret_cast<HANDLE>(_get_osfhandle(fd)); // NOLINT
  if (handle == INVALID_HANDLE_VALUE || handle == reinterpret_cast<HANDLE>(-2)) { // NOLINT
    return false;
  }
  const DWORD type = GetFileType(handle);
  return type == FILE_TYPE_DISK; // `FILE_TYPE_PIPE`: pipe or socket, `FILE_TYPE_CHAR`: console or `NUL`
#else
  if (fd < 0) {
    // No file descriptor, e.g., a stream made by `fmemopen` or `open_memstream`. Such streams are
    // implemented by the C library, which reports a position only if the stream is seekable
    return ftello(file) != -1;
  }
  struct stat st {};
  if (fstat(fd, &st) != 0) {
    return false;
  }
  return S_ISREG(st.st_mode) || S_ISBLK(st.st_mode);
#endif
}

u64
tell(FILE* file) {
  ROCKET_CHECK(file, file != nullptr);

#ifdef ROCKET_OS_WINDOWS
  const auto result = _ftelli64(file); // `__int64`
#else
  const auto result = ftello(file); // `off_t`
#endif
  return result < 0 ? NPOS : static_cast<u64>(result);
}

} // namespace file

// Functions working with streams ---------------------------------------------------------------------------

namespace stream {

bool
seekable(istream& is) noexcept {
  return tell(is) != NPOS;
}

bool
seekable(ostream& os) noexcept {
  // The stream buffers of the standard output streams are backed by `stdout` and `stderr`. On Windows and
  // macOS, they may report a meaningless position for a pipe or a terminal, so ask the `FILE` as well. Do so
  // first: with `sync_with_stdio`, asking the stream buffer seeks the `FILE`, which flushes it and may set
  // `errno`
  if ((&os == &cout && not file::seekable(stdout)) ||
    ((&os == &cerr || &os == &clog) && not file::seekable(stderr))) {
    return false;
  }

  // Ask the stream buffer directly, like `tell` for an input stream does
  // A stream buffer that throws cannot seek either
  auto* buf = os.rdbuf();
  try {
    return buf != nullptr &&
      buf->pubseekoff(0, ios::cur, ios::out) != ostream::pos_type(static_cast<ostream::off_type>(-1));
  } catch (...) {
    return false;
  }
}

u64
tell(istream& is) noexcept {
  // The stream buffer of `cin` is backed by `stdin`. On Windows and macOS, it may report a meaningless
  // position for a pipe or a terminal, so ask `stdin` as well. Do so first: with `sync_with_stdio`, asking
  // the stream buffer seeks `stdin`, which may set `errno`
  if (&is == &cin && not file::seekable(stdin)) {
    return NPOS;
  }

  // Ask the stream buffer directly. This is what `tellg` does after constructing a sentry, but the sentry
  // fails if any state bit is set, and it flushes the tied stream
  auto* buf = is.rdbuf();
  if (buf == nullptr) {
    return NPOS;
  }
  // A stream buffer that throws has no position
  istream::pos_type result;
  try {
    result = buf->pubseekoff(0, ios::cur, ios::in);
  } catch (...) {
    return NPOS;
  }
  if (result == istream::pos_type(static_cast<istream::off_type>(-1))) {
    return NPOS;
  }
  return static_cast<u64>(static_cast<streamoff>(result));
}

} // namespace stream

} // namespace rocket::io

// EOF
