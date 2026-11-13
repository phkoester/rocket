/*
 * nio.cc
 */

#include "nio.h"

#include "rocket/assert.h"
#include "rocket/io/io.h"
#include "rocket/unicode/unicode.h"

#include <algorithm>
#include <cstring>
#include <exception>
#include <iostream>
#include <optional>
#include <streambuf>

using namespace rocket;
using namespace rocket::nio;
using namespace std;

// Local functions ------------------------------------------------------------------------------------------

namespace {

/**
 * Computes the new position of a contiguous source of size @p size, currently at @p pos, for
 * #rocket::nio::Source#seek. A position beyond the end is clamped to the end.
 *
 * @return the new position, or null if it would be before the beginning
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
  return forward >= size - base ? size : base + forward;
}

/// Removes a trailing @c '\r' from @p line, which was read up to a @c '\n'.
void
removeTrailingCr(string& line) {
  if (not line.empty() && line.back() == '\r') {
    line.pop_back();
  }
}

// `FileStreambuf`, `FileIstream` ---------------------------------------------------------------------------

/**
 * An unbuffered stream buffer that reads through a `FILE` pointer.
 *
 * As there is no buffer, the stream and the `FILE` pointer always agree on the position. This also holds for
 * a `FILE` pointer that cannot seek, such as one over a pipe, and for text-mode translation on Windows.
 */
struct FileStreambuf : streambuf {
  explicit FileStreambuf(FILE* file) : file_(file) {}

protected:

  /// Peeks at the next character, without consuming it.
  int_type
  underflow() override {
    const i32 c = getc(file_);
    if (c == EOF) {
      return traits_type::eof();
    }
    ungetc(c, file_);
    return c; // `getc` yields an `unsigned char` value, which is what `to_int_type` would yield, too
  }

  /// Consumes the next character.
  int_type
  uflow() override {
    const i32 c = getc(file_);
    return c == EOF ? traits_type::eof() : c;
  }

  /// Puts back @p c. As there is no buffer, the previous character is unknown if @p c is EOF.
  int_type
  pbackfail(int_type c) override {
    if (traits_type::eq_int_type(c, traits_type::eof()) || ungetc(c, file_) == EOF) {
      return traits_type::eof();
    }
    return c;
  }

  streamsize
  xsgetn(char* s, streamsize n) override {
    return static_cast<streamsize>(fread(s, 1, static_cast<size_t>(n), file_));
  }

  pos_type
  seekoff(off_type off, ios::seekdir dir, [[maybe_unused]] ios::openmode which) override {
    // Querying the position must not seek, which would discard a character put back by `ungetc`
    if (off != 0 || dir != ios::cur) {
      const i32 origin = dir == ios::beg ? SEEK_SET : dir == ios::cur ? SEEK_CUR : SEEK_END;
      if (not io::seek(file_, off, origin)) {
        return pos_type(off_type(-1));
      }
    }
    const u64 pos = io::tell(file_);
    return pos == NPOS ? pos_type(off_type(-1)) : pos_type(static_cast<off_type>(pos));
  }

  pos_type
  seekpos(pos_type pos, ios::openmode which) override {
    return seekoff(off_type(pos), ios::beg, which);
  }

private:

  FILE* file_;
};

/// An input stream that reads through a `FILE` pointer, using a #FileStreambuf.
struct FileIstream : std::istream {
  explicit FileIstream(FILE* file) : std::istream(nullptr), buf_(file) { rdbuf(&buf_); }

private:

  FileStreambuf buf_;
};

} // namespace

namespace rocket::nio {

// `Sink` ---------------------------------------------------------------------------------------------------

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
}

FileSink::FileSink(const filesystem::path& path, bool append) :
  file_(io::open(path, append ? "ab" : "wb")),
  closeOnDestroy_(true) {
  if (file_ == nullptr) {
    throw InvalidState(fmt::format("Cannot open file `{}` for writing", path));
  }
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

  os_.flush();
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

u64
StreamSink::write(span<const u8> in) {
  if (bad()) {
    return 0;
  }

  // Through `ostream::write`, not the stream buffer, so that a failure sets `badbit` and `unitbuf` is honored
  os_.write(reinterpret_cast<const char*>(in.data()), static_cast<streamsize>(in.size()));
  return os_ ? in.size() : 0;
}

// `StringSink` ---------------------------------------------------------------------------------------------

StringSink::StringSink(string& ref) :
  ptr_(&ref) {}

u64
StringSink::write(span<const u8> in) {
  const string_view view(reinterpret_cast<const char*>(in.data()), in.size());
  if (ptr_ != nullptr) {
    ptr_->append(view);
  } else {
    owned_.append(view);
  }
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

ReadCodePointResult
Source::readCodePoint() {
  if (bad()) {
    return unexpected(Bad);
  }

  // A source that cannot report its position, such as a pipe or `NullSource`, cannot go back either. It is
  // not bad, though, so it is read all the same
  const u64 initialPos = tell();

  // Read first byte
  char c; // NOLINT
  if (read(c) != 1) {
    return unexpected(bad() ? Bad : Eof);
  }
  string s;
  s.push_back(c);

  // The source may only advance on success, so rewind the position on failure, if possible. Seeking a source
  // that cannot seek would not only fail, but might also discard buffered input
  const auto failInvalidUtf8 = [&] {
    if (initialPos != NPOS) {
      seek(static_cast<i64>(initialPos), SeekMode::beg);
    }
    return unexpected(InvalidUtf8);
  };

  // Read subsequent bytes
  const auto len = unicode::utf8::lengthFromByte(c, unicode::InvalidUnicodePolicy::Continue);
  if (len == NPOS) {
    return failInvalidUtf8(); // Neither a single byte nor a lead byte
  }
  for (u64 i = 0; i < len - 1; ++i) {
    if (read(c) != 1) {
      if (bad()) {
        return unexpected(Bad);
      }
      return failInvalidUtf8(); // Truncated sequence: there was input, so this isn’t a plain EOF
    }
    s.push_back(c);
  }

  // Decode the code point
  try {
    u64 pos = 0;
    auto ret = unicode::nextCodePoint(s, pos);
    if (pos != s.size()) {
      return failInvalidUtf8();
    }
    return ret;
  } catch (const exception&) {
    return failInvalidUtf8();
  }
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

string
Source::readln() {
  if (bad()) {
    return {};
  }

  string ret;

  bool lf = false;
  while (true) {
    char c; // NOLINT
    if (read(c) != 1) {
      break;
    }
    if (c == '\n') {
      lf = true;
      break;
    }
    ret.push_back(c);
  }

  if (lf) {
    removeTrailingCr(ret);
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

string
ContiguousSource::readln() {
#ifdef ROCKET_NIO_NO_CONTIGUOUS_SOURCE
  return Source::readln();
#else
  auto str = this->str();
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
#endif // ROCKET_NIO_NO_CONTIGUOUS_SOURCE
}

ReadCodePointResult
ContiguousSource::readCodePoint() {
#ifdef ROCKET_NIO_NO_CONTIGUOUS_SOURCE
  return Source::readCodePoint();
#else
  const auto str = this->str();
  if (str.empty()) {
    // `nextCodePoint` would throw for an empty string, which must not be mistaken for invalid UTF-8
    return unexpected(Eof);
  }

  try {
    u64 pos = 0;
    auto ret = unicode::nextCodePoint(str, pos);
    seek(static_cast<i64>(pos), SeekMode::cur);
    return ret;
  } catch (const exception&) {
    return unexpected(InvalidUtf8);
  }
#endif // ROCKET_NIO_NO_CONTIGUOUS_SOURCE
}

// `FileSource` ---------------------------------------------------------------------------------------------

FileSource::FileSource(FILE* file) :
  file_(file),
  closeOnDestroy_(false) {
  ROCKET_CHECK(file, file != nullptr);
}

FileSource::FileSource(const filesystem::path& path) :
  file_(io::open(path, "rb")),
  closeOnDestroy_(true) {
  if (file_ == nullptr) {
    throw InvalidState(fmt::format("Cannot open file `{}` for reading", path));
  }
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

istream&
FileSource::istream() {
  if (bad()) {
    throw InvalidState("`FileSource` is bad");
  }

  if (istream_ == nullptr) {
    istream_ = make_unique<FileIstream>(file_);
  }
  // The stream reads through `file_`, so it is always at the right position. Only clear what a previous use
  // left behind, such as `failbit` after a failed extraction
  istream_->clear();
  return *istream_;
}

optional<char>
FileSource::peek() {
  if (bad()) {
    return nullopt;
  }

  const i32 c = getc(file_);
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

string
FileSource::readln() {
  if (bad()) {
    return {};
  }

  string ret;
  for (i32 c = getc(file_); c != EOF; c = getc(file_)) {
    if (c == '\n') {
      removeTrailingCr(ret);
      break;
    }
    ret.push_back(static_cast<char>(c));
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

  return io::seek(file_, offset, origin);
}

u64
FileSource::tell() {
  if (bad()) {
    return NPOS;
  }

  return io::tell(file_);
}

// `NullSource` ---------------------------------------------------------------------------------------------

istream&
NullSource::istream() {
  if (istream_ == nullptr) {
    istream_ = make_unique<io::ispanstream>(span<const char>());
  }
  istream_->clear();
  return *istream_;
}

// `SpanSource` ---------------------------------------------------------------------------------------------

SpanSource::SpanSource(span<const u8> in) :
  in_(in) {}

istream&
SpanSource::istream() {
  if (istream_ == nullptr) {
    const span<const char> chars(reinterpret_cast<const char*>(in_.data()), in_.size());
    istream_ = make_unique<io::ispanstream>(chars);
  }
  istream_->clear(); // `seekg` does nothing while `failbit` is set
  istream_->seekg(static_cast<istream::off_type>(pos_), ios::beg);
  return *istream_;
}

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

  const auto c = is_.peek();
  if (c == istream::traits_type::eof()) {
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

string
StreamSource::readln() {
  if (bad()) {
    return {};
  }

  string ret;
  // `getline` sets `eofbit` only if it hit the end, so `eof()` tells whether the line had a line feed
  if (getline(is_, ret) && not is_.eof()) {
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

u64
StreamSource::tell() {
  if (bad()) {
    return NPOS;
  }

  // `io::tell` asks the stream buffer and leaves the state of the stream alone, unlike `tellg`. A stream that
  // cannot seek, such as one over a pipe, has no position
  return io::tell(is_);
}

// `StringSource` -------------------------------------------------------------------------------------------

StringSource::StringSource(string_view in) :
  in_(in) {}

istream&
StringSource::istream() {
  if (istream_ == nullptr) {
    const span<const char> chars(in_.data(), in_.size());
    istream_ = make_unique<io::ispanstream>(chars);
  }
  istream_->clear(); // `seekg` does nothing while `failbit` is set
  istream_->seekg(static_cast<istream::off_type>(pos_), ios::beg);
  return *istream_;
}

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
