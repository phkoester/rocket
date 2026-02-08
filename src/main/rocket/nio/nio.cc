/*
 * nio.cc
 */

#include "nio.h"

#include "rocket/assert.h"
#include "rocket/io/io.h"

#include <iostream>

using namespace rocket;
using namespace rocket::nio;
using namespace std;

using boost::safe_numerics::safe;

/* Logging --------------------------------------------------------------------------------------------------

Because the logging framework utilizes #rocket::nio, we can't use it to log #rocket::nio itself. So we need
to make up a quick and dirty logging facility here.

---------------------------------------------------------------------------------------------------------- */

// #define ROCKET_LOG_NIO // Use this to activate logging

#ifdef ROCKET_LOG_NIO
#define LOG(args) cout << "# " << ROCKET_SRC_FILE << ':' << __LINE__ << ' ' << __FUNCTION__ << ": " << args << endl;
#else
#define LOG(args)
#endif

namespace rocket::nio {

// #Io ------------------------------------------------------------------------------------------------------

bool
Io::checkOpen() const{
  if (not open_) {
    if (error_ == 0) {
      error_ = EBADF;
    }
    return false;
  }
  return true;
}

// #Sink ----------------------------------------------------------------------------------------------------

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

// #BufferedSink --------------------------------------------------------------------------------------------

BufferedSink::BufferedSink(Sink& underlying, u64 size) :
    underlying_(underlying),
    size_(size) {
  ROCKET_CHECK(size, size >= MIN_BUFFER_SIZE);
  buf_ = make_unique<u8[]>(size); // NOLINT
}

BufferedSink::~BufferedSink() {
  close(); // NOLINT
}

i32
BufferedSink::close() {
  if (not checkOpen()) {
    return error(); // NOLINT
  }

  flush(); // NOLINT

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
    underlying_.write(std::span<const u8>(&buf_[0], pos_));
    pos_ = 0;
  }
}

u64
BufferedSink::write(std::span<const u8> in) {
  if (not checkOpen()) {
    return error();
  }

  // Loop while there is data to write

  auto rest = in;
  while (not rest.empty()) {
    // Find out if the buffer can fulfill the request

    const u64 available = size_ - pos_;
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
    rest = rest.subspan(available);
    flushBuffer();
  }

  return in.size();
}

// #FileSink ------------------------------------------------------------------------------------------------

FileSink::FileSink(FILE* file, const Config& config) :
    file_(file),
    config_(config) {
  ROCKET_CHECK(file, file != nullptr);
  if (file == stdout || file == stderr) {
    config_.closeOnDestroy = false;
  }
}

FileSink::FileSink(const string& path, const Config& config) :
    file_(nullptr),
    config_(config) {
  const char* modes = config.append ? "ab" : "wb"; // "b" is for non-Linux only
  file_ = std::fopen(path.c_str(), modes); // NOLINT(*-owning-memory)
  LOG("fopen=" << file_ << ", ferror=" << (file_ ? ferror(file_) : -1));

  if (file_ == nullptr) {
    error_ = ENOENT;
    open_ = false;
  } else {
    error_ = ferror(file_);
  }
}

FileSink::~FileSink() {
  if (config_.closeOnDestroy) {
    close(); // NOLINT
  }
}

i32
FileSink::close()
{
  if (not checkOpen()) {
    return error_;
  }

  flush(); // NOLINT

  const i32 ret = std::fclose(file_); // NOLINT(*-owning-memory)
  // #file_ is invalid now, so we don't call #ferror on it
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

  const i32 ret = std::fflush(file_);
  LOG("fflush=" << ret << ", ferror=" << ferror(file_));
  if (ret != 0) {
    error_ = ferror(file_);
  }
  return ret;
}

i32
FileSink::handle() const {
  if (not checkOpen()) {
    return -1;
  }

  return fileno(file_);
}

u64
FileSink::write(std::span<const u8> in) {
  if (not checkOpen()) {
    return error_;
  }

  const u64 ret = std::fwrite(in.data(), 1, in.size(), file_);
  LOG("fwrite=" << ret << ", in.size=" << in.size() << ", ferror=" << ferror(file_));
  error_ = ferror(file_);
  ROCKET_ASSERT(ret == in.size() || error_ != 0); // NOLINT
  return ret;
}

// #SpanSink ------------------------------------------------------------------------------------------------

u64
SpanSink::write(std::span<const u8> in) {
  const u64 available = out_.size() - pos_;
  const u64 ret = min(available, in.size());
  if (ret > 0) {
    memcpy(&out_[pos_], in.data(), ret);
    pos_ += ret;
  }
  return ret;
}

// #StreamSink ----------------------------------------------------------------------------------------------

StreamSink::~StreamSink() {
  close(); // NOLINT
}

i32
StreamSink::close() {
  if (not checkOpen()) {
    return error_;
  }

  flush(); // NOLINT

  open_ = false;
  // A #std::ostream can't close, it can only be destroyed
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

i32
StreamSink::handle() const {
  if (not checkOpen()) {
    return -1;
  }

  if (&os_ == &cout) {
    return STDOUT_FILENO;
  }
  if (&os_ == &cerr) {
    return STDERR_FILENO;
  }
  return -1;
}

u64
StreamSink::write(std::span<const u8> in) {
  if (not checkOpen()) {
    return error_;
  }

  const u64 ret = os_.rdbuf()->sputn(reinterpret_cast<const char*>(in.data()), safe<streamsize>(in.size()));
  if (ret != in.size()) {
    os_.setstate(ios_base::badbit);
  }
  LOG("rdbuf()->sputn=" << ret << ", bad=" << os_.bad() << ", fail=" << os_.fail() << ", eof=" << os_.eof());
  error_ = os_.rdstate();
  return ret;
}

// #StringSink ----------------------------------------------------------------------------------------------

u64
StringSink::write(std::span<const u8> in) {
  if (not checkOpen()) {
    return 0;
  }

  std::string_view view(reinterpret_cast<const char*>(in.data()), in.size());
  if (ptr_ != nullptr) {
    ptr_->append(view);
  } else {
    owned_.append(view);
  }
  return in.size();
}

// #Source --------------------------------------------------------------------------------------------------

vector<u8>
Source::readAll() {
  if (not checkOpen()) {
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
  if (not checkOpen()) {
    return {};
  }

  string ret;
  auto buf = make_unique<u8[]>(DEFAULT_BUFFER_SIZE); // NOLINT
  const span<u8> out(&buf[0], DEFAULT_BUFFER_SIZE);
  while (true) {
    const u64 n = read(out);
    if (n > 0) {
      string_view view(reinterpret_cast<const char*>(out.data()), n);
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
  if (not checkOpen()) {
    return {};
  }

  string ret;

  bool lf = false;
  while (true) {
    char c = '\0';
    const u64 result = read(c);
    if (result == 0) {
      break;
    }
    if (c == '\n') {
      lf = true;
      break;
    }
    ret.push_back(c);
  }

  // Remove trailing '\r' if it precedes the '\n'
  if (lf && not ret.empty() && *ret.rbegin() == '\r') {
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
  bool lf = false;

  while (it != out.end()) {
    char c = '\0';
    const u64 result = read(c);
    if (result == 0) {
      break;
    }
    if (c == '\n') {
      lf = true;
      break;
    }
    *(it++) = c;
  }

  // Remove trailing '\r' if it precedes the '\n'
  u64 ret = it - out.begin();
  if (lf && ret > 0 && *(it - 1) == '\r') {
    --ret;
  }
  return ret;
}

// #BufferedSource ------------------------------------------------------------------------------------------

BufferedSource::BufferedSource(Source& underlying, u64 size) :
    underlying_(underlying),
    size_(size) {
  ROCKET_CHECK(size, size >= MIN_BUFFER_SIZE);
  buf_ = make_unique<u8[]>(size); // NOLINT
  bufPos_ = underlying.tell();
}

BufferedSource::~BufferedSource() {
  close(); // NOLINT
}

i32
BufferedSource::close() {
  if (not checkOpen()) {
    return error(); // NOLINT
  }

  size_ = 0;
  buf_ = nullptr;
  bufPos_ = -1;
  pos_ = 0;
  end_ = 0;
  return underlying_.close();
}

u64
BufferedSource::read(span<u8> out) {
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
      end_ = underlying_.read(span<u8>(&buf_[0], size_));
      LOG("Initialized buffer with " << end_ << " bytes from underlying; bufPos=" << bufPos_ << ", pos=" << pos_ << ", end=" << end_);
      if (end_ == 0) {
        break;
      }
    }

    // Find out if the buffer can fulfill the request

    const u64 available = end_ - pos_;
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
BufferedSource::seek(i64 offset, SeekMode mode) { // NOLINT
  if (not checkOpen()) {
    return error_;
  }

  // Get the old position so we can restore it later
  const u64 oldTell = underlying_.tell();
  if (oldTell == NPOS) {
    LOG("Getting old position failed; invalidating buffer");
    bufPos_ = NPOS;
    pos_ = end_ = 0;
    return EIO;
  }

  // Do the job
  const i32 ret = underlying_.seek(offset, mode);

  // Get the new position se we can see if we have a buffer hit
  const u64 newTell = underlying_.tell();
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
  const u64 ourPos = newTell - bufPos_;
  if (ourPos <= end_) {
    // Yes, we do: Update our position and restore the underlying position
    LOG("Going from " << pos_ << " to " << ourPos);
    pos_ = ourPos;
    return underlying_.seek(safe<i64>(oldTell));
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

// #FileSource ----------------------------------------------------------------------------------------------

FileSource::FileSource(FILE* file, const Config& config) :
    file_(file),
    config_(config) {
  if (file == stdin) {
    config_.closeOnDestroy = false;
  }
}

FileSource::FileSource(const string& path, const Config& config) :
    file_(std::fopen(path.c_str(), "rb")), // "b" is for non-Linux only NOLINT(*-owning-memory)
    config_(config) {
  LOG("fopen=" << file_ << ", ferror=" << (file_ ? ferror(file_) : -1));

  if (file_ == nullptr) {
    error_ = ENOENT;
    open_ = false;
  } else {
    error_ = ferror(file_);
  }
}

FileSource::~FileSource() {
  if (config_.closeOnDestroy) {
    close(); // NOLINT
  }
}

i32
FileSource::close()
{
  if (not checkOpen()) {
    return error_;
  }

  const i32 ret = std::fclose(file_); // NOLINT(*-owning-memory)
  LOG("fclose=" << ret);
  error_ = ret;
  open_ = false;
  file_ = nullptr;
  return ret;
}

i32
FileSource::handle() const {
  if (not checkOpen()) {
    return -1;
  }

  return fileno(file_);
}

u64
FileSource::read(span<u8> out) {
  if (not checkOpen()) {
    return 0;
  }

  const u64 ret = std::fread(out.data(), 1, out.size(), file_);
  LOG("fread=" << ret << ", out.size=" << out.size() << ", ferror=" << ferror(file_));
  error_ = ferror(file_);
  return ret;
}

i32
FileSource::seek(i64 offset, SeekMode mode) { // NOLINT
  if (not checkOpen()) {
    return error_;
  }

  i32 origin = -1;
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

  auto result = std::fseek(file_, safe<std_long>(offset), origin);
  LOG("fseek=" << result << ", ferror=" << ferror(file_));
  if (result != 0) {
    error_ = ferror(file_);
  }
  return safe<i32>(result);
}

u64
FileSource::tell() {
  if (not checkOpen()) {
    return NPOS;
  }

  auto result = std::ftell(file_);
  LOG("ftell=" << result << ", ferror=" << ferror(file_));
  if (result == -1) {
    error_ = ferror(file_);
    return NPOS;
  }
  ROCKET_ASSERT(result >= 0);
  return safe<u64>(result);
}

// #StreamSource --------------------------------------------------------------------------------------------

StreamSource::~StreamSource() {
  close(); // NOLINT
}

i32
StreamSource::close() {
  if (not checkOpen()) {
    return error_;
  }

  open_ = false;
  // A #std::istream can't close, it can only be destroyed
  return 0;
}

i32
StreamSource::handle() const {
  if (not checkOpen()) {
    return -1;
  }

  if (&is_ == &cin) {
    return STDIN_FILENO;
  }
  return -1;
}

u64
StreamSource::read(span<u8> out) {
  if (not checkOpen()) {
    return 0;
  }

  // If less bytes than `out.size()` are read, `bad`, `fail`, and `eof` all remain `false`
  is_.read(reinterpret_cast<char*>(out.data()), safe<streamsize>(out.size()));
  auto count = is_.gcount();
  // #std::istream::readsome didn't work in Windows with a #std::ifstream
  // u64 ret = is_.readsome(out.data(), out.size());
  LOG("count=" << count << ", out.size=" << out.size() << ", bad=" << is_.bad() << ", fail=" << is_.fail() << ", eof=" << is_.eof());
  error_ = is_.rdstate();
  return safe<u64>(count);
}

i32
StreamSource::seek(i64 offset, SeekMode mode) { // NOLINT
  if (not checkOpen()) {
    return error_;
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

  LOG("before clear failbit, bad=" << is_.bad() << ", fail=" << is_.fail() << ", eof=" << is_.eof() << ", tellg=" << io::tellg(is_));
  // We need to clear the fail bit, otherwise the seek will fail
  is_.clear(is_.rdstate() & ~ios_base::failbit);
  LOG("after clear failbit, bad=" << is_.bad() << ", fail=" << is_.fail() << ", eof=" << is_.eof() << ", tellg=" << io::tellg(is_));
  is_.seekg(safe<istream::off_type>(offset), dir);
  LOG("after seekg, bad=" << is_.bad() << ", fail=" << is_.fail() << ", eof=" << is_.eof() << ", tellg=" << io::tellg(is_));
  error_ = is_.rdstate();
  return error_;
}

u64
StreamSource::tell() {
  if (not checkOpen()) {
    return NPOS;
  }

  // Use the impl. from #rocket::io
  auto result = io::tellg(is_);
  LOG("tellg=" << result << ", bad=" << is_.bad() << ", fail=" << is_.fail() << ", eof=" << is_.eof());
  error_ = is_.rdstate();

  if (result < 0) {
    return NPOS;
  }
  return static_cast<u64>(result); // #boost::safe_numerics::safe doesn't work with #std::ios::pos_type
}

// #StringSource --------------------------------------------------------------------------------------------

u64
StringSource::read(span<u8> out) {
  if (not checkOpen()) {
    return 0;
  }

  const u64 ret = min(out.size(), in_.size() - static_cast<u64>(pos_));
  if (ret > 0) {
    memcpy(out.data(), in_.data() + static_cast<u64>(pos_), ret);
    pos_ += ret;
  }
  return ret;
}

i32
StringSource::seek(i64 offset, SeekMode mode) { // NOLINT
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
