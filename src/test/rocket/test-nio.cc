/*
 * test-nio.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/Exception.h"
#include "rocket/nio.h"

#include <filesystem>

using namespace rocket;
using namespace rocket::gtest;
using namespace rocket::nio;
using namespace std;
using namespace testing;

// Local functions ------------------------------------------------------------------------------------------

/// Returns "01234567890123...".
string
testString(size_t n, int add = 0) {
  string ret;
  for (size_t i = 0; i < n; ++i) {
    ret += static_cast<char>('0' + add);
    add = (add + 1) % 10;
  }
  return ret;
}

// `TEST` ---------------------------------------------------------------------------------------------------

// `Sink` ...................................................................................................

TEST(nio, BufferedSink) {
  StringSink s;
  BufferedSink buffered(s, 64);
  EXPECT_EQ(buffered.size_, 64);

  buffered.write(string(testString(32)));
  EXPECT_EQ(buffered.pos_, 32);
  EXPECT_EQ(s.str(), "");
  buffered.write(testString(33, 2));
  EXPECT_EQ(buffered.pos_, 1);
  EXPECT_EQ(s.str(), testString(64));
  buffered.flush();
  EXPECT_EQ(s.str(), testString(65));
  EXPECT_EQ(buffered.pos_, 0);
}

TEST(nio, FileSinkDoesNotExist) {
  FileSink sink("/does/not/exist");

  EXPECT_EQ(sink.error(), ENOENT);
  EXPECT_EQ(sink.fd(), -1);
  EXPECT_EQ(sink.good(), false);
  EXPECT_EQ(sink.open(), false);
  EXPECT_EQ(sink.file_, nullptr);

  sink.write("a");
  EXPECT_EQ(sink.error(), ENOENT);

  sink.close();
  EXPECT_EQ(sink.error(), ENOENT);

  sink.write("b");
  EXPECT_EQ(sink.error(), ENOENT);
}

TEST(nio, SpanSink) {
  string s = "---[XXXX]---";
  span<char> span(&s[4], 4);
  SpanSink sink(span);
  EXPECT_EQ(sink.write("YYYYYY"), 4); // 6 times `Y`, but only 4 fit
  EXPECT_EQ(s, "---[YYYY]---");
}

TEST(nio, StreamSink) {
  ostringstream os;
  StreamSink sink(os);
  sink.println("Hi {}", "there");
  EXPECT_EQ(os.str(), "Hi there\n");
}

TEST(nio, StringSink) {
  StringSink sink1; // With managed string
  sink1.println("Hi {}", "there");
  EXPECT_EQ(sink1.str(), "Hi there\n");

  string out;
  StringSink sink2(out); // With external string
  sink2.println("Hi {}", "there");
  EXPECT_EQ(out, "Hi there\n");
  EXPECT_THAT([&] { sink2.str(); }, Throws<InvalidState>());
}

// `Source` .................................................................................................

TEST(nio, BufferedSource) {
  string s65 = testString(65);
  StringSource source(s65);
  BufferedSource buffered(source, 64);
  EXPECT_EQ(buffered.size_, 64);
  EXPECT_EQ(buffered.pos_, 0);
  EXPECT_EQ(buffered.end_, 0);

  string s32(32, ' ');
  span<char> span32(s32);
  EXPECT_EQ(buffered.read(span32), 32);
  EXPECT_EQ(string_view(s32), testString(32));
  EXPECT_EQ(buffered.pos_, 32);
  EXPECT_EQ(buffered.end_, 64);

  string s40 = string(40, ' ');
  span<char> span40 = s40;
  EXPECT_EQ(buffered.read(span40), 33);
  EXPECT_EQ(buffered.pos_, 1);
  EXPECT_EQ(buffered.end_, 1);
  span<char> span33(&s40[0], 33);
  EXPECT_EQ(string_view(span33.data(), 33), testString(33, 2));
}

TEST(nio, BufferedSourceExactMatch) {
  string s20 = testString(20);
  StringSource source(s20);
  BufferedSource buffered(source, 64);

  string out20(20, ' ');
  span<char> outSpan20(out20);
  EXPECT_EQ(buffered.read(outSpan20), 20);
  EXPECT_EQ(buffered.pos_, 20);
  EXPECT_EQ(buffered.end_, 20);
}

TEST(nio, BufferedSourceSeek) {
  string s128 = testString(128);
  StringSource source(s128);
  BufferedSource buffered(source, 64);
  EXPECT_EQ(buffered.tell(), 0);

  char c;
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
  FileSource source("/does/not/exist");

  EXPECT_EQ(source.error(), ENOENT);
  EXPECT_EQ(source.fd(), -1);
  EXPECT_EQ(source.good(), false);
  EXPECT_EQ(source.open(), false);
  EXPECT_EQ(source.file_, nullptr);

  auto out = source.Source::read();
  EXPECT_TRUE(out.empty());
  EXPECT_EQ(source.error(), ENOENT);

  source.close();
  EXPECT_EQ(source.error(), ENOENT);

  out = source.Source::read();
  EXPECT_EQ(source.error(), ENOENT);
}

TEST(nio, FileSourceRead) {
  auto tmp = tempPath();

  FileSink sink(tmp);
  sink.writeln("Hey there");
  sink.close();

  FileSource source(tmp);
  string s = source.Source::read();
  EXPECT_EQ(source.error(), 0);
  EXPECT_EQ(s, "Hey there\n");
  s = source.Source::read();
  EXPECT_EQ(s, "");
  EXPECT_EQ(source.error(), 0);

  source.seek(-6, SeekMode::end);
  EXPECT_EQ(source.tell(), 4);
  s = source.Source::read();
  EXPECT_EQ(s, "there\n");
  EXPECT_EQ(source.tell(), 10);
}

TEST(nio, StreamSourceRead) {
  auto tmp = tempPath();

  ofstream os(tmp.c_str());
  StreamSink sink(os);
  sink.writeln("Hey there");

  ifstream is(tmp.c_str());
  StreamSource source(is);
  string s = source.Source::read();
  EXPECT_EQ(source.error(), 0);
  EXPECT_EQ(source.tell(), 10);
  EXPECT_EQ(s, "Hey there\n");
  s = source.Source::read();
  EXPECT_EQ(s, "");
  EXPECT_EQ(source.error(), 0);

  source.seek(-6, SeekMode::end);
  EXPECT_EQ(source.tell(), 4);
  s = source.Source::read();
  EXPECT_EQ(s, "there\n");
  EXPECT_EQ(source.tell(), 10);
}

TEST(nio, StringSource) {
  StringSource source("Hello, world!");
  string s = source.Source::read();
  EXPECT_EQ(s, "Hello, world!");
}

TEST(nio, StringSourceReadln) {
  StringSource source("First line\r\nSecond line\n");
  string line = source.Source::readln();
  EXPECT_EQ(line, "First line");
  line = source.Source::readln();
  EXPECT_EQ(line, "Second line");
  line = source.Source::readln();
  EXPECT_EQ(line, "");
}

TEST(nio, StringSourceReadlnSpan) {
  StringSource source("First line\r\nSecond line\n");
  string s11 = string(11, ' ');
  span<char> span11 = span<char>(s11);
  size_t n = source.readln(span11);
  EXPECT_EQ(n, 11);
  EXPECT_EQ(string_view(span11.data(), n), "First line\r");

  source.seek(0);
  string s12 = string(12, ' ');
  span<char> span12 = span<char>(s12);
  n = source.readln(span12);
  EXPECT_EQ(n, 10);
  EXPECT_EQ(string_view(span12.data(), n), "First line");

  string s20 = string(20, ' ');
  span<char> span20 = span<char>(s20);
  n = source.readln(span20);
  EXPECT_EQ(n, 11);
  EXPECT_EQ(string_view(span20.data(), n), "Second line");
}

TEST(nio, StringSourceSeek) {
  string s128 = testString(128);
  StringSource source(s128);

  source.seek(0);
  EXPECT_EQ(source.tell(), 0);
  char c;
  EXPECT_EQ(source.Source::read(c), 1);
  EXPECT_EQ(c, '0');
  EXPECT_EQ(source.tell(), 1);

  source.seek(127);
  EXPECT_EQ(source.tell(), 127);
  EXPECT_EQ(source.Source::read(c), 1);
  EXPECT_EQ(c, '7');
  EXPECT_EQ(source.tell(), 128);

  source.seek(129);
  EXPECT_EQ(source.tell(), 128);

  source.seek(-11, SeekMode::cur);
  EXPECT_EQ(source.tell(), 117);

  source.seek(-1);
  EXPECT_EQ(source.tell(), 0);

  source.seek(0, SeekMode::end);
  EXPECT_EQ(source.tell(), 128);

  source.seek(-10, SeekMode::end);
  EXPECT_EQ(source.tell(), 118);

  source.seek(3, SeekMode::end);
  EXPECT_EQ(source.tell(), 128);

  source.seek(-200, SeekMode::end);
  EXPECT_EQ(source.tell(), 0);
}

// EOF
