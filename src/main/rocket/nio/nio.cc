/*
 * nio.cc
 */

#include "nio.h"

#include "rocket/assert.h"

#include <cstdio>
#include <iostream>
#ifdef ROCKET_OS_WINDOWS
#include <windows.h>
#endif

using namespace rocket;
using namespace rocket::nio;
using namespace std;

using boost::safe_numerics::safe;

/* Logging --------------------------------------------------------------------------------------------------

Because the logging framework utilizes `nio`, we can't use it to log `nio` itself. So we need to make up a
tiny logging facility here.

---------------------------------------------------------------------------------------------------------- */

// #define NIO_LOG // Use this to activate logging

#ifdef NIO_LOG
#define LOG(args) cout << "# " << __FILE__ << ':' << __LINE__ << ' ' << __FUNCTION__ << ": " << args << endl;
#else
#define LOG(args)
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

u64
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

BufferedSink::BufferedSink(Sink& underlying, u64 size) :
    underlying_(underlying),
    size_(size) {
  ROCKET_CHECK(size, size >= MIN_BUFFER_SIZE);
  buf_ = make_unique<char[]>(size);
}

BufferedSink::~BufferedSink() {
  close();
}

i32
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

i32
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
    LOG("Flushing " << pos_ << " bytes from buffer to underlying")
    underlying_.write(string_view(&buf_[0], pos_));
    pos_ = 0;
  }
}

bool
BufferedSink::terminal(i32* fd) {
  if (not checkOpen()) {
    return false;
  }
  return underlying_.terminal(fd);
}

u64
BufferedSink::write(string_view in) {
  if (not checkOpen()) {
    return error();
  }

  // Loop while there is data to write

  auto rest = in;
  while (not rest.empty()) {
    // Find out if the buffer can fulfill the request

    u64 available = size_ - pos_;
    if (rest.size() <= available) {
      // Yes, it can: Store the rest in the buffer, exit loop
      memcpy(&buf_[pos_], rest.data(), rest.size());
      LOG("Buffer can fulfill request, storing " << rest.size() << " bytes in buffer");
      pos_ += rest.size();
      break;
    }

    // Fill and flush the buffer, continue in loop

    memcpy(&buf_[pos_], rest.data(), available);
    LOG("Storing " << available << " available bytes in buffer");
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
  if (file == stdout || file == stderr) {
    params_.closeOnDestroy = false;
  }
}

FileSink::FileSink(const string& path, const Params& params) :
    file_(nullptr),
    params_(params) {
  const char* modes = params.append ? "ab" : "wb"; // `b` is for non-Linux only
  file_ = std::fopen(path.c_str(), modes);
  LOG("fopen=" << file_ << ", ferror=" << (file_ ? ferror(file_) : -1));

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

i32
FileSink::close()
{
  if (not checkOpen()) {
    return error_;
  }

  flush();

  i32 ret = std::fclose(file_);
  // `file_` is probably invalid now, so we don't call `ferror` on it
  LOG("fclose=" << ret);
  if (ret != 0) {
    error_ = EIO;
  }
  open_ = false;
  file_ = nullptr;
  return ret;
}

i32
FileSink::flush() {
  if (not checkOpen()) {
    return error_;
  }

  i32 ret = std::fflush(file_);
  LOG("fflush=" << ret << ", ferror=" << ferror(file_));
  if (ret != 0) {
    error_ = ferror(file_);
  }
  return ret;
}

bool
FileSink::terminal(i32* fd) {
  if (not checkOpen()) {
    return false;
  }

  auto handle = fileno(file_);
  if (handle == -1) {
    return false;
  }
  bool ret = isatty(handle);
  if (ret && fd) {
    *fd = handle;
  }
  return ret;
}

u64
FileSink::write(string_view in) {
  if (not checkOpen()) {
    return error_;
  }

  u64 ret = std::fwrite(in.data(), 1, in.size(), file_);
  LOG("fwrite=" << ret << ", in.size=" << in.size() << ", ferror=" << ferror(file_));
  error_ = ferror(file_);
  ROCKET_ASSERT(ret == in.size() || error_ != 0);
  return ret;
}

// `NullSink` -----------------------------------------------------------------------------------------------

NullSink::~NullSink() {
  close();
}

i32
NullSink::close()
{
  if (not checkOpen()) {
    return error_;
  }

  open_ = false;
  return 0;
}

i32
NullSink::flush() {
  checkOpen();
  return error_;
}

bool
NullSink::terminal(i32*) {
  checkOpen();
  return false;
}

u64
NullSink::write(string_view) {
  checkOpen();
  return 0;
}

// `SpanSink` -----------------------------------------------------------------------------------------------

SpanSink::~SpanSink() {
  close();
}

i32
SpanSink::close() {
  if (not checkOpen()) {
    return error_;
  }

  open_ = false;
  return 0;
}

i32
SpanSink::flush() {
  checkOpen();
  return error_;
}

bool
SpanSink::terminal(i32*) {
  checkOpen();
  return false;
}

u64
SpanSink::write(string_view in) {
  if (not checkOpen()) {
    return error_;
  }

  u64 available = out_.size() - pos_;
  u64 ret = min(available, in.size());
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

i32
StreamSink::close() {
  if (not checkOpen()) {
    return error_;
  }

  flush();

  open_ = false;
  // An `ostream` can't close, it can only be destroyed
  return 0;
}

i32
StreamSink::flush() {
  if (not checkOpen()) {
    return error_;
  }

  os_.flush();
  LOG("bad=" << os_.bad() << ", fail=" << os_.fail() << ", eof=" << os_.eof());
  error_ = os_.rdstate();
  return error_;
}

bool
StreamSink::terminal(i32* fd) {
  if (not checkOpen()) {
    return false;
  }

  if (&os_ == &cout) {
    if (isatty(STDOUT_FILENO)) {
      if (fd) {
        *fd = STDOUT_FILENO;
      }
      return true;
    }
  } else if (&os_ == &cerr) {
    if (isatty(STDERR_FILENO)) {
      if (fd) {
        *fd = STDERR_FILENO;
      }
      return true;
    }
  }
  return false;
}

u64
StreamSink::write(string_view in) {
  if (not checkOpen()) {
    return error_;
  }

  u64 ret = os_.rdbuf()->sputn(in.data(), in.size());
  if (ret != in.size()) {
    os_.setstate(ios_base::badbit);
  }
  LOG("rdbuf()->sputn=" << ret << ", bad=" << os_.bad() << ", fail=" << os_.fail() << ", eof=" << os_.eof());
  error_ = os_.rdstate();
  return ret;
}

// `StringSink` ---------------------------------------------------------------------------------------------

StringSink::~StringSink() {
  close();
}

i32
StringSink::close() {
  if (not checkOpen()) {
    return error_;
  }

  open_ = false;
  return 0;
}

i32
StringSink::flush() {
  checkOpen();
  return error_;
}

bool
StringSink::terminal(i32*) {
  checkOpen();
  return false;
}

u64
StringSink::write(string_view in) {
  if (not checkOpen()) {
    return 0;
  }

  if (ptr_) {
    ptr_->append(in);
  } else {
    owned_.append(in);
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
    u64 n = read(out);
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
    u64 result = read(c);
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

u64
Source::readln(span<char> out) {
  if (not checkOpen()) {
    return 0;
  }

  auto it = out.begin();
  bool crlf = false;

  while (it != out.end()) {
    char c;
    u64 result = read(c);
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
  u64 ret = it - out.begin();
  if (crlf && ret > 0 && *(it - 1) == '\r') {
    --ret;
  }
  return ret;
}

// `BufferedSource` -----------------------------------------------------------------------------------------

BufferedSource::BufferedSource(Source& underlying, u64 size) :
    underlying_(underlying),
    size_(size) {
  ROCKET_CHECK(size, size >= MIN_BUFFER_SIZE);
  buf_ = make_unique<char[]>(size);
  bufPos_ = underlying.tell();
}

BufferedSource::~BufferedSource() {
  close();
}

i32
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

u64
BufferedSource::read(span<char> out) {
  if (not checkOpen()) {
    return 0;
  }

  // Loop while there is data to read

  u64 ret = 0;

  auto rest = out;
  while (not rest.empty()) {
    // If needed, initialize buffer

    if (pos_ == end_) {
      bufPos_ = underlying_.tell();
      pos_ = 0;
      end_ = underlying_.read(span<char>(&buf_[0], size_));
      LOG("Initialized buffer with " << end_ << " bytes from underlying; bufPos=" << bufPos_ << ", pos=" << pos_ << ", end=" << end_);
      if (end_ == 0) {
        break;
      }
    }

    // Find out if the buffer can fulfill the request

    u64 available = end_ - pos_;
    if (rest.size() <= available) {
      // Yes, it can: Copy the buffer to the rest, exit loop
      LOG("Buffer can fulfill request, copying " << rest.size() << " bytes from buffer");
      memcpy(rest.data(), &buf_[pos_], rest.size());
      pos_ += rest.size();
      ret += rest.size();
      break;
    }

    // Flush the buffer, continue in loop

    LOG("Copying " << available << " available bytes from buffer");
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

i32
BufferedSource::seek(i64 offset, SeekMode mode) {
  if (not checkOpen()) {
    return error_;
  }

  // Get the old position so we can restore it later
  u64 oldTell = underlying_.tell();
  if (oldTell == NPOS) {
    LOG("Getting old position failed; invalidating buffer");
    bufPos_ = NPOS;
    pos_ = end_ = 0;
    return EIO;
  }

  // Do the job
  i32 ret = underlying_.seek(offset, mode);

  // Get the new position se we can see if we have a buffer hit
  u64 newTell = underlying_.tell();
  if (newTell == NPOS) {
    LOG("Getting new position failed; invalidating buffer");
    bufPos_ = NPOS;
    pos_ = end_ = 0;
    return ret;
  }

  // Do we know at all where we are?
  if (bufPos_ == NPOS) {
    LOG("Buffer position is unknown; invalidating the buffer");
    bufPos_ = newTell;
    pos_ = end_ = 0;
    return ret;
  }

  // Do we have a buffer hit?
  u64 ourPos = newTell - bufPos_;
  if (ourPos <= end_) {
    // Yes, we do: Update our position and restore the underlying position
    LOG("Going from " << pos_ << " to " << ourPos);
    pos_ = ourPos;
    return underlying_.seek(oldTell);
  }

  // No buffer hit
  LOG("New position is beyond the buffer; invalidating buffer");
  bufPos_ = newTell;
  pos_ = end_ = 0;
  return ret;
}

u64
BufferedSource::tell() {
  if (bufPos_ == NPOS) {
    return NPOS;
  }
  return bufPos_ + pos_;
}

bool
BufferedSource::terminal(i32* fd) {
  if (not checkOpen()) {
    return false;
  }
  return underlying_.terminal(fd);
}

// `FileSource` ---------------------------------------------------------------------------------------------

FileSource::FileSource(FILE* file, const Params& params) :
    file_(file),
    params_(params) {
  if (file == stdin) {
    params_.closeOnDestroy = false;
  }
}

FileSource::FileSource(const string& path, const Params& params) :
    file_(nullptr),
    params_(params) {
  file_ = std::fopen(path.c_str(), "rb");  // `b` is for non-Linux only
  LOG("fopen=" << file_ << ", ferror=" << (file_ ? ferror(file_) : -1));

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

i32
FileSource::close()
{
  if (not checkOpen()) {
    return error_;
  }

  i32 ret = std::fclose(file_);
  LOG("fclose=" << ret);
  error_ = ret;
  open_ = false;
  file_ = nullptr;
  return ret;
}

u64
FileSource::read(span<char> out) {
  if (not checkOpen()) {
    return 0;
  }

  u64 ret = std::fread(out.data(), 1, out.size(), file_);
  LOG("fread=" << ret << ", out.size=" << out.size() << ", ferror=" << ferror(file_));
  error_ = ferror(file_);
  return ret;
}

i32
FileSource::seek(i64 offset, SeekMode mode) {
  if (not checkOpen()) {
    return error_;
  }

  i32 origin;
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
    ROCKET_FLOP(mode, "Invalid seek mode {}", static_cast<i32>(mode));
  }


  // The type of the `offset` parameter is `std_long`, se we can directly pass `offset`
  static_assert(is_same_v<decltype(offset), std_long>);
  i32 ret = std::fseek(file_, offset, origin);
  LOG("fseek=" << ret << ", ferror=" << ferror(file_));
  if (ret != 0) {
    error_ = ferror(file_);
  }
  return ret;
}

u64
FileSource::tell() {
  if (not checkOpen()) {
    return NPOS;
  }

  using ftell_t = decltype(std::ftell(file_));
  static_assert(is_same_v<ftell_t, i64>);
  i64 result = std::ftell(file_);
  LOG("ftell=" << result << ", ferror=" << ferror(file_));
  if (result == -1) {
    error_ = ferror(file_);
    return NPOS;
  }
  ROCKET_ASSERT(result >= 0);
  return static_cast<u64>(result); // We know `result` >= 0
}

bool
FileSource::terminal(i32* fd) {
  if (not checkOpen()) {
    return false;
  }

  auto handle = fileno(file_);
  if (handle == -1) {
    return false;
  }
  bool ret = isatty(handle);
  if (ret && fd) {
    *fd = handle;
  }
  return ret;
}

// `NullSource` ---------------------------------------------------------------------------------------------

NullSource::~NullSource() {
  close();
}

i32
NullSource::close()
{
  if (not checkOpen()) {
    return error_;
  }

  open_ = false;
  return 0;
}

u64
NullSource::read(span<char>) {
  checkOpen();
  return 0;
}

i32
NullSource::seek(i64, SeekMode) {
  checkOpen();
  return EINVAL;
}

u64
NullSource::tell() {
  checkOpen();
  return NPOS;
}

bool
NullSource::terminal(i32*) {
  checkOpen();
  return false;
}

// `StreamSource` -------------------------------------------------------------------------------------------

StreamSource::~StreamSource() {
  close();
}

i32
StreamSource::close() {
  if (not checkOpen()) {
    return error_;
  }

  open_ = false;
  // An `istream` can't close, it can only be destroyed
  return 0;
}

u64
StreamSource::read(span<char> out) {
  if (not checkOpen()) {
    return 0;
  }

  // If less bytes than `out.size()` are read, `bad`, `fail`, and `eof` all remain `false``
  u64 ret = is_.readsome(out.data(), out.size());
  LOG("readsome=" << ret << ", out.size=" << out.size() << ", bad=" << is_.bad() << ", fail=" << is_.fail() << ", eof=" << is_.eof());
  error_ = is_.rdstate();
  return ret;
}

i32
StreamSource::seek(i64 offset, SeekMode mode) {
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
    ROCKET_FLOP(mode, "Invalid seek mode {}", static_cast<i32>(mode));
  }

  // `istream::off_type` is `i64`, so we can directly pass `offset`
  static_assert(is_same_v<istream::off_type, i64>);
  is_.seekg(offset, dir);
  error_ = is_.rdstate();
  return error_;
}

u64
StreamSource::tell() {
  if (not checkOpen()) {
    return NPOS;
  }

  auto result = is_.tellg();
  LOG("tellg=" << result << ", bad=" << is_.bad() << ", fail=" << is_.fail() << ", eof=" << is_.eof());
  error_ = is_.rdstate();

  if (result < 0) {
    return NPOS;
  }
  return static_cast<u64>(result);
}

bool
StreamSource::terminal(i32* fd) {
  if (not checkOpen()) {
    return false;
  }

  if (&is_ == &cin) {
    if (isatty(STDIN_FILENO)) {
      if (fd) {
        *fd = STDIN_FILENO;
      }
      return true;
    }
  }
  return false;
}

// `StringSource` -------------------------------------------------------------------------------------------

StringSource::~StringSource() {
  close();
}

i32
StringSource::close()
{
  if (not checkOpen()) {
    return error_;
  }

  open_ = false;
  pos_ = 0;
  return 0;
}

u64
StringSource::read(span<char> out) {
  if (not checkOpen()) {
    return 0;
  }

  u64 ret = min(out.size(), in_.size() - static_cast<u64>(pos_));
  if (ret > 0) {
    memcpy(out.data(), in_.data() + static_cast<u64>(pos_), ret);
    pos_ += ret;
  }
  return ret;
}

i32
StringSource::seek(i64 offset, SeekMode mode) {
  if (not checkOpen()) {
    return error_;
  }

  switch (mode) {
  case SeekMode::beg:
    pos_ = offset;
    break;
  case SeekMode::cur:
    if (offset >= 0) {
      pos_ += offset;
    } else {
      pos_ -= (-offset);
    }
    break;
  case SeekMode::end:
    if (offset >= 0) {
      pos_ = safe<u64>(in_.size()) + offset;
    } else {
      pos_ = safe<u64>(in_.size()) - (-offset);
    }
    break;
  }
  pos_ = min(static_cast<u64>(pos_), in_.size());
  return 0;
}

u64
StringSource::tell() {
  if (not checkOpen()) {
    return NPOS;
  }

  return pos_;
}

bool
StringSource::terminal(i32*) {
  checkOpen();
  return false;
}

} // namespace rocket::nio

// Variables ------------------------------------------------------------------------------------------------

namespace {

FileSource fileSourceIn = FileSource(stdin);
FileSink fileSinkOut = FileSink(stdout);
FileSink fileSinkErr = FileSink(stderr);

} // namespace

namespace rocket::nio {

Source& in = fileSourceIn;
Sink& out = fileSinkOut;
Sink& err = fileSinkErr;

} // namespace rocket::nio

// EOF
