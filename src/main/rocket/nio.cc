/*
 * nio.cc
 */

#include "nio.h"

#include "assert.h"

#include <cstdio>
#include <iostream>
#include <unistd.h>

using namespace rocket;
using namespace rocket::nio;
using namespace std;

/* Logging --------------------------------------------------------------------------------------------------

Because the logging framework utilitizes `nio`, we can't use it to log `nio` itself. So we need to make up a
tiny logging facility here.

---------------------------------------------------------------------------------------------------------- */

#define NIO_LOG // Use this to activate logging

#ifdef NIO_LOG
#define LOG(name, args) cout << "# " << #name << ": " << args << endl;
#else
#define LOG(name, args)
#endif

// Local functions ------------------------------------------------------------------------------------------

namespace {

size_t
seekPos(size_t current, size_t size, long pos, SeekMode mode) {
  size_t ret;

  switch (mode) {
  case SeekMode::beg:
    ret = pos;
    break;
  case SeekMode::cur:
    ret = current + pos;
    break;
  case SeekMode::end:
     ret = size - pos;
     break;
  default:
    ROCKET_FAIL_UNREACHABLE_CODE();
  }

  return min(ret, size);
}

} // namespace

namespace rocket::nio {

// `Sink` ---------------------------------------------------------------------------------------------------

bool
Sink::checkOpen() {
  if (not open_) {
    if (error_ == 0) {
      error_ = EBADF;
    }
    return false;
  }
  return true;
}

size_t
Sink::writeln(std::string_view in) {
  if (not checkOpen()) {
    return error_;
  }

  size_t ret = write(in);
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
    LOG(BufferedSink::flushBuffer, "Flushing " << pos_ << " bytes from buffer to underlying sink")
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

  int result = std::fclose(file_);
  LOG(FileSink::ctor, "fclose=" << result << ", ferror=" << ferror(file_));
  if (result != 0) {
    error_ = ferror(file_);
  }
  open_ = false;
  file_ = nullptr;
  return error_;
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

  int result = std::fflush(file_);
  LOG(FileSink::flush, "fflush=" << result << ", ferror=" << ferror(file_));
  if (result != 0) {
    error_ = ferror(file_);
  }
  return error_;
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
  return error_;
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
  return error_;
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
  return error_;
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
  return error_;
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
    return error_;
  }

  if (out) {
    out->append(in);
  } else {
    managed_.append(in);
  }
  return in.size();
}

// `Source` -------------------------------------------------------------------------------------------------

bool
Source::checkOpen() {
  if (not open_ && error_ == 0) {
    error_ = EBADF;
  }
  return open_;
}

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
  pos_ = 0;
  end_ = 0;
  return underlying_.close();
}

size_t
BufferedSource::read(span<char> out) {
  if (not checkOpen()) {
    return error();
  }

  // If needed, initialize buffer

  if (end_ == 0) {
    ROCKET_ASSERT(pos_ == 0);
    end_ = underlying_.read(span<char>(&buf_[0], size_));
    LOG(BufferedSource::read, "Initialized buffer with " << end_ << " bytes from underlying source");
    if (end_ == 0) {
      return 0;
    }
  }

  // Loop while there is data to read

  size_t ret = 0;

  auto rest = out;
  while (true) {
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
    if (end_ < size_ || rest.empty()) {
      break;
    }

    // Fill the buffer

    pos_ = 0;
    end_ = underlying_.read(span<char>(&buf_[0], size_));
    LOG(BufferedSource::read, "Filled buffer with " << end_ << " bytes from underlying source");
    if (end_ == 0) {
      break;
    }
  }

  ROCKET_ASSERT(ret <= out.size());
  return ret;
}

int
BufferedSource::seek(long pos, SeekMode mode) {
  ROCKET_FAIL_NOT_IMPLEMENTED; // XXX
}

// `FileSource` ---------------------------------------------------------------------------------------------

FileSource::FileSource(FILE* file, const Params& params) :
    file_(file),
    params_(params) {
  ROCKET_CHECK(file, file != nullptr);
  if (int fd = this->fd(); fd == STDIN_FILENO || fd == STDERR_FILENO) {
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

  int result = std::fclose(file_);
  LOG(FileSource::close, "fclose=" << result);
  error_ = result;
  open_ = false;
  file_ = nullptr;
  return error_;
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
FileSource::seek(long pos, SeekMode mode) {
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

  size_t result = std::fseek(file_, pos, origin);
  LOG(FileSource::seek, "fseek=" << result << ", ferror=" << ferror(file_));
  if (result != 0) {
    error_ = ferror(file_);
  }
  return error_;
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
  return error_;
}

size_t
NullSource::read(span<char> out) {
  checkOpen();
  return error_;
}

int
NullSource::seek(long pos, SeekMode mode) {
  checkOpen();
  return error_;
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
  return error_;
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
    return error_;
  }

  // If less bytes than `out.size()` are read, `bad`, `fail`, and `eof` all remain `false``
  size_t ret = is_.readsome(out.data(), out.size());
  LOG(StreamSource::read, "readsome=" << ret << ", out.size=" << out.size() << ", bad=" << is_.bad() << ", fail=" << is_.fail() << ", eof=" << is_.eof());
  error_ = is_.rdstate();
  return ret;
}

int
StreamSource::seek(long pos, SeekMode mode) {
  if (not checkOpen()) {
    return error_;
  }

  ios::seekdir dir;
  switch (mode) {
  case SeekMode::beg:
    dir = std::ios::beg;
    break;
  case SeekMode::cur:
    dir = std::ios::cur;
    break;
  case SeekMode::end:
    dir = std::ios::end;
    break;
  default:
    ROCKET_FAIL_UNREACHABLE_CODE();
  }

  is_.seekg(pos, dir);
  error_ = is_.rdstate();
  return error_;
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
  return error_;
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
StringSource::seek(long pos, SeekMode mode) {
  if (not checkOpen()) {
    return error_;
  }

  pos_ = seekPos(pos_, in_.size(), pos, mode);
  return error_;
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
