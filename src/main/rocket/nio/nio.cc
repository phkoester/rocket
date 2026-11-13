/*
 * nio.cc
 */

#include "nio.h"

#include "rocket/assert.h"
#include "rocket/io/io.h"
#include "rocket/unicode/unicode.h"

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

#include <algorithm>
#include <cstring>
#include <exception>
#include <iostream>
#include <optional>

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
  flush();
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
  if (&os_ == &cerr) {
    return STDERR_FILENO;
  }
  return -1;
}

u64
StreamSink::write(span<const u8> in) {
  if (bad()) {
    return 0;
  }

  const auto result = os_.rdbuf()->sputn(reinterpret_cast<const char*>(in.data()), static_cast<streamsize>(in.size()));
  return result > 0 ? static_cast<u64>(result) : 0;
}

// `StringSink` ---------------------------------------------------------------------------------------------

StringSink::StringSink(string& ref) :
  ptr_(&ref) {}

u64
StringSink::write(span<const u8> in) {
  if (bad()) {
    return 0;
  }

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

  // Remove trailing '\r' if it precedes the '\n'
  if (lf && not ret.empty() && ret.back() == '\r') {
    ret.pop_back();
  }

  return ret;
}

// `ContiguousSource` ---------------------------------------------------------------------------------------

optional<char>
ContiguousSource::peek() {
  if (bad()) {
    return nullopt;
  }

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
  if (bad()) {
    return {};
  }

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
  if (bad()) {
    return unexpected(Bad);
  }

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
    namespace bio = boost::iostreams;
    using FileDescriptorIstream = bio::stream<bio::file_descriptor_source>;
    istream_ = make_unique<FileDescriptorIstream>(handle(), bio::never_close_handle);
  }
  if (const u64 pos = tell(); pos != NPOS) {
    istream_->seekg(static_cast<istream::off_type>(pos), ios::beg);
  }
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
  if (bad()) {
    throw InvalidState("`NullSource` is bad");
  }

  if (istream_ == nullptr) {
    istream_ = make_unique<io::ispanstream>(span<const char>());
  }
  return *istream_;
}

// `SpanSource` ---------------------------------------------------------------------------------------------

SpanSource::SpanSource(span<const u8> in) :
  in_(in) {}

istream&
SpanSource::istream() {
  if (bad()) {
    throw InvalidState("`SpanSource` is bad");
  }

  if (istream_ == nullptr) {
    const span<const char> chars(reinterpret_cast<const char*>(in_.data()), in_.size());
    istream_ = make_unique<io::ispanstream>(chars);
  }
  istream_->seekg(static_cast<istream::off_type>(pos_), ios::beg);
  return *istream_;
}

u64
SpanSource::read(span<u8> out) {
  if (bad()) {
    return 0;
  }

  const u64 ret = min<u64>(out.size(), in_.size() - pos_);
  if (ret > 0) {
    memcpy(out.data(), in_.data() + pos_, ret);
    pos_ += ret;
  }
  return ret;
}

bool
SpanSource::seek(i64 offset, SeekMode mode) {
  if (bad()) {
    return false;
  }

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

  // We need to clear the fail bit, otherwise the seek will fail
  is_.clear(is_.rdstate() & ~ios_base::failbit);
  is_.seekg(static_cast<istream::off_type>(offset), dir);
  return not bad();
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
  if (bad()) {
    throw InvalidState("`StringSource` is bad");
  }

  if (istream_ == nullptr) {
    const span<const char> chars(in_.data(), in_.size());
    istream_ = make_unique<io::ispanstream>(chars);
  }
  istream_->seekg(static_cast<istream::off_type>(pos_), ios::beg);
  return *istream_;
}

u64
StringSource::read(span<u8> out) {
  if (bad()) {
    return 0;
  }

  const u64 ret = min<u64>(out.size(), in_.size() - pos_);
  if (ret > 0) {
    memcpy(out.data(), in_.data() + pos_, ret);
    pos_ += ret;
  }
  return ret;
}

bool
StringSource::seek(i64 offset, SeekMode mode) { // NOLINT
  if (bad()) {
    return false;
  }

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
