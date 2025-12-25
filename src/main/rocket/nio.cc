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
  write("\n");
  return flush();
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
  string modes = params.append ? "a" : "w";
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
    if (int result = std::fwrite(data.data(), 1, data.size(), file_); result < data.size()) {
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

FileSink stderr = FileSink(::stderr, FileSink::Params { .closeOnDestroy=false });

FileSink stdout = FileSink(::stdout, FileSink::Params { .closeOnDestroy=false });

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
