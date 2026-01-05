/*
 * nio.cc
 */

#include "nio.h"

#include "rocket/assert.h"
#include "rocket/numeric.h"

#include <cstdio>
#include <iostream>
#include <unistd.h>

using namespace rocket;
using namespace rocket::nio;
using namespace std;

/* Logging --------------------------------------------------------------------------------------------------

Because the logging framework utilizes `nio`, we can't use it to log `nio` itself. So we need to make up a
tiny logging facility here.

---------------------------------------------------------------------------------------------------------- */

// #define NIO_LOG // Use this to activate logging

#ifdef NIO_LOG
#define LOG(func, args) cout << "# " << #func << ": " << args << endl;
#else
#define LOG(func, args)
#endif

namespace rocket::nio {

// `Io` -----------------------------------------------------------------------------------------------------

bool
Io::checkOpen() {
  if (not open_) {
    if (error_ == 0) {
      error_ = EBADF;
    }
    return false;
  }
  return true;
}

// `Sink` ---------------------------------------------------------------------------------------------------

size_t
Sink::writeln(std::string_view in) {
  if (not checkOpen()) {
    return 0;
  }

  auto ret = write(in);
  ret += write('\n');
  flush();
  return ret;
}

// `BufferedSink` -------------------------------------------------------------------------------------------

BufferedSink::BufferedSink(Sink& underlying, size_t size) :
    underlying_(underlying),
    size_(size) {
  ROCKET_CHECK(size, size >= MIN_BUFFER_SIZE);
  buf_ = make_unique<char[]>(size);
}

BufferedSink::~BufferedSink() {
  close();
}

int
BufferedSink::close() {
  if (not checkOpen()) {
    return error();
  }

  flush();

  size_ = 0;
  buf_ = nullptr;
  pos_ = 0;
  return underlying_.close();
}

int
BufferedSink::fd() {
  if (not checkOpen()) {
    return -1;
  }

  return underlying_.fd();
}

int
BufferedSink::flush() {
  if (not checkOpen()) {
    return error();
  }

  flushBuffer();
  return underlying_.flush();
}

void
BufferedSink::flushBuffer() {
  ROCKET_ASSERT(buf_);

  if (pos_ > 0) {
    LOG(BufferedSink::flushBuffer, "Flushing " << pos_ << " bytes from buffer to underlying")
    underlying_.write(string_view(&buf_[0], pos_));
    pos_ = 0;
  }
}

size_t
BufferedSink::write(string_view in) {
  if (not checkOpen()) {
    return error();
  }

  // Loop while there is data to write

  auto rest = in;
  while (not rest.empty()) {
    // Find out if the buffer can fulfill the request

    size_t available = size_ - pos_;
    if (rest.size() <= available) {
      // Yes, it can: Store the rest in the buffer, exit loop
      memcpy(&buf_[pos_], rest.data(), rest.size());
      LOG(BufferedSink::write, "Buffer can fulfill request, storing " << rest.size() << " bytes in buffer");
      pos_ += rest.size();
      break;
    }

    // Fill and flush the buffer, continue in loop

    memcpy(&buf_[pos_], rest.data(), available);
    LOG(BufferedSink::write, "Storing " << available << " available bytes in buffer");
    pos_ += available;
    rest = rest.substr(available);
    flushBuffer();
  }

  return in.size();
}

// `FileSink` -----------------------------------------------------------------------------------------------

FileSink::FileSink(FILE* file, const Params& params) :
    file_(file),
    params_(params) {
  ROCKET_CHECK(file, file != nullptr);
  if (int fd = this->fd(); fd == STDOUT_FILENO || fd == STDERR_FILENO) {
    params_.closeOnDestroy = false;
  }
}

FileSink::FileSink(const string& path, const Params& params) :
    file_(nullptr),
    params_(params) {
  const char* modes = params.append ? "ab" : "wb"; // `b` is for non-Linux only
  file_ = std::fopen(path.c_str(), modes);
  LOG(FileSink::ctor, "fopen=" << file_ << ", ferror=" << (file_ ? ferror(file_) : -1));

  if (file_ == nullptr) {
    error_ = ENOENT;
    open_ = false;
  } else {
    error_ = ferror(file_);
  }
}

FileSink::~FileSink() {
  if (params_.closeOnDestroy) {
    close();
  }
}

int
FileSink::close()
{
  if (not checkOpen()) {
    return error_;
  }

  flush();

  int ret = std::fclose(file_);
  LOG(FileSink::ctor, "fclose=" << ret << ", ferror=" << ferror(file_));
  if (ret != 0) {
    error_ = ferror(file_);
  }
  open_ = false;
  file_ = nullptr;
  return ret;
}

int
FileSink::fd() {
  if (not checkOpen()) {
    return -1;
  }

  return fileno(file_);
}

int
FileSink::flush() {
  if (not checkOpen()) {
    return error_;
  }

  int ret = std::fflush(file_);
  LOG(FileSink::flush, "fflush=" << ret << ", ferror=" << ferror(file_));
  if (ret != 0) {
    error_ = ferror(file_);
  }
  return ret;
}

size_t
FileSink::write(string_view in) {
  if (not checkOpen()) {
    return error_;
  }

  size_t ret = std::fwrite(in.data(), 1, in.size(), file_);
  LOG(FileSink::write, "fwrite=" << ret << ", in.size=" << in.size() << ", ferror=" << ferror(file_));
  error_ = ferror(file_);
  ROCKET_ASSERT(ret == in.size() || error_ != 0);
  return ret;
}

// `NullSink` -----------------------------------------------------------------------------------------------

NullSink::~NullSink() {
  close();
}

int
NullSink::close()
{
  if (not checkOpen()) {
    return error_;
  }

  open_ = false;
  return 0;
}

int
NullSink::fd() {
  checkOpen();
  return -1;
}

int
NullSink::flush() {
  checkOpen();
  return error_;
}

size_t
NullSink::write(string_view in) {
  checkOpen();
  return 0;
}

// `SpanSink` -----------------------------------------------------------------------------------------------

SpanSink::~SpanSink() {
  close();
}

int
SpanSink::close() {
  if (not checkOpen()) {
    return error_;
  }

  open_ = false;
  return 0;
}

int
SpanSink::flush() {
  checkOpen();
  return error_;
}

size_t
SpanSink::write(string_view in) {
  if (not checkOpen()) {
    return error_;
  }

  size_t available = out_.size() - pos_;
  size_t ret = min(available, in.size());
  if (ret > 0) {
    memcpy(&out_[pos_], in.data(), ret);
    pos_ += ret;
  }
  return ret;
}

// `StreamSink` ---------------------------------------------------------------------------------------------

StreamSink::~StreamSink() {
  close();
}

int
StreamSink::close() {
  if (not checkOpen()) {
    return error_;
  }

  flush();

  open_ = false;
  // An `ostream` can't close, it can only be destroyed
  return 0;
}

int
StreamSink::fd() {
  if (not checkOpen()) {
    return -1;
  }

  if (&os_ == &cout) {
    return STDOUT_FILENO;
  } else if (&os_ == &cerr) {
    return STDERR_FILENO;
  } else {
    return -1;
  }
}

int
StreamSink::flush() {
  if (not checkOpen()) {
    return error_;
  }

  os_.flush();
  LOG(StreamSink::flush, "bad=" << os_.bad() << ", fail=" << os_.fail() << ", eof=" << os_.eof());
  error_ = os_.rdstate();
  return error_;
}

size_t
StreamSink::write(string_view in) {
  if (not checkOpen()) {
    return error_;
  }

  size_t ret = os_.rdbuf()->sputn(in.data(), in.size());
  if (ret != in.size()) {
    os_.setstate(ios_base::badbit);
  }
  LOG(StreamSink::write, "rdbuf()->sputn=" << ret << ", bad=" << os_.bad() << ", fail=" << os_.fail() << ", eof=" << os_.eof());
  error_ = os_.rdstate();
  return ret;
}

// `StringSink` ---------------------------------------------------------------------------------------------

StringSink::~StringSink() {
  close();
}

int
StringSink::close() {
  if (not checkOpen()) {
    return error_;
  }

  open_ = false;
  return 0;
}

int
StringSink::flush() {
  checkOpen();
  return error_;
}

string
StringSink::str() const {
  ROCKET_EXPECT(not out);
  return managed_;
}

size_t
StringSink::write(string_view in) {
  if (not checkOpen()) {
    return 0;
  }

  if (out) {
    out->append(in);
  } else {
    managed_.append(in);
  }
  return in.size();
}

// `Source` -------------------------------------------------------------------------------------------------

string
Source::read() {
  if (not checkOpen()) {
    return string();
  }

  string ret;
  auto buf = make_unique<char[]>(DEFAULT_BUFFER_SIZE);
  span<char> out(&buf[0], DEFAULT_BUFFER_SIZE);
  while (true) {
    size_t n = read(out);
    if (n > 0) {
      ret.append(out.data(), n);
    }
    if (n != out.size()) {
      break;
    }
  }
  return ret;
}

string
Source::readln() {
  if (not checkOpen()) {
    return string();
  }

  string ret;
  bool crlf = false;

  while (true) {
    char c;
    size_t result = read(c);
    if (result == 0) {
      break;
    }
    if (c == '\n') {
      crlf = true;
      break;
    }
    ret.push_back(c);
  }

  // Remove trailing `\r` if it precedes the `\n`
  if (crlf && not ret.empty() && *ret.rbegin() == '\r') {
    ret.pop_back();
  }

  return ret;
}

size_t
Source::readln(span<char> out) {
  if (not checkOpen()) {
    return 0;
  }

  auto it = out.begin();
  bool crlf = false;

  while (it != out.end()) {
    char c;
    size_t result = read(c);
    if (result == 0) {
      break;
    }
    if (c == '\n') {
      crlf = true;
      break;
    }
    *(it++) = c;
  }

  // Remove trailing `\r` if it precedes the `\n`
  size_t ret = it - out.begin();
  if (crlf && ret > 0 && *(it - 1) == '\r') {
    --ret;
  }
  return ret;
}

// `BufferedSource` -----------------------------------------------------------------------------------------

BufferedSource::BufferedSource(Source& underlying, size_t size) :
    underlying_(underlying),
    size_(size) {
  ROCKET_CHECK(size, size >= MIN_BUFFER_SIZE);
  buf_ = make_unique<char[]>(size);
  bufPos_ = underlying.tell();
}

BufferedSource::~BufferedSource() {
  close();
}

int
BufferedSource::close() {
  if (not checkOpen()) {
    return error();
  }

  size_ = 0;
  buf_ = nullptr;
  bufPos_ = -1;
  pos_ = 0;
  end_ = 0;
  return underlying_.close();
}

int
BufferedSource::fd() {
  if (not checkOpen()) {
    return -1;
  }

  return underlying_.fd();
}

size_t
BufferedSource::read(span<char> out) {
  if (not checkOpen()) {
    return 0;
  }

  // Loop while there is data to read

  size_t ret = 0;

  auto rest = out;
  while (not rest.empty()) {
    // If needed, initialize buffer

    if (pos_ == end_) {
      bufPos_ = underlying_.tell();
      pos_ = 0;
      end_ = underlying_.read(span<char>(&buf_[0], size_));
      LOG(BufferedSource::read, "Initialized buffer with " << end_ << " bytes from underlying; bufPos=" << bufPos_ << ", pos=" << pos_ << ", end=" << end_);
      if (end_ == 0) {
        break;
      }
    }

    // Find out if the buffer can fulfill the request

    size_t available = end_ - pos_;
    if (rest.size() <= available) {
      // Yes, it can: Copy the buffer to the rest, exit loop
      LOG(BufferedSource::read, "Buffer can fulfill request, copying " << rest.size() << " bytes from buffer");
      memcpy(rest.data(), &buf_[pos_], rest.size());
      pos_ += rest.size();
      ret += rest.size();
      break;
    }

    // Flush the buffer, continue in loop

    LOG(BufferedSource::read, "Copying " << available << " available bytes from buffer");
    memcpy(rest.data(), &buf_[pos_], available);
    pos_ += available;
    ret += available;
    rest = rest.subspan(available);
    if (end_ < size_) {
      break;
    }
  }

  ROCKET_ASSERT(ret <= out.size());
  return ret;
}

int
BufferedSource::seek(Offset offset, SeekMode mode) {
  if (not checkOpen()) {
    return error_;
  }

  // Get the old position so we can restore it later
  Position oldTell = underlying_.tell();
  if (oldTell == -1) {
    LOG(BufferedSource::seek, "Getting old position failed; invalidating buffer");
    bufPos_ = -1;
    pos_ = end_ = 0;
    return EIO;
  }

  // Do the job
  int ret = underlying_.seek(offset, mode);

  // Get the new position se we can see if we have a buffer hit
  Position newTell = underlying_.tell();
  if (newTell == -1) {
    LOG(BufferedSource::seek, "Getting new position failed; invalidating buffer");
    bufPos_ = -1;
    pos_ = end_ = 0;
    return ret;
  }

  // Do we know at all where we are?
  if (bufPos_ == -1) {
    LOG(BufferedSource::seek, "Buffer position is unknown; invalidating the buffer");
    bufPos_ = newTell;
    pos_ = end_ = 0;
    return ret;
  }

  // Do we have a buffer hit?
  Position ourPos = newTell - bufPos_;
  if (ourPos <= end_) {
    // Yes, we do: Update our position and restore the underlying position
    LOG(BufferedSource::seek, "Going from " << pos_ << " to " << ourPos);
    pos_ = ourPos;
    return underlying_.seek(oldTell);
  }

  // No buffer hit
  LOG(BufferedSource::seek, "New position is beyond the buffer; invalidating buffer");
  bufPos_ = newTell;
  pos_ = end_ = 0;
  return ret;
}

Io::Position
BufferedSource::tell() {
  if (bufPos_ == -1) {
    return -1;
  }
  return bufPos_ + pos_;
}

// `FileSource` ---------------------------------------------------------------------------------------------

FileSource::FileSource(FILE* file, const Params& params) :
    file_(file),
    params_(params) {
  ROCKET_CHECK(file, file != nullptr);
  if (int fd = this->fd(); fd == STDIN_FILENO) {
    params_.closeOnDestroy = false;
  }
}

FileSource::FileSource(const string& path, const Params& params) :
    file_(nullptr),
    params_(params) {
  file_ = std::fopen(path.c_str(), "rb");  // `b` is for non-Linux only
  LOG(FileSource::ctor, "fopen=" << file_ << ", ferror=" << (file_ ? ferror(file_) : -1));

  if (file_ == nullptr) {
    error_ = ENOENT;
    open_ = false;
  } else {
    error_ = ferror(file_);
  }
}

FileSource::~FileSource() {
  if (params_.closeOnDestroy) {
    close();
  }
}

int
FileSource::close()
{
  if (not checkOpen()) {
    return error_;
  }

  int ret = std::fclose(file_);
  LOG(FileSource::close, "fclose=" << ret);
  error_ = ret;
  open_ = false;
  file_ = nullptr;
  return ret;
}

int
FileSource::fd() {
  if (not checkOpen()) {
    return -1;
  }

  return fileno(file_);
}

size_t
FileSource::read(span<char> out) {
  if (not checkOpen()) {
    return 0;
  }

  size_t ret = std::fread(out.data(), 1, out.size(), file_);
  LOG(FileSource::read, "fread=" << ret << ", out.size=" << out.size() << ", ferror=" << ferror(file_));
  error_ = ferror(file_);
  return ret;
}

int
FileSource::seek(Offset offset, SeekMode mode) {
  if (not checkOpen()) {
    return error_;
  }

  int origin;
  switch (mode) {
  case SeekMode::beg:
    origin = SEEK_SET;
    break;
  case SeekMode::cur:
    origin = SEEK_CUR;
    break;
  case SeekMode::end:
    origin = SEEK_END;
    break;
  default:
    ROCKET_FAIL_UNREACHABLE_CODE();
  }


  // The type of the `offset` parameter is `long`, se we can directly pass `offset`
  static_assert(is_same_v<Offset, long>);
  size_t ret = std::fseek(file_, offset, origin);
  LOG(FileSource::seek, "fseek=" << ret << ", ferror=" << ferror(file_));
  if (ret != 0) {
    error_ = ferror(file_);
  }
  return ret;
}

Io::Position
FileSource::tell() {
  if (not checkOpen()) {
    return -1;
  }

  using ftell_t = decltype(std::ftell(file_));
  static_assert(is_same_v<ftell_t, long>);
  long ret = std::ftell(file_);
  LOG(FileSource::tell, "ftell=" << ret << ", ferror=" << ferror(file_));
  if (ret == -1) {
    error_ = ferror(file_);
    return -1;
  }
  ROCKET_ASSERT(ret >= 0);
  // Convert nonnegative `long` to `Position`
  return ret;
}

// `NullSource` ---------------------------------------------------------------------------------------------

NullSource::~NullSource() {
  close();
}

int
NullSource::close()
{
  if (not checkOpen()) {
    return error_;
  }

  open_ = false;
  return 0;
}

size_t
NullSource::read(span<char> out) {
  checkOpen();
  return 0;
}

int
NullSource::seek(Offset offset, SeekMode mode) {
  checkOpen();
  return EINVAL;
}

Io::Position
NullSource::tell() {
  checkOpen();
  return -1;
}

// `StreamSource` -------------------------------------------------------------------------------------------

StreamSource::~StreamSource() {
  close();
}

int
StreamSource::close() {
  if (not checkOpen()) {
    return error_;
  }

  open_ = false;
  // An `istream` can't close, it can only be destroyed
  return 0;
}

int
StreamSource::fd() {
  if (not checkOpen()) {
    return -1;
  }

  if (&is_ == &cin) {
    return STDIN_FILENO;
  } else {
    return -1;
  }
}

size_t
StreamSource::read(span<char> out) {
  if (not checkOpen()) {
    return 0;
  }

  // If less bytes than `out.size()` are read, `bad`, `fail`, and `eof` all remain `false``
  size_t ret = is_.readsome(out.data(), out.size());
  LOG(StreamSource::read, "readsome=" << ret << ", out.size=" << out.size() << ", bad=" << is_.bad() << ", fail=" << is_.fail() << ", eof=" << is_.eof());
  error_ = is_.rdstate();
  return ret;
}

int
StreamSource::seek(Offset offset, SeekMode mode) {
  if (not checkOpen()) {
    return error_;
  }

  ios::seekdir dir;
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
    ROCKET_FAIL_UNREACHABLE_CODE();
  }

  // `istream::off_type` is `long`, so we can directly pass `offset`
  static_assert(is_same_v<istream::off_type, long>);
  is_.seekg(offset, dir);
  error_ = is_.rdstate();
  return error_;
}

Io::Position
StreamSource::tell() {
  if (not checkOpen()) {
    return -1;
  }

  using tellg_t = decltype(is_.tellg());
  // It is some 128-bit type, we don't know whether it is signed or unsigned
  static_assert(sizeof(tellg_t) == 16);
  tellg_t ret = is_.tellg();
  LOG(StreamSource::tell, "tellg=" << ret << ", bad=" << is_.bad() << ", fail=" << is_.fail() << ", eof=" << is_.eof());
  error_ = is_.rdstate();

  if (ret < 0 || ret > numeric_limits<long>::max()) {
    return -1;
  }
  return static_cast<Position>(ret);
}

// `StringSource` -------------------------------------------------------------------------------------------

StringSource::~StringSource() {
  close();
}

int
StringSource::close()
{
  if (not checkOpen()) {
    return error_;
  }

  open_ = false;
  pos_ = 0;
  return 0;
}

size_t
StringSource::read(span<char> out) {
  if (not checkOpen()) {
    return 0;
  }

  size_t ret = min(out.size(), in_.size() - pos_);
  if (ret > 0) {
    memcpy(out.data(), in_.data() + pos_, ret);
    pos_ += ret;
  }
  return ret;
}

int
StringSource::seek(Offset offset, SeekMode mode) {
  if (not checkOpen()) {
    return error_;
  }

  // XXX
  int128_t newPos;
  switch (mode) {
  case SeekMode::beg:
    newPos = to<int128_t>(offset);
    break;
  case SeekMode::cur:
    newPos = add<int128_t>(pos_, offset);
    break;
  case SeekMode::end:
    newPos = add<int128_t>(in_.size(), offset);
    break;
  default:
    ROCKET_FAIL_UNREACHABLE_CODE();
  }

  newPos = max<int128_t>(0, newPos);
  newPos = min<int128_t>(in_.size(), newPos);
  pos_ = static_cast<Position>(newPos);
  return 0;
}

Io::Position
StringSource::tell() {
  if (not checkOpen()) {
    return -1;
  }

  return pos_;
}

} // namespace rocket::nio

// Variables ------------------------------------------------------------------------------------------------

namespace {

FileSink fileSinkStdout = FileSink(::stdout);
FileSink fileSinkStderr = FileSink(::stderr);
FileSource fileSourceStdin = FileSource(::stdin);

} // namespace

namespace rocket::nio {

Sink& stdout = fileSinkStdout;
Sink& stderr = fileSinkStderr;
Source& stdin = fileSourceStdin;

} // namespace rocket::nio

// EOF
