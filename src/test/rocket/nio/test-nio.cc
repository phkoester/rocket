/*
 * test-nio.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/filesystem/filesystem.h"
#include "rocket/log/log.h"
#include "rocket/nio/nio.h"

#include <scn/istream.h>

#include <fstream>

using namespace rocket::nio;

namespace {

// Local functions ------------------------------------------------------------------------------------------

/// Returns "01234567890123...".
string
testString(u64 n, i32 add = 0) {
  string ret;
  for (u64 i = 0; i < n; ++i) {
    ret += static_cast<char>('0' + add);
    add = (add + 1) % 10;
  }
  return ret;
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

// `Sink` ...................................................................................................

TEST(nio, BufferedSink) {
  StringSink out;
  BufferedSink buffered(out, 64);
  EXPECT_EQ(buffered.size_, 64);

  buffered.Sink::write(string(testString(32)));
  EXPECT_EQ(buffered.pos_, 32);
  EXPECT_EQ(out.str(), "");
  buffered.Sink::write(testString(33, 2));
  EXPECT_EQ(buffered.pos_, 1);
  EXPECT_EQ(out.str(), testString(64));
  buffered.flush();
  EXPECT_EQ(out.str(), testString(65));
  EXPECT_EQ(buffered.pos_, 0);
}

TEST(nio, FileSinkDoesNotExist) {
  FileSink out("/does/not/exist");

  EXPECT_TRUE(out.bad());
  EXPECT_EQ(out.file_, nullptr);

  EXPECT_EQ(out.Sink::write("a"), 0);
  EXPECT_TRUE(out.bad());

  out.close();
  EXPECT_TRUE(out.bad());

  EXPECT_EQ(out.Sink::write("b"), 0);
  EXPECT_TRUE(out.bad());
}

TEST(nio, NullSink) {
  NullSink out;
  EXPECT_FALSE(out.bad());
  EXPECT_TRUE(out.eof());
  EXPECT_EQ(out.Sink::write("a"), 0);
}

TEST(nio, SpanSink) {
  string str = "---[abcd]---";
  const span<char> span(&str[4], 4);
  SpanSink out(span);
  EXPECT_EQ(out.Sink::write("ABCDEF"), 4); // Writing 6 chars, but only 4 fit
  EXPECT_EQ(str, "---[ABCD]---");
}

TEST(nio, StreamSink) {
  ostringstream os;
  StreamSink out(os);
  out.println("Hi {}", "there");
  EXPECT_EQ(os.str(), "Hi there\n");
}

TEST(nio, StringSink) {
  StringSink out1; // With managed string
  out1.println("Hi {}", "there");
  EXPECT_EQ(out1.str(), "Hi there\n");

  string buf;
  StringSink out2(buf); // With external string
  out2.println("Hi {}", "there");
  EXPECT_EQ(buf, "Hi there\n");
}

// `Source` .................................................................................................

TEST(nio, BufferedSource) {
  const string str65 = testString(65);
  StringSource in(str65);
  BufferedSource buffered(in, 64);
  EXPECT_EQ(buffered.size_, 64);
  EXPECT_EQ(buffered.pos_, 0);
  EXPECT_EQ(buffered.end_, 0);

  string str32(32, ' ');
  EXPECT_EQ(buffered.Source::read(str32), 32);
  EXPECT_EQ(string_view(str32), testString(32));
  EXPECT_EQ(buffered.pos_, 32);
  EXPECT_EQ(buffered.end_, 64);

  string str40 = string(40, ' ');
  EXPECT_EQ(buffered.Source::read(str40), 33);
  EXPECT_EQ(buffered.pos_, 1);
  EXPECT_EQ(buffered.end_, 1);
  const span<char> span33(str40.data(), 33);
  EXPECT_EQ(string_view(span33.data(), 33), testString(33, 2));
}

TEST(nio, BufferedSourceExactMatch) {
  const string str20 = testString(20);
  StringSource in(str20);
  BufferedSource buffered(in, 64);

  string out20(20, ' ');
  EXPECT_EQ(buffered.Source::read(out20), 20);
  EXPECT_EQ(buffered.pos_, 20);
  EXPECT_EQ(buffered.end_, 20);
}

TEST(nio, BufferedSourceSeek) {
  const string s128 = testString(128);
  StringSource in(s128);
  BufferedSource buffered(in, 64);
  EXPECT_EQ(buffered.tell(), 0);

  char c = '\0';
  EXPECT_EQ(buffered.Source::read(c), 1);
  EXPECT_EQ(c, '0');
  EXPECT_EQ(buffered.pos_, 1);
  EXPECT_EQ(buffered.end_, 64);
  EXPECT_EQ(buffered.tell(), 1);
  EXPECT_EQ(buffered.underlying_.tell(), 64);

  buffered.seek(30);
  EXPECT_EQ(buffered.bufPos_, 0);
  EXPECT_EQ(buffered.pos_, 30);
  EXPECT_EQ(buffered.end_, 64);
  EXPECT_EQ(buffered.tell(), 30);
  EXPECT_EQ(buffered.underlying_.tell(), 64);

  buffered.seek(64);
  EXPECT_EQ(buffered.bufPos_, 0);
  EXPECT_EQ(buffered.pos_, 64);
  EXPECT_EQ(buffered.end_, 64);
  EXPECT_EQ(buffered.tell(), 64);
  EXPECT_EQ(buffered.underlying_.tell(), 64);

  EXPECT_EQ(buffered.Source::read(c), 1);
  EXPECT_EQ(c, '4');
  EXPECT_EQ(buffered.bufPos_, 64);
  EXPECT_EQ(buffered.pos_, 1);
  EXPECT_EQ(buffered.end_, 64);
  EXPECT_EQ(buffered.tell(), 65);
  EXPECT_EQ(buffered.underlying_.tell(), 128);

  buffered.seek(-20, SeekMode::end);
  EXPECT_EQ(buffered.bufPos_, 64);
  EXPECT_EQ(buffered.pos_, 44);
  EXPECT_EQ(buffered.end_, 64);
  EXPECT_EQ(buffered.tell(), 108);
  EXPECT_EQ(buffered.underlying_.tell(), 128);

  buffered.seek(20);
  EXPECT_EQ(buffered.bufPos_, 20);
  EXPECT_EQ(buffered.pos_, 0);
  EXPECT_EQ(buffered.end_, 0);
  EXPECT_EQ(buffered.tell(), 20);
  EXPECT_EQ(buffered.underlying_.tell(), 20);
}

TEST(nio, FileSourceDoesNotExist) {
  FileSource in("/does/not/exist");

  EXPECT_TRUE(in.bad());
  EXPECT_EQ(in.file_, nullptr);

  auto out = in.readString();
  EXPECT_TRUE(out.empty());
  EXPECT_TRUE(in.bad());

  in.close();
  EXPECT_TRUE(in.bad());

  out = in.readString();
  EXPECT_TRUE(in.bad());
}

TEST(nio, FileSourceReadAll) {
  const auto temp = rocket::filesystem::tempFile();

  FileSink out(temp.string());
  const vector<u8> data { 0, 0, 0, 0 };
  out.write(data);
  out.close();

  FileSource in(temp.string());
  auto bytes = in.readAll();
  EXPECT_FALSE(in.bad());
  EXPECT_TRUE(in.eof());
  EXPECT_EQ(in.tell(), 4);
  EXPECT_FALSE(in.bad());
  EXPECT_EQ(bytes, data);

  bytes = in.readAll();
  EXPECT_TRUE(bytes.empty());
}

TEST(nio, FileSourceReadString) {
  const auto temp = rocket::filesystem::tempFile();

  FileSink out(temp.string());
  out.writeln("Hey there");
  out.close();

  FileSource in(temp.string());
  auto str = in.readString();
  EXPECT_FALSE(in.bad());
  EXPECT_TRUE(in.eof());
  EXPECT_EQ(str, "Hey there\n");
  str = in.readString();
  EXPECT_EQ(str, "");
  EXPECT_FALSE(in.bad());

  in.seek(-6, SeekMode::end);
  EXPECT_EQ(in.tell(), 4);
  str = in.readString();
  EXPECT_EQ(str, "there\n");
  EXPECT_EQ(in.tell(), 10);
}

TEST(nio, FileSourceScanIstream) {
  const auto path = testSource("test-nio-FileSourceScanIstream.txt");
  FILE* file = fopen(path.string().c_str(), "rb");
  FileSource in(file);
  auto& is = in.istream();
  auto result = scn::scan<log::LogLevel, log::LogLevel>(is, "{}, {}");
  ASSERT_TRUE(result);
  const auto [level1, level2] = result->values();
  EXPECT_EQ(level1, log::LogLevel::debug);
  EXPECT_EQ(level2, log::LogLevel::info);
  EXPECT_EQ(io::tellg(is), 11);
}

TEST(nio, NullSource) {
  NullSource in;
  EXPECT_FALSE(in.bad());
  EXPECT_TRUE(in.eof());
  char c; // NOLINT
  EXPECT_EQ(in.Source::read(c), 0);
}

TEST(nio, SpanSource) {
  vector<u8> vec { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!' };
  SpanSource in(vec);
  const auto str = in.readString();
  EXPECT_EQ(str, "Hello, world!");
}

TEST(nio, StreamSourceReadAll) {
  auto temp = rocket::filesystem::tempFile();

  const vector<u8> data { 0, 0, 0, 0 };

  {
    ofstream os(temp.c_str(), ios::binary);
    StreamSink out(os);
    out.write(data);
  }

  ifstream is(temp.c_str());
  StreamSource in(is);
  auto bytes = in.readAll();
  EXPECT_FALSE(in.bad());
  EXPECT_TRUE(in.eof());
  EXPECT_EQ(in.tell(), 4);
  EXPECT_EQ(bytes, data);

  bytes = in.readAll();
  EXPECT_TRUE(bytes.empty());
}

TEST(nio, StreamSourceReadString) {
  auto temp = rocket::filesystem::tempFile();

  {
    ofstream os(temp.c_str(), ios::binary);
    StreamSink out(os);
    out.writeln("Hey there");
  }

  ifstream is(temp.c_str());
  StreamSource in(is);
  auto str = in.readString();
  EXPECT_FALSE(in.bad());
  EXPECT_TRUE(in.eof());
  EXPECT_EQ(in.tell(), 10);
  EXPECT_EQ(str, "Hey there\n");
  str = in.readString();
  EXPECT_EQ(str, "");

  in.seek(-6, SeekMode::end);
  EXPECT_EQ(in.tell(), 4);
  str = in.readString();
  EXPECT_EQ(str, "there\n");
  EXPECT_EQ(in.tell(), 10);
}

TEST(nio, StringSource) {
  StringSource in("Hello, world!");
  const auto str = in.readString();
  EXPECT_EQ(str, "Hello, world!");
}

TEST(nio, StringSourceReadln) {
  StringSource in("First line\r\nSecond line\n");
  string line = in.Source::readln();
  EXPECT_EQ(line, "First line");
  line = in.Source::readln();
  EXPECT_EQ(line, "Second line");
  line = in.Source::readln();
  EXPECT_EQ(line, "");
}

TEST(nio, StringSourceSeek) {
  const string str128 = testString(128);
  StringSource in(str128);

  in.seek(0);
  EXPECT_EQ(in.tell(), 0);
  char c = '\0';
  EXPECT_EQ(in.Source::read(c), 1);
  EXPECT_EQ(c, '0');
  EXPECT_EQ(in.tell(), 1);

  in.seek(127);
  EXPECT_EQ(in.tell(), 127);
  EXPECT_EQ(in.Source::read(c), 1);
  EXPECT_EQ(c, '7');
  EXPECT_EQ(in.tell(), 128);

  in.seek(129);
  EXPECT_EQ(in.tell(), 128);

  in.seek(-11, SeekMode::cur);
  EXPECT_EQ(in.tell(), 117);

  EXPECT_THAT([&] { in.seek(-1); }, Throws<system_error>());

  in.seek(0, SeekMode::end);
  EXPECT_EQ(in.tell(), 128);

  in.seek(-10, SeekMode::end);
  EXPECT_EQ(in.tell(), 118);

  in.seek(3, SeekMode::end);
  EXPECT_EQ(in.tell(), 128);

  EXPECT_THAT([&] { in.seek(-200, SeekMode::end); }, Throws<system_error>());
}

// EOF
