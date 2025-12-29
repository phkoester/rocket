/*
 * nio.cc
 */

#include "nio.h"

#include "assert.h"
#include "log.h"

#include <cstdio>
#include <iostream>
#include <unistd.h>

using namespace rocket::nio;
using namespace std;

ROCKET_LOG_DEFINE(rocket_nio);

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

int
Sink::writeln(std::string_view data) {
  write(data);
  write('\n');
  return flush();
}

// `BufferedSink` -------------------------------------------------------------------------------------------

BufferedSink::BufferedSink(Sink& underlying, size_t size) :
    underlying_(underlying),
    size_(size) {
  ROCKET_CHECK(size, size >= MIN_BUFFER_SIZE);
  buf_ = make_unique<char[]>(size);
}

int
BufferedSink::close() {
  if (not checkOpen()) {
    return error();
  }

  flush();
  buf_ = nullptr;
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
  ROCKET_EXPECT(buf_);

  if (buf_ && pos_ > 0) {
    underlying_.write(string_view(&buf_[0], pos_));
    pos_ = 0;
  }
}

int
BufferedSink::write(string_view data) {
  if (not checkOpen()) {
    return error();
  }

  // Loop while there is data to write
  auto rest = data;
  while (not rest.empty()) {
    size_t free = size_ - pos_;
    if (rest.size() <= free) {
      // Store the rest in the buffer, exit loop
      memcpy(&buf_[pos_], rest.data(), rest.size());
      pos_ += rest.size();
      break;
    }
    // Fill and flush the buffer, continue in loop
    memcpy(&buf_[pos_], rest.data(), free);
    pos_ += free;
    rest = rest.substr(free);
    flushBuffer();
  }

  return error();
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
  ROCKET_LOG(rocket_nio);

  const char* modes = params.append ? "ab" : "wb"; // `b` is for non-Linux only
  file_ = std::fopen(path.c_str(), modes);
  ROCKET_LOG_DEBUG("fopen={}, errno={}, ferror={}", fmt::ptr(file_), errno, file_ ? ferror(file_) : -1);

  if (file_ == nullptr) {
    error_ = ENOENT;
    open_ = false;
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
  ROCKET_LOG(rocket_nio);

  if (not checkOpen()) {
    return error_;
  }

  flush();
  open_ = false;
  int result;
  if ((result = std::fclose(file_)) != 0) {
    error_ = result;
  }
  ROCKET_LOG_DEBUG("fclose={}, errno={}", result, errno);
  file_ = nullptr;
  return error_;
}

int
FileSink::fd() const {
  return file_ ? fileno(file_) : -1;
}

int
FileSink::flush() {
  ROCKET_LOG(rocket_nio);

  if (not checkOpen()) {
    return error_;
  }

  int result;
  if ((result = std::fflush(file_)) != 0) {
    error_ = result;
  }
  ROCKET_LOG_DEBUG("fflush={}, errno={}, ferror={}", result, errno, ferror(file_));
  return error_;
}

int
FileSink::write(string_view data) {
  ROCKET_LOG(rocket_nio);

  if (not checkOpen()) {
    return error_;
  }

  size_t result;
  if ((result = std::fwrite(data.data(), 1, data.size(), file_)) < data.size()) {
    error_ = errno;
    if (error_ == 0) {
      error_ = EIO;
    }
  }
  ROCKET_LOG_DEBUG("fwrite={}, data.size={}, errno={}, ferror={}", result, data.size(), errno, ferror(file_));
  return error_;
}

// `NullSink` -----------------------------------------------------------------------------------------------

int
NullSink::close()
{
  if (open_) {
    open_ = false;
  } else if (error_ == 0) {
    error_ = EBADF;
  }
  return error_;
}

int
NullSink::flush() {
  if (not open_ && error_ == 0) {
    error_ = EBADF;
  }
  return error_;
}

int
NullSink::write(string_view data) {
  if (not open_ && error_ == 0) {
    error_ = EBADF;
  }
  return error_;
}

// `StreamSink` ---------------------------------------------------------------------------------------------

int
StreamSink::close() {
  if (open_) {
    flush();
    open_ = false;
    if (os_.fail()) {
      error_ = EIO;
    }
  } else if (error_ == 0) {
    error_ = EBADF;
  }
  return error_;
}

int
StreamSink::fd() const {
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
  if (os_.fail()) {
    error_ = EIO;
  }
  return error_;
}

int
StreamSink::write(string_view data) {
  if (not checkOpen()) {
    return error_;
  }

  os_.write(data.data(), data.size());
  if (os_.fail()) {
    error_ = EIO;
  }
  return error_;
}

// `StringSink` ---------------------------------------------------------------------------------------------

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

int
StringSink::write(string_view data) {
  if (not checkOpen()) {
    return error_;
  }

  if (out) {
    *out += data;
  } else {
    managed_ += data;
  }
  return error_;
}

// `Source` -------------------------------------------------------------------------------------------------

string
Source::read() {
  if (not checkOpen()) {
    return string();
  }

  auto buf = make_unique<char[]>(DEFAULT_BUFFER_SIZE);
  return ""; // XXX
}

bool
Source::checkOpen() {
  if (not open_) {
    if (error_ == 0) {
      error_ = EBADF;
    }
    return false;
  }
  return true;
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
  ROCKET_LOG(rocket_nio);

  file_ = std::fopen(path.c_str(), "fb");
  ROCKET_LOG_DEBUG("fopen={}, errno={}, ferror={}", fmt::ptr(file_), errno, file_ ? ferror(file_) : -1);

  if (file_ == nullptr) {
    error_ = errno;
    if (error_ == 0) {
      error_ = ENOENT;
    }
    open_ = false;
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
  ROCKET_LOG(rocket_nio);

  if (not checkOpen()) {
    return error_;
  }

  open_ = false;
  int result;
  if ((result = std::fclose(file_)) != 0) {
    error_ = result;
  }
  ROCKET_LOG_DEBUG("fclose={}, errno={}", result, errno);
  return error_;
}

int
FileSource::fd() const {
  return file_ ? fileno(file_) : -1;
}

size_t
FileSource::read(string_view out) {
  ROCKET_LOG(rocket_nio);

  if (not checkOpen()) {
    return error_;
  }

  if (size_t result = std::fread(&out, 1, out.size(), file_); result != 1) {
    error_ = errno;
    if (error_ == 0) {
      error_ = EIO;
    }
  }
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
