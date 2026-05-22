/*
 * nio.cc
 */

#include "nio.h"

#include "rocket/assert.h"
#include "rocket/io/io.h"
#include "rocket/unicode/unicode.h"

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

#include <iostream>

using namespace rocket;
using namespace rocket::nio;
using namespace std;

using boost::safe_numerics::safe;

/* Logging --------------------------------------------------------------------------------------------------

Because the logging framework utilizes #rocket::nio, we can't use it to log #rocket::nio itself. So we need
to make up a quick and dirty logging facility here.

---------------------------------------------------------------------------------------------------------- */

#ifdef ROCKET_NIO_LOG
#define LOG(args) cout << "# " << ROCKET_SRC_FILE << ':' << __LINE__ << ' ' << __FUNCTION__ << ": " << args << endl;
#else
#define LOG(args)
#endif

namespace rocket::nio {

// #Sink ----------------------------------------------------------------------------------------------------

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

// #BufferedSink --------------------------------------------------------------------------------------------

BufferedSink::BufferedSink(Sink& underlying, u64 size) :
    underlying_(underlying),
    size_(size) {
  ROCKET_CHECK(size, size >= MIN_BUFFER_SIZE);
  status_ = underlying.status();
  if (not bad()) {
    buf_ = make_unique<u8[]>(size); // NOLINT
  }
}

BufferedSink::~BufferedSink() {
  close(); // NOLINT
}

bool
BufferedSink::close() {
  if (bad()) {
    return false;
  }

  flush(); // NOLINT

  status_.bad = true;
  size_ = 0;
  buf_ = nullptr;
  pos_ = 0;
  return underlying_.close();
}

bool
BufferedSink::flush() {
  if (bad()) {
    return false;
  }

  flushBuffer();
  return underlying_.flush();
}

void
BufferedSink::flushBuffer() {
  ROCKET_ASSERT(buf_);

  if (pos_ > 0) {
    LOG("Flushing " << pos_ << " bytes from buffer to underlying")
    underlying_.write(span<const u8>(&buf_[0], pos_));
    pos_ = 0;
  }
}

u64
BufferedSink::write(span<const u8> in) {
  if (bad()) {
    return 0;
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
  status_.bad = false;
  if (file == stdout || file == stderr) {
    config_.closeOnDestroy = false;
  }
}

FileSink::FileSink(const string& path, const Config& config) :
  config_(config) {
  const char* modes = config.append ? "ab" : "wb";
#ifdef ROCKET_OS_WINDOWS
  [[maybe_unused]] const auto result = fopen_s(&file_, path.c_str(), modes);
  LOG("fopen_s=" << result << ", file=" << file_ << ", ferror=" << (file_ ? ferror(file_) : -1));
#else
  file_ = fopen(path.c_str(), modes); // NOLINT(*-owning-memory)
  LOG("fopen=" << file_ << ", ferror=" << (file_ ? ferror(file_) : -1));
#endif

  if (file_ != nullptr) {
    status_.bad = false;
  }
}

FileSink::~FileSink() {
  if (config_.closeOnDestroy) {
    close(); // NOLINT
  }
}

bool
FileSink::close()
{
  if (bad()) {
    return false;
  }

  flush(); // NOLINT

  const auto result = fclose(file_); // NOLINT(*-owning-memory)
  // #file_ is invalid now, so we don't call #ferror on it
  LOG("fclose=" << result);
  status_.bad = true;
  file_ = nullptr;
  return result == 0;
}

bool
FileSink::flush() {
  if (bad()) {
    return false;
  }

  const auto result = fflush(file_);
  LOG("fflush=" << result << ", ferror=" << ferror(file_));
  return result == 0;
}

i32
FileSink::handle() const {
  if (bad()) {
    return -1;
  }

  ROCKET_ASSERT(file_ != nullptr);
  return ROCKET_FILENO(file_);
}

u64
FileSink::write(span<const u8> in) {
  if (bad()) {
    return 0;
  }

  const auto result = fwrite(in.data(), 1, in.size(), file_);
  auto error = ferror(file_);
  LOG("fwrite=" << result << ", in.size=" << in.size() << ", ferror=" << error);
  ROCKET_ASSERT(result == in.size() || error != 0);
  return result;
}

// #NullSink ------------------------------------------------------------------------------------------------

NullSink::NullSink() {
  status_.bad = false;
  status_.eof = true;
}

// #SpanSink ------------------------------------------------------------------------------------------------

SpanSink::SpanSink(span<char> out) :
  out_(out) {
  status_.bad = false;
  status_.eof = out.empty();
}

bool
SpanSink::close() {
  if (bad()) {
    return false;
  }

  status_.bad = true;
  return true;
}

u64
SpanSink::write(span<const u8> in) {
  const u64 available = out_.size() - pos_;
  const u64 ret = min(available, in.size());
  if (ret > 0) {
    memcpy(&out_[pos_], in.data(), ret);
    pos_ += ret;
  }
  if (ret < in.size()) {
    status_.eof = true;
  }
  return ret;
}

// #StreamSink ----------------------------------------------------------------------------------------------

StreamSink::StreamSink(ostream& os) :
    os_(os) {
  status_.bad = os.bad();
  status_.eof = os.eof();
}

StreamSink::~StreamSink() {
  close(); // NOLINT
}

bool
StreamSink::close() {
  if (bad()) {
    return false;
  }

  flush(); // NOLINT

  status_.bad = true;
  // A #std::ostream can't close, it can only be destroyed
  return true;
}

bool
StreamSink::flush() {
  if (bad()) {
    return false;
  }

  os_.flush();
  LOG("bad=" << os_.bad() << ", fail=" << os_.fail() << ", eof=" << os_.eof());
  status_.bad = os_.bad();
  status_.eof = os_.eof();
  return not bad();
}

i32
StreamSink::handle() const {
  if (bad()) {
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
StreamSink::write(span<const u8> in) {
  if (bad()) {
    return 0;
  }

  const auto result = os_.rdbuf()->sputn(reinterpret_cast<const char*>(in.data()), safe<streamsize>(in.size()));
  LOG("rdbuf()->sputn=" << result << ", bad=" << os_.bad() << ", fail=" << os_.fail() << ", eof=" << os_.eof());
  status_.bad = os_.bad();
  status_.eof = os_.eof();
  return safe<u64>(result);
}

// #StringSink ----------------------------------------------------------------------------------------------

StringSink::StringSink() {
  status_.bad = false;
}

StringSink::StringSink(string& ref) :
  ptr_(&ref) {
  status_.bad = false;
}

bool
StringSink::close() {
  if (bad()) {
    return false;
  }

  status_.bad = true;
  return true;
}

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

// #Source --------------------------------------------------------------------------------------------------

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

optional<unicode::CodePoint>
Source::readCodePoint() {
  if (bad()) {
    return {};
  }

  // Read first byte
  char c; // NOLINT
  if (read(c) != 1) {
    return {};
  }
  string s;
  s.push_back(c);

  // Read subsequent bytes
  auto len = unicode::utf8::lengthFromByte(c);
  for (u64 i = 0; i < len - 1; ++i) {
    if (read(c) != 1) {
      return {};
    }
    s.push_back(c);
  }

  // Decode the code point
  try {
    u64 pos = 0;
    auto ret = unicode::nextCodePoint(s, pos);
    ROCKET_EXPECT(pos == s.size(), "Invalid UTF-8 sequence");
    return ret;
  } catch (const exception&) {
    return {};
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

// #ContiguousSource ----------------------------------------------------------------------------------------

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

  seek(safe<i64>(pos + 1), SeekMode::cur);
  str = str.substr(0, pos);
  if (not str.empty() && str.back() == '\r') {
    str = str.substr(0, str.size() - 1);
  }
  string ret(str);
  return ret;
#endif
}

optional<unicode::CodePoint>
ContiguousSource::readCodePoint() {
#ifdef ROCKET_NIO_NO_CONTIGUOUS_SOURCE
  return Source::readCodePoint();
#else
  if (bad()) {
    return {};
  }

  const auto str = this->str();
  try {
    u64 pos = 0;
    auto ret = unicode::nextCodePoint(str, pos);
    seek(safe<i64>(pos), SeekMode::cur);
    return ret;
  } catch (const exception&) {
    return {};
  }
#endif
}

// #BufferedSource ------------------------------------------------------------------------------------------

BufferedSource::BufferedSource(Source& underlying, u64 size) :
  underlying_(underlying),
  size_(size) {
  ROCKET_CHECK(size, size >= MIN_BUFFER_SIZE);
  status_ = underlying.status();
  if (not bad()) {
    buf_ = make_unique<u8[]>(size); // NOLINT
    bufPos_ = underlying.tell();
  }
}

BufferedSource::~BufferedSource() {
  close(); // NOLINT
}

bool
BufferedSource::close() {
  if (bad()) {
    return false;
  }

  status_.bad = true;
  size_ = 0;
  buf_ = nullptr;
  bufPos_ = -1;
  pos_ = 0;
  end_ = 0;
  return underlying_.close();
}

istream&
BufferedSource::istream() {
  if (bad()) {
    throw InvalidState("`BufferedSource` is bad");
  }

  auto& ret = underlying_.istream();
  ret.seekg(safe<istream::off_type>(tell()), ios::beg);
  return ret;
}

u64
BufferedSource::read(span<u8> out) {
  if (bad()) {
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

  status_.eof = ret < out.size();

  ROCKET_ASSERT(ret <= out.size());
  return ret;
}

bool
BufferedSource::seek(i64 offset, SeekMode mode) { // NOLINT
  if (bad()) {
    return false;
  }

  status_.eof = false;

  // Get the old position so we can restore it later
  const u64 oldTell = underlying_.tell();
  if (oldTell == NPOS) {
    LOG("Getting old position failed; invalidating buffer");
    bufPos_ = NPOS;
    pos_ = end_ = 0;
    return false;
  }

  // Do the job
  const auto ret = underlying_.seek(offset, mode);

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
  if (bad()) {
    return NPOS;
  }

  if (bufPos_ == NPOS) {
    return NPOS;
  }
  return bufPos_ + pos_;
}

// #FileSource ----------------------------------------------------------------------------------------------

FileSource::FileSource(FILE* file, const Config& config) :
  file_(file),
  config_(config) {
  ROCKET_CHECK(file, file != nullptr);
  status_.bad = false;
  if (file == stdin) {
    config_.closeOnDestroy = false;
  }
}

FileSource::FileSource(const string& path, const Config& config) :
  config_(config) {
#ifdef ROCKET_OS_WINDOWS
  [[maybe_unused]] const auto result = fopen_s(&file_, path.c_str(), "rb");
  LOG("fopen_s=" << result << ", file=" << file_ << ", ferror=" << (file_ ? ferror(file_) : -1));
#else
  file_ = fopen(path.c_str(), "rb"); // NOLINT
  LOG("fopen=" << file_ << ", ferror=" << (file_ ? ferror(file_) : -1));
#endif

  if (file_ != nullptr) {
    status_.bad = false;
  }
}

FileSource::~FileSource() {
  if (config_.closeOnDestroy) {
    close(); // NOLINT
  }
}

bool
FileSource::close()
{
  if (bad()) {
    return false;
  }

  const auto result = fclose(file_); // NOLINT(*-owning-memory)
  LOG("fclose=" << result);
  status_.bad = true;
  file_ = nullptr;
  return result == 0;
}

i32
FileSource::handle() const {
  if (bad()) {
    return -1;
  }

  ROCKET_ASSERT(file_ != nullptr);
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
  istream_->seekg(safe<istream::off_type>(tell()), ios::beg);
  return *istream_;
}

u64
FileSource::read(span<u8> out) {
  if (bad()) {
    return 0;
  }

  const auto result = fread(out.data(), 1, out.size(), file_);
  LOG("fread=" << result << ", out.size=" << out.size() << ", ferror=" << ferror(file_));
  status_.eof = result < out.size();
  return result;
}

bool
FileSource::seek(i64 offset, SeekMode mode) { // NOLINT
  if (bad()) {
    return false;
  }

  status_.eof = false;

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

  auto result = fseek(file_, safe<std_long>(offset), origin);
  LOG("fseek=" << result << ", ferror=" << ferror(file_));
  return result == 0;
}

u64
FileSource::tell() {
  if (bad()) {
    return NPOS;
  }

  auto result = ftell(file_);
  LOG("ftell=" << result << ", ferror=" << ferror(file_));
  if (result == -1) {
    return NPOS;
  }
  ROCKET_ASSERT(result >= 0);
  return safe<u64>(result);
}

// #NullSource ----------------------------------------------------------------------------------------------

NullSource::NullSource() {
  status_.bad = false;
  status_.eof = true;
}

istream&
NullSource::istream() {
  if (bad()) {
    throw InvalidState("`NullSource` is bad");
  }

  if (istream_ == nullptr) {
    istream_ = make_unique<ispanstream>(span<const char>());
  }
  return *istream_;
}

// #SpanSource ----------------------------------------------------------------------------------------------

SpanSource::SpanSource(span<const u8> in) :
  in_(in) {
  status_.bad = false;
  status_.eof = in.empty();
}

bool
SpanSource::close() {
  if (bad()) {
    return false;
  }

  status_.bad = true;
  return true;
}

istream&
SpanSource::istream() {
  if (bad()) {
    throw InvalidState("`SpanSource` is bad");
  }

  if (istream_ == nullptr) {
    const span<const char> chars(reinterpret_cast<const char*>(in_.data()), in_.size());
    istream_ = make_unique<ispanstream>(chars);
  }
  istream_->seekg(safe<istream::off_type>(tell()), ios::beg);
  return *istream_;
}

u64
SpanSource::read(span<u8> out) {
  if (bad()) {
    return 0;
  }

  const u64 ret = min(out.size(), in_.size() - static_cast<u64>(pos_));
  if (ret > 0) {
    memcpy(out.data(), in_.data() + static_cast<u64>(pos_), ret);
    pos_ += ret;
  }
  status_.eof = ret < out.size();
  return ret;
}

bool
SpanSource::seek(i64 offset, SeekMode mode) { // NOLINT
  if (bad()) {
    return false;
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
  status_.eof = pos_ == in_.size();
  return true;
}

// #StreamSource --------------------------------------------------------------------------------------------

StreamSource::StreamSource(std::istream& is) :
  is_(is) {
  status_.bad = is.bad();
  status_.eof = is.eof();
}

StreamSource::~StreamSource() {
  close(); // NOLINT
}

bool
StreamSource::close() {
  if (bad()) {
    return false;
  }

  status_.bad = true;
  // A #std::istream can't close, it can only be destroyed
  return true;
}

i32
StreamSource::handle() const {
  if (bad()) {
    return -1;
  }

  if (&is_ == &cin) {
    return STDIN_FILENO;
  }
  return -1;
}

u64
StreamSource::read(span<u8> out) {
  if (bad()) {
    return 0;
  }

  // If less bytes than `out.size()` are read, `bad`, `fail`, and `eof` all remain `false`
  is_.read(reinterpret_cast<char*>(out.data()), safe<streamsize>(out.size()));
  auto count = is_.gcount();
  // #std::istream::readsome didn't work in Windows with a #std::ifstream
  // u64 ret = is_.readsome(out.data(), out.size());
  LOG("count=" << count << ", out.size=" << out.size() << ", bad=" << is_.bad() << ", fail=" << is_.fail() << ", eof=" << is_.eof());
  status_.bad = is_.bad();
  status_.eof = is_.eof();
  return safe<u64>(count);
}

bool
StreamSource::seek(i64 offset, SeekMode mode) { // NOLINT
  if (bad()) {
    return false;
  }

  status_.eof = false;

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
  status_.bad = is_.bad();
  return not bad();
}

u64
StreamSource::tell() {
  if (bad()) {
    return NPOS;
  }

  // Use the impl. from #rocket::io
  auto result = io::tellg(is_);
  LOG("tellg=" << result << ", bad=" << is_.bad() << ", fail=" << is_.fail() << ", eof=" << is_.eof());
  status_.bad = is_.bad();

  if (result < 0) {
    return NPOS;
  }
  return static_cast<u64>(result); // #boost::safe_numerics::safe doesn't work with #std::ios::pos_type
}

// #StringSource --------------------------------------------------------------------------------------------

StringSource::StringSource(string_view in) :
  in_(in) {
  status_.bad = false;
  status_.eof = in.empty();
}

bool
StringSource::close() {
  if (bad()) {
    return false;
  }

  status_.bad = true;
  return true;
}

istream&
StringSource::istream() {
  if (bad()) {
    throw InvalidState("`StringSource` is bad");
  }

  if (istream_ == nullptr) {
    const span<const char> chars(in_.data(), in_.size());
    istream_ = make_unique<ispanstream>(chars);
  }
  istream_->seekg(safe<istream::off_type>(tell()), ios::beg);
  return *istream_;
}

u64
StringSource::read(span<u8> out) {
  if (bad()) {
    return 0;
  }

  const u64 ret = min(out.size(), in_.size() - static_cast<u64>(pos_));
  if (ret > 0) {
    memcpy(out.data(), in_.data() + static_cast<u64>(pos_), ret);
    pos_ += ret;
  }
  status_.eof = ret < out.size();
  return ret;
}

bool
StringSource::seek(i64 offset, SeekMode mode) { // NOLINT
  if (bad()) {
    return false;
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
  status_.eof = pos_ == in_.size();
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
