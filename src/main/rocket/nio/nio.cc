/*
 * nio.cc
 */

#include "nio.h"

#include "rocket/assert.h"
#include "rocket/io/io.h"

#include <fmt/ostream.h>

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

// Local functions ------------------------------------------------------------------------------------------

namespace {

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
    default: ROCKET_FLOP(mode, "Invalid seek mode {}", static_cast<i32>(mode));
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

/// Returns the bytes of @p buf.
span<const u8>
bytes(const fmt::memory_buffer& buf) {
  return { reinterpret_cast<const u8*>(buf.data()), buf.size() };
}

/// Formats into a buffer that keeps short messages on the stack.
fmt::memory_buffer
formatToBuffer(fmt::string_view fmt, fmt::format_args args) {
  fmt::memory_buffer ret;
  fmt::vformat_to(fmt::appender(ret), fmt, args);
  return ret;
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

/// Returns the error of @p result, if any, dropping its value.
template<typename T>
scn::scan_expected<void>
toVoid(const scn::scan_expected<T>& result) {
  if (not result) {
    return scn::unexpected(result.error());
  }
  return {};
}

/**
 * Checks the result of scanning from @p file, a `scn::scan_file` over @p in.
 *
 * Afterwards, `scn::scan_file` puts unread input back into its `FILE`. Input it could not put back is left
 * in its prelude, which would be lost. If @p in can seek, it seeks back over that input instead.
 */
template<typename T>
scn::scan_expected<void>
checkFileScan(Source& in, const scn::scan_file& file, const scn::scan_expected<T>& result) {
  // glibc can put back any amount of input, but the Windows C runtime only guarantees one character
  if (const auto n = file.prelude().size(); n != 0) {
    const auto pos = in.seekable() ? in.tell() : optional<u64>();
    if (not pos || *pos < n || not in.seek(static_cast<i64>(*pos - n), SeekMode::beg)) {
      return scn::unexpected(
        scn::scan_error(scn::scan_error::invalid_source_state, "Cannot put back unread input"));
    }
  }
  return toVoid(result);
}

/**
 * Writes @p size bytes at @p data to @p os, for #rocket::nio::StreamSink.
 *
 * This goes through `ostream::write`, not the stream buffer, so that a failure sets `badbit` and `unitbuf` is
 * honored. If the exception mask of @p os makes the failure throw `ios_base::failure`, a `system_error`, this
 * returns `false` instead, as all of #rocket::nio::StreamSink does.
 *
 * @return if @p os is still good
 */
bool
writeToStream(ostream& os, const char* data, u64 size) {
  try {
    os.write(data, static_cast<streamsize>(size));
  } catch (const system_error&) {
    return false;
  }
  return static_cast<bool>(os);
}

/**
 * Holds the lock of a `FILE`, so that several characters can be read with #getcUnlocked, without locking and
 * unlocking the `FILE` for each of them. The lock is recursive, so the `FILE` can still be used otherwise.
 */
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
  return getc_unlocked(file);
#endif
}

/// Removes a trailing @c '\r' from @p line, which was read up to a @c '\n'.
void
removeTrailingCr(string& line) {
  if (not line.empty() && line.back() == '\r') {
    line.pop_back();
  }
}

} // namespace

namespace rocket::nio {

// `Sink` ---------------------------------------------------------------------------------------------------

u64
Sink::write(span<const u8> in, u64 offset, u64 n) {
  ROCKET_CHECK(offset, offset <= in.size(), "Must not be greater than the size {}", in.size());
  return write(in.subspan(offset, min<u64>(n, in.size() - offset)));
}

u64
Sink::write(string_view in, u64 offset, u64 n) {
  ROCKET_CHECK(offset, offset <= in.size(), "Must not be greater than the size {}", in.size());
  return write(in.substr(offset, n));
}

u64
Sink::writeln(string_view in, u64 offset, u64 n) {
  ROCKET_CHECK(offset, offset <= in.size(), "Must not be greater than the size {}", in.size());
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

bool
Sink::vprintln(fmt::string_view fmt, fmt::format_args args) {
  return vprint(fmt, args) && write('\n') == 1;
}

bool
Sink::vprintln(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) {
  return vprint(style, fmt, args) && write('\n') == 1;
}

bool
Sink::vprintln(const locale& locale, fmt::string_view fmt, fmt::format_args args) {
  return vprint(locale, fmt, args) && write('\n') == 1;
}

// `FileSink` -----------------------------------------------------------------------------------------------

FileSink::FileSink(FILE* file) :
  file_(file),
  closeOnDestroy_(false) {
  ROCKET_CHECK(file, file != nullptr);
  seekable_ = io::seekable(file_);
}

FileSink::FileSink(const filesystem::path& path, bool append) :
  file_(io::open(path, append ? "ab" : "wb")),
  closeOnDestroy_(true) {
  if (file_ == nullptr) {
    throw InvalidState(fmt::format("Cannot open file `{}` for writing", path));
  }
  seekable_ = io::seekable(file_);
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

  const auto result = fflush(file_);
  return result == 0;
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

  try {
    // Formats directly into the buffer of `file_`, if possible
    fmt::vprint(file_, fmt, args);
  } catch (const system_error&) {
    return false;
  }
  return not bad();
}

bool
FileSink::vprint(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  try {
    fmt::vprint(file_, style, fmt, args);
  } catch (const system_error&) {
    return false;
  }
  return not bad();
}

bool
FileSink::vprint(const locale& locale, fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  // {fmt} cannot print to a `FILE*` with a locale
  const auto buf = formatToBuffer(locale, fmt, args);
  return fwrite(buf.data(), 1, buf.size(), file_) == buf.size();
}

bool
FileSink::vprintln(fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  try {
    // Writes message and line feed at once
    fmt::vprintln(file_, fmt, args);
  } catch (const system_error&) {
    return false;
  }
  return not bad();
}

bool
FileSink::vprintln(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  try {
    fmt::vprintln(file_, style, fmt, args);
  } catch (const system_error&) {
    return false;
  }
  return not bad();
}

bool
FileSink::vprintln(const locale& locale, fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  // {fmt} cannot print to a `FILE*` with a locale
  auto buf = formatToBuffer(locale, fmt, args);
  buf.push_back('\n');
  return fwrite(buf.data(), 1, buf.size(), file_) == buf.size();
}

u64
FileSink::write(span<const u8> in) {
  if (bad()) {
    return 0;
  }

  const auto result = fwrite(in.data(), 1, in.size(), file_);
  auto error = ferror(file_);
  ROCKET_ASSERT(result == in.size() || error != 0);
  return result;
}

// `SpanSink` -----------------------------------------------------------------------------------------------

SpanSink::SpanSink(span<char> out) :
  out_(out) {}

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
  os_(os) {}

StreamSink::~StreamSink() {
  flush(); // NOLINT
}

bool
StreamSink::flush() {
  if (bad()) {
    return false;
  }

  try {
    os_.flush();
  } catch (const system_error&) {
    return false;
  }
  return not bad();
}

i32
StreamSink::handle() const {
  if (&os_ == &cout) {
    return STDOUT_FILENO;
  }
  if (&os_ == &cerr || &os_ == &clog) {
    return STDERR_FILENO;
  }
  return -1;
}

bool
StreamSink::seekable() const {
  return io::seekable(os_);
}

bool
StreamSink::vprint(fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  try {
    fmt::vprint(os_, fmt, args);
  } catch (const system_error&) {
    return false;
  }
  return static_cast<bool>(os_);
}

bool
StreamSink::vprint(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  // {fmt} cannot print to a `std::ostream` with a text style
  const auto buf = formatToBuffer(style, fmt, args);
  return writeToStream(os_, buf.data(), buf.size());
}

bool
StreamSink::vprint(const locale& locale, fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  // {fmt} cannot print to a `std::ostream` with a locale
  const auto buf = formatToBuffer(locale, fmt, args);
  return writeToStream(os_, buf.data(), buf.size());
}

bool
StreamSink::vprintln(fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  // {fmt} cannot print a line to a `std::ostream` without formatting into a temporary string, so format
  // message and line feed into a buffer and write both at once
  auto buf = formatToBuffer(fmt, args);
  buf.push_back('\n');
  return writeToStream(os_, buf.data(), buf.size());
}

bool
StreamSink::vprintln(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  auto buf = formatToBuffer(style, fmt, args);
  buf.push_back('\n');
  return writeToStream(os_, buf.data(), buf.size());
}

bool
StreamSink::vprintln(const locale& locale, fmt::string_view fmt, fmt::format_args args) {
  if (bad()) {
    return false;
  }

  auto buf = formatToBuffer(locale, fmt, args);
  buf.push_back('\n');
  return writeToStream(os_, buf.data(), buf.size());
}

u64
StreamSink::write(span<const u8> in) {
  if (bad()) {
    return 0;
  }

  return writeToStream(os_, reinterpret_cast<const char*>(in.data()), in.size()) ? in.size() : 0;
}

// `StringSink` ---------------------------------------------------------------------------------------------

StringSink::StringSink(string& ref) :
  ptr_(&ref) {}

bool
StringSink::vprint(fmt::string_view fmt, fmt::format_args args) {
  fmt::vformat_to(back_inserter(target()), fmt, args);
  return true;
}

bool
StringSink::vprint(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) {
  fmt::vformat_to(back_inserter(target()), style, fmt, args);
  return true;
}

bool
StringSink::vprint(const locale& locale, fmt::string_view fmt, fmt::format_args args) {
  fmt::vformat_to(back_inserter(target()), fmt::locale_ref(locale), fmt, args);
  return true;
}

u64
StringSink::write(span<const u8> in) {
  target().append(reinterpret_cast<const char*>(in.data()), in.size());
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
    string ret(str);
    return ret;
  }

  seek(static_cast<i64>(pos + 1), SeekMode::cur);
  str = str.substr(0, pos);
  if (not str.empty() && str.back() == '\r') {
    str = str.substr(0, str.size() - 1);
  }
  string ret(str);
  return ret;
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
  seekable_ = io::seekable(file_);
}

FileSource::FileSource(const filesystem::path& path) :
  file_(io::open(path, "rb")),
  closeOnDestroy_(true) {
  if (file_ == nullptr) {
    throw InvalidState(fmt::format("Cannot open file `{}` for reading", path));
  }
  seekable_ = io::seekable(file_);
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
  const i32 c = getcUnlocked(file_);
  if (c == EOF) {
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

  const auto result = fread(out.data(), 1, out.size(), file_);
  return result;
}

optional<string>
FileSource::readln() {
  if (bad()) {
    return nullopt;
  }

  // Lock the file once for the whole line, not for each character
  const FileLock lock(file_);

  i32 c = getcUnlocked(file_);
  if (c == EOF) {
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
    return nullopt;
  }
  return ret;
}

bool
FileSource::seek(i64 offset, SeekMode mode) { // NOLINT
  if (bad()) {
    return false;
  }

  i32 origin; // NOLINT
  switch (mode) {
    case SeekMode::beg: origin = SEEK_SET; break;
    case SeekMode::cur: origin = SEEK_CUR; break;
    case SeekMode::end: origin = SEEK_END; break;
    default: ROCKET_FLOP(mode, "Invalid seek mode {}", static_cast<i32>(mode));
  }

  return io::seek(file_, offset, origin, seekable_);
}

optional<u64>
FileSource::tell() {
  if (bad()) {
    return nullopt;
  }

  const u64 ret = io::tell(file_, seekable_);
  return ret == NPOS ? nullopt : optional(ret);
}

scn::scan_expected<void>
FileSource::vscan(string_view fmt, scn::scan_args args) {
  if (bad()) {
    return scn::unexpected(scn::scan_error(scn::scan_error::invalid_source_state, "`FileSource` is bad"));
  }

  // Scans directly through `file_`. `scn::scan_file` locks `file_` while scanning
  scn::scan_file file(file_);
  const auto result = scn::vscan(file, fmt, args);
  return checkFileScan(*this, file, result);
}

scn::scan_expected<void>
FileSource::vscan(const locale& locale, string_view fmt, scn::scan_args args) {
  if (bad()) {
    return scn::unexpected(scn::scan_error(scn::scan_error::invalid_source_state, "`FileSource` is bad"));
  }

  scn::scan_file file(file_);
  const auto result = scn::vscan(locale, file, fmt, args);
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
  is_(is) {}

i32
StreamSource::handle() const {
  if (&is_ == &cin) {
    return STDIN_FILENO;
  }
  return -1;
}

optional<char>
StreamSource::peek() {
  if (bad()) {
    return nullopt;
  }

  // If the stream buffer throws, the MSVC STL's `peek` sets `badbit` but returns 0 instead of EOF
  const auto c = is_.peek();
  if (c == istream::traits_type::eof() || is_.bad()) {
    return nullopt;
  }
  return istream::traits_type::to_char_type(c);
}

u64
StreamSource::read(span<u8> out) {
  if (bad()) {
    return 0;
  }

  is_.read(reinterpret_cast<char*>(out.data()), static_cast<streamsize>(out.size()));
  auto count = is_.gcount();
  return static_cast<u64>(count);
}

optional<string>
StreamSource::readln() {
  if (bad()) {
    return nullopt;
  }

  // `getline` does nothing while `failbit` is set, which a previous read may have left behind
  is_.clear(is_.rdstate() & ~ios::failbit);

  // `getline` fails if it extracts nothing, i.e., at the end of the input. Otherwise, it sets `eofbit` only
  // if it hit the end, so `eof()` tells whether the line had a line feed
  string ret;
  if (not getline(is_, ret)) {
    return nullopt;
  }
  if (not is_.eof()) {
    removeTrailingCr(ret);
  }
  return ret;
}

bool
StreamSource::seek(i64 offset, SeekMode mode) { // NOLINT
  if (bad()) {
    return false;
  }

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
      ROCKET_FLOP(mode, "Invalid seek mode {}", static_cast<i32>(mode));
  }

  // On Windows and macOS, seeking `cin` may report success for a pipe or a terminal, as with `io::tell`
  if (&is_ == &cin && not io::seekable(stdin)) {
    return false;
  }

  // `seekg` does nothing while `failbit` is set, which a previous read may have left behind
  is_.clear(is_.rdstate() & ~ios::failbit);
  is_.seekg(static_cast<istream::off_type>(offset), dir);
  if (is_.fail()) {
    // The seek failed, e.g., on a stream that cannot seek. Clear `failbit` again, so the stream stays usable
    is_.clear(is_.rdstate() & ~ios::failbit);
    return false;
  }
  return true;
}

bool
StreamSource::seekable() const {
  return io::seekable(is_);
}

optional<u64>
StreamSource::tell() {
  if (bad()) {
    return nullopt;
  }

  // `io::tell` asks the stream buffer and leaves the state of the stream alone, unlike `tellg`. A stream that
  // cannot seek, such as one over a pipe, has no position
  const u64 ret = io::tell(is_);
  return ret == NPOS ? nullopt : optional(ret);
}

scn::scan_expected<void>
StreamSource::vscan(string_view fmt, scn::scan_args args) {
  if (bad()) {
    return scn::unexpected(scn::scan_error(scn::scan_error::invalid_source_state, "`StreamSource` is bad"));
  }

  // Scans directly from the stream buffer of `is_`, putting unread input back afterwards
  return toVoid(scn::vscan(is_, fmt, args));
}

scn::scan_expected<void>
StreamSource::vscan(const locale& locale, string_view fmt, scn::scan_args args) {
  if (bad()) {
    return scn::unexpected(scn::scan_error(scn::scan_error::invalid_source_state, "`StreamSource` is bad"));
  }

  return toVoid(scn::vscan(locale, is_, fmt, args));
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
