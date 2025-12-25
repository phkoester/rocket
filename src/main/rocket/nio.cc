/*
 * nio.cc
 */

#include "nio.h"

#include "assert.h"

#include <cstdio>
#include <unistd.h>

using namespace std;

namespace rocket::nio {

// `Sink` ---------------------------------------------------------------------------------------------------

int
Sink::writeln(std::string_view data) {
  write(data);
  write('\n');
  return flush();
}

// `BufferedSink` -------------------------------------------------------------------------------------------

BufferedSink::BufferedSink(Sink& sink, size_t size) :
    sink_(sink),
    size_(size) {
  ROCKET_CHECK(size, size >= MIN_BUFFER_SIZE);
  buf_ = make_unique<char[]>(size);
}

int
BufferedSink::close() {
  int ret = sink_.close();
  buf_ = nullptr;
  pos_ = 0;
  return ret;
}

int
BufferedSink::flush() {
  if (buf_ && pos_ > 0) {
    flushBuffer();
  }
  return sink_.flush();
}

void
BufferedSink::flushBuffer() {
  sink_.write(string_view(&buf_[0], pos_));
  pos_ = 0;
}

int
BufferedSink::write(string_view data) {
  if (not good()) {
    return sink_.write(data);
  }
  ROCKET_ASSERT(buf_);

  auto rest = data;
  while (not rest.empty()) {
    size_t free = size_ - pos_;
    if (rest.size() <= free) {
      memcpy(&buf_[pos_], rest.data(), rest.size());
      pos_ += rest.size();
      break;
    }
    memcpy(&buf_[pos_], rest.data(), free);
    pos_ += free;
    rest = rest.substr(free);
    flushBuffer();
  }
  return 0;
}

// `FileSink` -----------------------------------------------------------------------------------------------

FileSink::FileSink(FILE* file, const Params& params) :
    file_(file),
    params_(params) {
  ROCKET_CHECK(file, file != nullptr);
}

FileSink::FileSink(const string& path, const Params& params) :
    file_(nullptr),
    params_(params) {
  string modes = params.append ? "ab" : "wb"; // `b` is for non-Linux only
  file_ = std::fopen(path.c_str(), modes.c_str());

  if (file_ == nullptr) {
    error_ = errno;
    if (error_ == 0) {
      error_ = ENOENT;
    }
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
  if (open_) {
    open_ = false;
    if (int result = std::fclose(file_); result != 0) {
      error_ = result;
      if (error_ == 0) {
        error_ = errno;
      }
      if (error_ == 0) {
        error_ = EIO;
      }
    }
  } else if (error_ == 0) {
    error_ = EBADF;
  }
  return error_;
}

int
FileSink::flush() {
  if (open_) {
    if (int result = std::fflush(file_); result != 0) {
      error_ = result;
    }
  } else if (error_ == 0) {
    error_ = EBADF;
  }
  return error_;
}

int
FileSink::write(string_view data) {
  if (open_) {
    if (size_t result = std::fwrite(data.data(), 1, data.size(), file_); result < data.size()) {
      error_ = errno;
      if (error_ == 0) {
        error_ = EIO;
      }
    }
  } else if (error_ == 0) {
    error_ = EBADF;
  }
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
    open_ = false;
    os_.flush();
    if (not os_) {
      error_ = EIO;
    }
  } else if (error_ == 0) {
    error_ = EBADF;
  }
  return error_;
}

int
StreamSink::flush() {
  if (open_) {
    os_.flush();
    if (not os_) {
      error_ = EIO;
    }
  } else if (error_ == 0) {
    error_ = EBADF;
  }
  return error_;
}

int
StreamSink::write(string_view data) {
  if (open_) {
    os_.write(data.data(), data.size());
    if (not os_) {
      error_ = EIO;
    }
  } else if (error_ == 0) {
    error_ = EBADF;
  }
  return error_;
}

// `StringSink` ---------------------------------------------------------------------------------------------

int
StringSink::close() {
  if (open_) {
    open_ = false;
  } else if (error_ == 0) {
    error_ = EBADF;
  }
  return error_;
}

int
StringSink::flush() {
  if (not open_ && error_ == 0) {
    error_ = EBADF;
  }
  return error_;
}

int
StringSink::write(string_view data) {
  if (open_) {
    buf_ += data;
  } else if (error_ == 0) {
    error_ = EBADF;
  }
  return error_;
}

// Variables ------------------------------------------------------------------------------------------------

namespace {

FileSink fileSinkStdout = FileSink(::stdout, FileSink::Params { .closeOnDestroy=false });
FileSink fileSinkStderr = FileSink(::stderr, FileSink::Params { .closeOnDestroy=false });

} // namespace

Sink& stdout = fileSinkStdout;
Sink& stderr = fileSinkStderr;

// Functions ------------------------------------------------------------------------------------------------

int
fd(const nio::Sink& sink) {
  const auto* fileSink = dynamic_cast<const nio::FileSink*>(&sink);
  if (fileSink) {
    if (fileSink->stdout()) {
      return STDOUT_FILENO;
    } else if (fileSink->stderr()) {
      return STDERR_FILENO;
    }
  }
  return -1;
}

bool
isatty(const nio::Sink& sink) {
  return ::isatty(fd(sink));
}

} // namespace rocket::nio

// EOF
