/*
 * nio.cc
 */

#include "nio.h"

#include "assert.h"

using namespace std;

namespace rocket::nio {

// `FileSink` -----------------------------------------------------------------------------------------------

FileSink::FileSink(FILE* file, bool closeOnDestroy) :
    file_(file),
    closeOnDestroy_(closeOnDestroy) {
  ROCKET_CHECK(file, file != nullptr);
}

FileSink::FileSink(const string& path, bool closeOnDestroy) :
    file_(fopen(path.c_str(), "wb")),
    closeOnDestroy_(closeOnDestroy) {}

FileSink::~FileSink() {
  if (closeOnDestroy_) {
    close();
  }
}

int
FileSink::close()
{
  if (open_) {
    open_ = false;
    if (int result = fclose(file_); result != 0 ) {
      error_ = result;
    }
  } else if (error_ == 0) {
    error_ = EBADF;
  }
  return error_;
}

int
FileSink::flush() {
  if (open_) {
    if (int result = fflush(file_); result != 0) {
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
    if (int result = fwrite(data.data(), 1, data.size(), file_); result != 0) {
      error_ = result;
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

void
Sink::vprint(fmt::locale_ref locale, fmt::string_view fmt, fmt::format_args args) {
  fmt::memory_buffer buf;
  fmt::detail::vformat_to(buf, fmt, args, locale);
  write({ buf.data(), buf.size() });
}

void
Sink::vprintln(fmt::locale_ref locale, fmt::string_view fmt, fmt::format_args args) {
  fmt::memory_buffer buf;
  fmt::detail::vformat_to(buf, fmt, args, locale);
  buf.push_back('\n');
  write({ buf.data(), buf.size() });
  flush();
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

} // namespace rocket::nio

// EOF
