/*
 * nio.cc
 */

#include "nio.h"

#include "rocket/SystemError.h"
#include "rocket/assert.h"
#include "rocket/io/io.h"

#include <fmt/ostream.h>
#include <fmt/std.h>

#include <scn/istream.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <iterator>
#include <locale>
#include <optional>
#include <system_error>

using namespace rocket;
using namespace rocket::nio;
using namespace std;

namespace fs = std::filesystem;

namespace {

// `FileLock` -----------------------------------------------------------------------------------------------

/// Holds the lock of a `FILE`, so that several characters can be read with #getcUnlocked, without locking
/// and unlocking the `FILE` for each of them. The lock is recursive, so the `FILE` can still be used
/// otherwise.
struct FileLock {
  explicit FileLock(FILE* file) : file_(file) {
#ifdef ROCKET_OS_WINDOWS
    _lock_file(file_);
#else
    flockfile(file_);
#endif
  }

  FileLock(const FileLock&) = delete;

  FileLock& operator=(const FileLock&) = delete;

  ~FileLock() {
#ifdef ROCKET_OS_WINDOWS
    _unlock_file(file_);
#else
    funlockfile(file_);
#endif
  }

private:

  FILE* file_;
};

/// Reads a character from @p file, like `getc`, but without locking it. The caller must hold a #FileLock.
i32
getcUnlocked(FILE* file) {
#ifdef ROCKET_OS_WINDOWS
  return _getc_nolock(file);
#else
  return getc_unlocked(file); // NOLINT(concurrency-mt-unsafe): the caller holds a `FileLock`
#endif
}

/// Returns the `fopen` mode for writing to a file.
const char*
writeMode(bool append, bool binary) {
  if (append) {
    return binary ? "ab" : "a";
  }
  return binary ? "wb" : "w";
}

// `checkFileScan` ------------------------------------------------------------------------------------------

/// Returns the error of @p result, if any, dropping its value.
template<typename T>
scn::scan_expected<void>
toVoid(const scn::scan_expected<T>& result) {
  if (not result) {
    return scn::unexpected(result.error());
  }
  return {};
}

/// Checks the result of scanning from @p file, a `scn::scan_file` over @p in.
///
/// Afterwards, `scn::scan_file` puts unread input back into its `FILE`. Input it could not put back is left
/// in its prelude, which would be lost. If @p in can seek, it seeks back over that input instead.
template<typename T>
scn::scan_expected<void>
checkFileScan(Source& in, const scn::scan_file& file, const scn::scan_expected<T>& result) {
  // glibc can put back any amount of input, but the Windows C runtime only guarantees one character
  if (const auto n = file.prelude().size(); n != 0) {
    const auto pos = in.tell();
    if (not pos || *pos < n || not in.seek(static_cast<i64>(*pos - n), SeekMode::beg)) {
      return scn::unexpected(
        scn::scan_error(scn::scan_error::invalid_source_state, "Cannot put back unread input"));
    }
  }
  return toVoid(result);
}

// Local functions ------------------------------------------------------------------------------------------

/// Returns the bytes of @p buf.
span<const u8>
bytes(const fmt::memory_buffer& buf) {
  return { reinterpret_cast<const u8*>(buf.data()), buf.size() };
}

/**
 * Clears the state of @p s. This throws only if @p s has no stream buffer, which sets `badbit`, and `badbit`
 * is in the exception mask. This is ignored here, @p s stays bad then.
 */
void
clearState(ios& s) noexcept {
  try {
    s.clear();
  } catch (const ios::failure&) { // NOLINT(bugprone-empty-catch)
  }
}

/**
 * Computes the new position of a contiguous source of size @p size, currently at @p pos, for
 * #rocket::nio::Source#seek.
 *
 * @return the new position, or null if it would be before the beginning or beyond the end
 */
optional<u64>
contiguousSeekPos(u64 pos, u64 size, i64 offset, SeekMode mode) {
  u64 base = 0;
  switch (mode) {
    case SeekMode::beg: base = 0; break;
    case SeekMode::cur: base = pos; break;
    case SeekMode::end: base = size; break;
    default: ROCKET_FLOP(mode, "invalid seek mode {}", static_cast<i32>(mode));
  }

  if (offset < 0) {
    // `-offset` would overflow for the lowest `i64`, `-(offset + 1) + 1` cannot
    const u64 back = static_cast<u64>(-(offset + 1)) + 1;
    if (back > base) {
      return nullopt;
    }
    return base - back;
  }

  // `base <= size` always holds, so `size - base` cannot wrap, and comparing against it avoids an overflow
  // of `base + offset`
  const auto forward = static_cast<u64>(offset);
  if (forward > size - base) {
    return nullopt;
  }
  return base + forward;
}

/// Formats into a buffer that keeps short messages on the stack.
fmt::memory_buffer
formatToBuffer(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) {
  fmt::memory_buffer ret;
  fmt::vformat_to(fmt::appender(ret), style, fmt, args);
  return ret;
}

/// Formats into a buffer that keeps short messages on the stack.
fmt::memory_buffer
formatToBuffer(const locale& locale, fmt::string_view fmt, fmt::format_args args) {
  fmt::memory_buffer ret;
  fmt::vformat_to(fmt::appender(ret), fmt::locale_ref(locale), fmt, args);
  return ret;
}

/// Returns the scan error for an I/O error.
scn::scan_expected<void>
ioScanError() {
  return scn::unexpected(scn::scan_error(scn::scan_error::invalid_source_state, "I/O error"));
}

void
removeTrailingCr(string& line) {
  if (line.ends_with('\r')) {
    line.pop_back();
  }
}

void
removeTrailingCr(string_view& line) {
  if (line.ends_with('\r')) {
    line.remove_suffix(1);
  }
}

/// Sets `badbit` on @p s. This throws if `badbit` is in the exception mask of @p s, which is ignored here.
void
setBad(ios& s) noexcept {
  try {
    s.setstate(ios::badbit);
  } catch (const ios::failure&) { // NOLINT(bugprone-empty-catch)
  }
}

/**
 * Runs @p op, an operation on @p s, and maintains @p ec, whatever the exception mask of @p s.
 *
 * - An `ios_base::failure` is the stream telling about a state bit in its exception mask, after it set the
 *   state. It is ignored here, as the state tells what happened.
 * - Any other exception comes from the stream buffer. The stream rethrows it if `badbit` is in its exception
 *   mask, and @p op may call the stream buffer directly, too. It is an I/O error, so `badbit` is set.
 * - A `fmt::format_error` or a `std::bad_alloc` is no I/O error, it is rethrown.
 *
 * If @p s is bad afterwards, @p ec is set: to the code of a `std::system_error` from the stream buffer, to
 * `errc::io_error` for any other exception from it, or else to #streamBufferErrorCode. @p s must not be bad
 * when this is called.
 */
template<typename F>
void
runStreamOp(ios& s, error_code& ec, F op) {
  errno = 0;
  try {
    op();
  } catch (const ios::failure&) { // NOLINT(bugprone-empty-catch)
  } catch (const system_error& ex) {
    setBad(s);
    ec = ex.code();
    return;
  } catch (const fmt::format_error&) {
    throw;
  } catch (const bad_alloc&) {
    if (s.bad()) {
      ec = errorCode(errc::not_enough_memory);
    }
    throw;
  } catch (const exception&) {
    // Throwing leaves `errno` meaningless, the exception runtime may have touched it
    setBad(s);
    ec = errorCode(errc::io_error);
    return;
  }
  if (s.bad()) {
    ec = errorCode(s);
  }
}

} // namespace

namespace rocket::nio {

// `Sink` ---------------------------------------------------------------------------------------------------

u64
Sink::write(span<const u8> in, u64 offset, u64 n) {
  ROCKET_CHECK(offset, offset <= in.size(), "must not be greater than the size {}", in.size());
  return write(in.subspan(offset, min<u64>(n, in.size() - offset)));
}

u64
Sink::write(string_view in, u64 offset, u64 n) {
  ROCKET_CHECK(offset, offset <= in.size(), "must not be greater than the size {}", in.size());
  return write(in.substr(offset, n));
}

u64
Sink::writeln(string_view in, u64 offset, u64 n) {
  ROCKET_CHECK(offset, offset <= in.size(), "must not be greater than the size {}", in.size());
  return writeln(in.substr(offset, n));
}

u64
Sink::writeln(string_view in) {
  if (bad()) {
    return 0;
  }

  auto ret = write(in);
  ret += write('\n');
  return ret;
}

// `FileSink` -----------------------------------------------------------------------------------------------

FileSink::FileSink(FILE* file) :
  file_(file),
  closeOnDestroy_(false) {
  ROCKET_CHECK(file, file != nullptr);
  if (bad()) {
    ec_ = errorCode(errc::io_error);
  }
  seekable_ = io::file::seekable(file_);
}

FileSink::FileSink(const fs::path& path, bool append, bool binary) :
  closeOnDestroy_(true) {
  errno = 0;
  file_ = io::file::open(path, writeMode(append, binary));
  if (file_ == nullptr) {
    throw SystemError(errorCode(errno), fmt::format("cannot open file `{}` for writing", path));
  }
  seekable_ = io::file::seekable(file_);
}

FileSink::~FileSink() {
  flush(); // NOLINT

  if (closeOnDestroy_) {
    fclose(file_); // NOLINT
  }
}

bool
FileSink::bad() const {
  return ferror(file_) != 0;
}

bool
FileSink::eof() const {
  return feof(file_) != 0;
}

bool
FileSink::flush() {
  if (bad()) {
    return false;
  }

  errno = 0;
  if (fflush(file_) != 0) {
    if (bad()) {
      ec_ = errorCode(errno);
    }
    return false;
  }
  return true;
}

i32
FileSink::handle() const {
  return ROCKET_FILENO(file_);
}

bool
FileSink::vprint(fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  // Formats directly into the buffer of `file_`, if possible. {fmt} throws a `system_error` with the code
  // for `errno` if writing fails
  errno = 0;
  try {
    fmt::vprint(file_, fmt, args);
  } catch (const system_error& ex) {
    ec_ = ex.code();
    return false;
  }
  if (bad()) {
    ec_ = errorCode(errno);
    return false;
  }
  return true;
}

bool
FileSink::vprint(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  errno = 0;
  try {
    fmt::vprint(file_, style, fmt, args);
  } catch (const system_error& ex) {
    ec_ = ex.code();
    return false;
  }
  if (bad()) {
    ec_ = errorCode(errno);
    return false;
  }
  return true;
}

bool
FileSink::vprint(const locale& locale, fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  // {fmt} cannot print to a `FILE*` with a locale
  const auto buf = formatToBuffer(locale, fmt, args);
  return write(bytes(buf)) == buf.size();
}

u64
FileSink::write(span<const u8> in) {
  if (bad()) {
    return 0;
  }

  errno = 0;
  const auto ret = fwrite(in.data(), 1, in.size(), file_);
  if (bad()) {
    ec_ = errorCode(errno);
  }
  return ret;
}

// `SpanSink` -----------------------------------------------------------------------------------------------

SpanSink::SpanSink(span<char> out) :
  out_(out) {
  seekable_ = true;
}

bool
SpanSink::vprint(fmt::string_view fmt, fmt::format_args args) {
  // Format directly into the span. `result.size` is the untruncated size
  const u64 available = out_.size() - pos_;
  const auto result = fmt::vformat_to_n(out_.data() + pos_, available, fmt, args);
  pos_ += min(available, static_cast<u64>(result.size));
  return result.size <= available;
}

bool
SpanSink::vprint(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) {
  const auto buf = formatToBuffer(style, fmt, args);
  return write(bytes(buf)) == buf.size();
}

bool
SpanSink::vprint(const locale& locale, fmt::string_view fmt, fmt::format_args args) {
  const auto buf = formatToBuffer(locale, fmt, args);
  return write(bytes(buf)) == buf.size();
}

u64
SpanSink::write(span<const u8> in) {
  const u64 available = out_.size() - pos_;
  const u64 ret = min(available, static_cast<u64>(in.size()));
  if (ret > 0) {
    memcpy(&out_[pos_], in.data(), ret);
    pos_ += ret;
  }
  return ret;
}


// `StreamSink` ---------------------------------------------------------------------------------------------

StreamSink::StreamSink(ostream& os) :
  ptr_(&os) {
  if (bad()) {
    ec_ = errorCode(errc::io_error);
  }
  seekable_ = io::stream::seekable(out());
}

StreamSink::StreamSink(const fs::path& path, bool append, bool binary) {
  // A failing `open` sets `failbit`, not `badbit`. The exception mask of `owned_` is empty, so it does not throw
  errno = 0;
  owned_.open(path, (append ? ios::app : ios::trunc) | (binary ? ios::binary : ios::openmode {}));
  if (not owned_.is_open()) {
    throw SystemError(errorCode(errno), fmt::format("cannot open file `{}` for writing", path));
  }
  seekable_ = io::stream::seekable(out());
}

StreamSink::~StreamSink() {
  flush(); // NOLINT
}

bool
StreamSink::flush() {
  if (bad()) {
    return false;
  }

  runStreamOp(out(), ec_, [this] { out().flush(); });
  return not bad();
}

i32
StreamSink::handle() const {
  if (&out() == &cout) {
    return STDOUT_FILENO;
  }
  if (&out() == &cerr || &out() == &clog) {
    return STDERR_FILENO;
  }
  return -1;
}

bool
StreamSink::vprint(fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  runStreamOp(out(), ec_, [&] { fmt::vprint(out(), fmt, args); });
  return not bad();
}

bool
StreamSink::vprint(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  // {fmt} cannot print to a `std::ostream` with a text style
  const auto buf = formatToBuffer(style, fmt, args);
  return write(bytes(buf)) == buf.size();
}

bool
StreamSink::vprint(const locale& locale, fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  // {fmt} cannot print to a `std::ostream` with a locale
  const auto buf = formatToBuffer(locale, fmt, args);
  return write(bytes(buf)) == buf.size();
}

u64
StreamSink::write(span<const u8> in) {
  if (bad()) {
    return 0;
  }

  // Not `ostream::write`, which only tells whether all bytes were written, but `sputn`, which tells how
  // many. The sentry does what `write` does around it: it flushes a tied stream, and on destruction honors
  // `unitbuf`. `write` sets `badbit` on a short count, and so do we. If the stream buffer throws, we cannot
  // know how much it took
  auto& os = out();
  u64 ret = 0;
  runStreamOp(os, ec_, [&] {
    const ostream::sentry sentry(os);
    if (sentry) {
      ret = static_cast<u64>(
        os.rdbuf()->sputn(reinterpret_cast<const char*>(in.data()), static_cast<streamsize>(in.size())));
      if (ret < in.size()) {
        setBad(os);
      }
    }
  });
  return ret;
}

// `StringSink` ---------------------------------------------------------------------------------------------

StringSink::StringSink() {
  seekable_ = true;
}

StringSink::StringSink(string& ref) :
  ptr_(&ref) {
  seekable_ = true;
}

bool
StringSink::vprint(fmt::string_view fmt, fmt::format_args args) {
  fmt::vformat_to(back_inserter(out()), fmt, args);
  return true;
}

bool
StringSink::vprint(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) {
  fmt::vformat_to(back_inserter(out()), style, fmt, args);
  return true;
}

bool
StringSink::vprint(const locale& locale, fmt::string_view fmt, fmt::format_args args) {
  fmt::vformat_to(back_inserter(out()), fmt::locale_ref(locale), fmt, args);
  return true;
}

u64
StringSink::write(span<const u8> in) {
  out().append(reinterpret_cast<const char*>(in.data()), in.size());
  return in.size();
}

// `Source` -------------------------------------------------------------------------------------------------

vector<u8>
Source::readAll() {
  if (bad()) {
    return {};
  }

  vector<u8> ret;
  auto buf = make_unique<u8[]>(DEFAULT_BUFFER_SIZE); // NOLINT
  const span<u8> out(&buf[0], DEFAULT_BUFFER_SIZE);
  while (true) {
    const u64 n = read(out);
    if (n > 0) {
      ret.insert(ret.end(), out.data(), out.data() + n);
    }
    if (n != out.size()) {
      break;
    }
  }
  return ret;
}

string
Source::readString() {
  if (bad()) {
    return {};
  }

  string ret;
  auto buf = make_unique<u8[]>(DEFAULT_BUFFER_SIZE); // NOLINT
  const span<u8> out(&buf[0], DEFAULT_BUFFER_SIZE);
  while (true) {
    const u64 n = read(out);
    if (n > 0) {
      const string_view view(reinterpret_cast<const char*>(out.data()), n);
      ret.append(view.data(), n);
    }
    if (n != out.size()) {
      break;
    }
  }
  return ret;
}

// `ContiguousSource` ---------------------------------------------------------------------------------------

optional<char>
ContiguousSource::peek() {
  const auto str = this->str();
  if (str.empty()) {
    return nullopt;
  }
  return str.front();
}

optional<string>
ContiguousSource::readln() {
  auto str = this->str();
  if (str.empty()) {
    return nullopt;
  }
  auto pos = str.find('\n');

  if (pos == NPOS) {
    seek(0, SeekMode::end);
    return string(str);
  }

  seek(static_cast<i64>(pos + 1), SeekMode::cur);
  str = str.substr(0, pos);
  removeTrailingCr(str);
  return string(str);
}

scn::scan_expected<void>
ContiguousSource::vscan(string_view fmt, scn::scan_args args) {
  const auto str = this->str();
  const auto result = scn::vscan(str, fmt, args);
  if (not result) {
    return scn::unexpected(result.error());
  }
  seek(static_cast<i64>(result->begin() - str.begin()), SeekMode::cur);
  return {};
}

scn::scan_expected<void>
ContiguousSource::vscan(const locale& locale, string_view fmt, scn::scan_args args) {
  const auto str = this->str();
  const auto result = scn::vscan(locale, str, fmt, args);
  if (not result) {
    return scn::unexpected(result.error());
  }
  seek(static_cast<i64>(result->begin() - str.begin()), SeekMode::cur);
  return {};
}

// `FileSource` ---------------------------------------------------------------------------------------------

FileSource::FileSource(FILE* file) :
  file_(file),
  closeOnDestroy_(false) {
  ROCKET_CHECK(file, file != nullptr);
  if (bad()) {
    ec_ = errorCode(errc::io_error);
  }
  seekable_ = io::file::seekable(file_);
}

FileSource::FileSource(const fs::path& path, bool binary) :
  closeOnDestroy_(true) {
  errno = 0;
  file_ = io::file::open(path, binary ? "rb" : "r");
  if (file_ == nullptr) {
    throw SystemError(errorCode(errno), fmt::format("cannot open file `{}` for reading", path));
  }
  seekable_ = io::file::seekable(file_);
}

FileSource::~FileSource() {
  if (closeOnDestroy_) {
    fclose(file_); // NOLINT
  }
}

bool
FileSource::bad() const {
  return ferror(file_) != 0;
}

bool
FileSource::eof() const {
  return feof(file_) != 0;
}

i32
FileSource::handle() const {
  return ROCKET_FILENO(file_);
}

optional<char>
FileSource::peek() {
  if (bad()) {
    return nullopt;
  }

  // Holding the lock across `getc` and `ungetc` also keeps other threads from reading in between
  const FileLock lock(file_);
  errno = 0;
  const i32 c = getcUnlocked(file_);
  if (c == EOF) {
    if (bad()) {
      ec_ = errorCode(errno);
    }
    return nullopt;
  }
  // C guarantees one character of pushback, even for streams that cannot seek
  ungetc(c, file_);
  return static_cast<char>(c);
}

u64
FileSource::read(span<u8> out) {
  if (bad()) {
    return 0;
  }

  // A short count may just be the end of the input
  errno = 0;
  const auto ret = fread(out.data(), 1, out.size(), file_);
  if (bad()) {
    ec_ = errorCode(errno);
  }
  return ret;
}

optional<string>
FileSource::readln() {
  if (bad()) {
    return nullopt;
  }

  // Lock the file once for the whole line, not for each character
  const FileLock lock(file_);

  errno = 0;
  i32 c = getcUnlocked(file_);
  if (c == EOF) {
    if (bad()) {
      ec_ = errorCode(errno);
    }
    return nullopt;
  }

  string ret;
  for (; c != EOF; c = getcUnlocked(file_)) {
    if (c == '\n') {
      removeTrailingCr(ret);
      break;
    }
    ret.push_back(static_cast<char>(c));
  }

  // `getc` also returns `EOF` on an I/O error. The line read so far is incomplete then, so it is dropped, as
  // with `StreamSource`, where `getline` fails once `badbit` is set
  if (bad()) {
    ec_ = errorCode(errno);
    return nullopt;
  }
  return ret;
}

bool
FileSource::seek(i64 offset, SeekMode mode) { // NOLINT
  i32 origin; // NOLINT
  switch (mode) {
    case SeekMode::beg: origin = SEEK_SET; break;
    case SeekMode::cur: origin = SEEK_CUR; break;
    case SeekMode::end: origin = SEEK_END; break;
    default: ROCKET_FLOP(mode, "invalid seek mode {}", static_cast<i32>(mode));
  }

  // Restore the device. `fseek` clears the end-of-file indicator, but not the error indicator. A seek that
  // fails, e.g., on a pipe, does not make the device bad
  clearerr(file_);
  ec_.clear();
  errno = 0;
  if (not seekable()) {
    errno = ESPIPE;
    ec_ = errorCode(errno);
    return false;
  }
  const auto ret = io::file::seek(file_, offset, origin);
  if (bad()) {
    ec_ = errorCode(errno);
    return false;
  }
  return ret;
}

optional<u64>
FileSource::tell() {
  if (bad() || not seekable()) {
    return nullopt;
  }

  // `ftell` does not make the device bad if it fails
  const u64 ret = io::file::tell(file_);
  return ret == NPOS ? nullopt : optional(ret);
}

scn::scan_expected<void>
FileSource::vscan(string_view fmt, scn::scan_args args) {
  if (bad()) {
    return scn::unexpected(scn::scan_error(scn::scan_error::invalid_source_state, "`FileSource` is bad"));
  }

  // Scans directly through `file_`. `scn::scan_file` locks `file_` while scanning
  errno = 0;
  scn::scan_file file(file_);
  const auto result = scn::vscan(file, fmt, args);
  if (bad()) {
    // Do not put back unread input, a seek would restore the device
    ec_ = errorCode(errno);
    return ioScanError();
  }
  return checkFileScan(*this, file, result);
}

scn::scan_expected<void>
FileSource::vscan(const locale& locale, string_view fmt, scn::scan_args args) {
  if (bad()) {
    return scn::unexpected(scn::scan_error(scn::scan_error::invalid_source_state, "`FileSource` is bad"));
  }

  errno = 0;
  scn::scan_file file(file_);
  const auto result = scn::vscan(locale, file, fmt, args);
  if (bad()) {
    // Do not put back unread input, a seek would restore the device
    ec_ = errorCode(errno);
    return ioScanError();
  }
  return checkFileScan(*this, file, result);
}

// `NullSource` ---------------------------------------------------------------------------------------------

scn::scan_expected<void>
NullSource::vscan(string_view fmt, scn::scan_args args) {
  // Scanning from empty input fails, unless the format string expects nothing
  return toVoid(scn::vscan(string_view(), fmt, args));
}

scn::scan_expected<void>
NullSource::vscan(const locale& locale, string_view fmt, scn::scan_args args) {
  return toVoid(scn::vscan(locale, string_view(), fmt, args));
}

// `SpanSource` ---------------------------------------------------------------------------------------------

SpanSource::SpanSource(span<const u8> in) :
  in_(in) {}

u64
SpanSource::read(span<u8> out) {
  const u64 ret = min<u64>(out.size(), in_.size() - pos_);
  if (ret > 0) {
    memcpy(out.data(), in_.data() + pos_, ret);
    pos_ += ret;
  }
  return ret;
}

bool
SpanSource::seek(i64 offset, SeekMode mode) {
  const auto pos = contiguousSeekPos(pos_, in_.size(), offset, mode);
  if (not pos) {
    return false;
  }
  pos_ = *pos;
  return true;
}

// `StreamSource` -------------------------------------------------------------------------------------------

StreamSource::StreamSource(std::istream& is) :
  ptr_(&is) {
  if (bad()) {
    ec_ = errorCode(errc::io_error);
  }
  seekable_ = io::stream::seekable(in());
}

StreamSource::StreamSource(const fs::path& path, bool binary) {
  // A failing `open` sets `failbit`, not `badbit`. The exception mask of `owned_` is empty, so it does not
  // throw
  errno = 0;
  owned_.open(path, binary ? ios::binary : ios::openmode {});
  if (not owned_.is_open()) {
    throw SystemError(errorCode(errno), fmt::format("cannot open file `{}` for reading", path));
  }
  seekable_ = io::stream::seekable(in());
}

i32
StreamSource::handle() const {
  if (&in() == &cin) {
    return STDIN_FILENO;
  }
  return -1;
}

optional<char>
StreamSource::peek() {
  if (bad()) {
    return nullopt;
  }

  // If the stream buffer throws, the MSVC STL’s `peek` sets `badbit` but returns 0 instead of EOF, so check
  // `bad()` as well
  auto c = istream::traits_type::eof();
  runStreamOp(in(), ec_, [&] { c = in().peek(); });
  if (c == istream::traits_type::eof() || bad()) {
    return nullopt;
  }
  return istream::traits_type::to_char_type(c);
}

u64
StreamSource::read(span<u8> out) {
  if (bad()) {
    return 0;
  }

  // `gcount` is the number of characters extracted. If the stream buffer threw, it may not count the
  // characters extracted before, as with libstdc++
  runStreamOp(in(), ec_, [&] {
    in().read(reinterpret_cast<char*>(out.data()), static_cast<streamsize>(out.size()));
  });
  return static_cast<u64>(in().gcount());
}

optional<string>
StreamSource::readln() {
  // The end of the input sticks, as with a `FILE`, until a seek
  if (bad() || eof()) {
    return nullopt;
  }

  // `getline` does nothing while `failbit` is set, which a previous operation may have left behind. It fails
  // if it extracts nothing, i.e., at the end of the input. Otherwise, it sets `eofbit` only if it hit the end,
  // so `eof()` tells whether the line had a line feed. If the exception mask makes it throw, the state is set
  // all the same
  string ret;
  runStreamOp(in(), ec_, [&] {
    clearState(in());
    getline(in(), ret);
  });
  if (in().fail()) { // Also if bad
    return nullopt;
  }
  if (not eof()) {
    removeTrailingCr(ret);
  }
  return ret;
}

bool
StreamSource::seek(i64 offset, SeekMode mode) { // NOLINT
  ios::seekdir dir = ios::beg;
  switch (mode) {
    case SeekMode::beg:
      dir = ios::beg;
      break;
    case SeekMode::cur:
      dir = ios::cur;
      break;
    case SeekMode::end:
      dir = ios::end;
      break;
    default:
      ROCKET_FLOP(mode, "invalid seek mode {}", static_cast<i32>(mode));
  }

  // Restore the device. `seekg` does nothing while `failbit` or `badbit` is set, which a previous operation
  // may have left behind. A seek that fails, e.g., on a pipe, does not make the device bad
  clearState(in());
  ec_.clear();
  if (bad()) { // Without a stream buffer
    ec_ = errorCode(errc::io_error);
    return false;
  }
  errno = 0;
  if (not seekable()) {
    errno = ESPIPE;
    ec_ = errorCode(errno);
    return false;
  }

  // `seekg` sets `failbit` if the stream buffer cannot seek, e.g., over a pipe
  runStreamOp(in(), ec_, [&] { in().seekg(static_cast<istream::off_type>(offset), dir); });
  if (bad()) {
    return false;
  }
  if (in().fail()) {
    // Clear `failbit` again, so the stream stays usable
    clearState(in());
    return false;
  }
  return true;
}

optional<u64>
StreamSource::tell() {
  if (bad() || not seekable()) {
    return nullopt;
  }

  // `io::stream::tell` asks the stream buffer and leaves the state of the stream alone, unlike `tellg`
  const u64 ret = io::stream::tell(in());
  return ret == NPOS ? nullopt : optional(ret);
}

scn::scan_expected<void>
StreamSource::vscan(string_view fmt, scn::scan_args args) {
  if (bad()) {
    return scn::unexpected(scn::scan_error(scn::scan_error::invalid_source_state, "`StreamSource` is bad"));
  }

  // Scans directly from the stream buffer of `in()`, putting unread input back afterwards
  scn::scan_expected<void> ret = ioScanError();
  runStreamOp(in(), ec_, [&] { ret = toVoid(scn::vscan(in(), fmt, args)); });
  return bad() ? ioScanError() : ret;
}

scn::scan_expected<void>
StreamSource::vscan(const locale& locale, string_view fmt, scn::scan_args args) {
  if (bad()) {
    return scn::unexpected(scn::scan_error(scn::scan_error::invalid_source_state, "`StreamSource` is bad"));
  }

  scn::scan_expected<void> ret = ioScanError();
  runStreamOp(in(), ec_, [&] { ret = toVoid(scn::vscan(locale, in(), fmt, args)); });
  return bad() ? ioScanError() : ret;
}

// `StringSource` -------------------------------------------------------------------------------------------

StringSource::StringSource(string_view in) :
  in_(in) {}

u64
StringSource::read(span<u8> out) {
  const u64 ret = min<u64>(out.size(), in_.size() - pos_);
  if (ret > 0) {
    memcpy(out.data(), in_.data() + pos_, ret);
    pos_ += ret;
  }
  return ret;
}

bool
StringSource::seek(i64 offset, SeekMode mode) { // NOLINT
  const auto pos = contiguousSeekPos(pos_, in_.size(), offset, mode);
  if (not pos) {
    return false;
  }
  pos_ = *pos;
  return true;
}

} // namespace rocket::nio

// Variables ------------------------------------------------------------------------------------------------

namespace {

FileSource fileSourceIn = FileSource(stdin);
FileSink fileSinkOut = FileSink(stdout);
FileSink fileSinkErr = FileSink(stderr);

} // namespace

namespace rocket::nio {

ROCKET_PUBLIC Source& in = fileSourceIn; // NOLINT
ROCKET_PUBLIC Sink& out = fileSinkOut; // NOLINT
ROCKET_PUBLIC Sink& err = fileSinkErr; // NOLINT

} // namespace rocket::nio

// EOF
