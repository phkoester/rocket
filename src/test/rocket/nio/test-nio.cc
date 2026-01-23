/*
 * test-nio.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Exception.h"
#include "rocket/nio/nio.h"

#include <fstream>

using namespace rocket::nio;

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

// `TEST` ---------------------------------------------------------------------------------------------------

// `Sink` ...................................................................................................

TEST(nio, BufferedSink) {
  StringSink out;
  BufferedSink buffered(out, 64);
  EXPECT_EQ(buffered.size_, 64);

  buffered.write(string(testString(32)));
  EXPECT_EQ(buffered.pos_, 32);
  EXPECT_EQ(out.str(), "");
  buffered.write(testString(33, 2));
  EXPECT_EQ(buffered.pos_, 1);
  EXPECT_EQ(out.str(), testString(64));
  buffered.flush();
  EXPECT_EQ(out.str(), testString(65));
  EXPECT_EQ(buffered.pos_, 0);
}

TEST(nio, FileSinkDoesNotExist) {
  FileSink out("/does/not/exist");

  EXPECT_EQ(out.error(), ENOENT);
  EXPECT_EQ(out.good(), false);
  EXPECT_EQ(out.open(), false);
  EXPECT_EQ(out.terminal(), false);
  EXPECT_EQ(out.file_, nullptr);

  out.write("a");
  EXPECT_EQ(out.error(), ENOENT);

  out.close();
  EXPECT_EQ(out.error(), ENOENT);

  out.write("b");
  EXPECT_EQ(out.error(), ENOENT);
}

TEST(nio, SpanSink) {
  string str = "---[abcd]---";
  span<char> span(&str[4], 4);
  SpanSink out(span);
  EXPECT_EQ(out.write("ABCDEF"), 4); // 6 times `Y`, but only 4 fit
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
  string s65 = testString(65);
  StringSource in(s65);
  BufferedSource buffered(in, 64);
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
  StringSource in(s20);
  BufferedSource buffered(in, 64);

  string out20(20, ' ');
  span<char> outSpan20(out20);
  EXPECT_EQ(buffered.read(outSpan20), 20);
  EXPECT_EQ(buffered.pos_, 20);
  EXPECT_EQ(buffered.end_, 20);
}

TEST(nio, BufferedSourceSeek) {
  string s128 = testString(128);
  StringSource in(s128);
  BufferedSource buffered(in, 64);
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
  FileSource in("/does/not/exist");

  EXPECT_EQ(in.error(), ENOENT);
  EXPECT_EQ(in.good(), false);
  EXPECT_EQ(in.open(), false);
  EXPECT_EQ(in.terminal(), false);
  EXPECT_EQ(in.file_, nullptr);

  auto out = in.Source::read();
  EXPECT_TRUE(out.empty());
  EXPECT_EQ(in.error(), ENOENT);

  in.close();
  EXPECT_EQ(in.error(), ENOENT);

  out = in.Source::read();
  EXPECT_EQ(in.error(), ENOENT);
}

TEST(nio, FileSourceRead) {
  auto tmp = ROCKET_TEST_TEMP_PATH();

  FileSink out(tmp);
  out.writeln("Hey there");
  out.close();

  FileSource in(tmp);
  string str = in.Source::read();
  EXPECT_EQ(in.error(), 0);
  EXPECT_EQ(str, "Hey there\n");
  str = in.Source::read();
  EXPECT_EQ(str, "");
  EXPECT_EQ(in.error(), 0);

  in.seek(-6, SeekMode::end);
  EXPECT_EQ(in.tell(), 4);
  str = in.Source::read();
  EXPECT_EQ(str, "there\n");
  EXPECT_EQ(in.tell(), 10);
}

TEST(nio, StreamSourceRead) {
  auto tmp = ROCKET_TEST_TEMP_PATH();

  ofstream os(tmp.c_str());
  StreamSink out(os);
  out.writeln("Hey there");

  ifstream is(tmp.c_str());
  StreamSource in(is);
  string str = in.Source::read();
  EXPECT_EQ(in.error(), 0);
  EXPECT_EQ(in.tell(), 10);
  EXPECT_EQ(str, "Hey there\n");
  str = in.Source::read();
  EXPECT_EQ(str, "");
  EXPECT_EQ(in.error(), 0);

  in.seek(-6, SeekMode::end);
  EXPECT_EQ(in.tell(), 4);
  str = in.Source::read();
  EXPECT_EQ(str, "there\n");
  EXPECT_EQ(in.tell(), 10);
}

TEST(nio, StringSource) {
  StringSource in("Hello, world!");
  string str = in.Source::read();
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

TEST(nio, StringSourceReadlnSpan) {
  StringSource in("First line\r\nSecond line\n");
  string s11 = string(11, ' ');
  span<char> span11 = span<char>(s11);
  u64 n = in.readln(span11);
  EXPECT_EQ(n, 11);
  EXPECT_EQ(string_view(span11.data(), n), "First line\r");

  in.seek(0);
  string s12 = string(12, ' ');
  span<char> span12 = span<char>(s12);
  n = in.readln(span12);
  EXPECT_EQ(n, 10);
  EXPECT_EQ(string_view(span12.data(), n), "First line");

  string s20 = string(20, ' ');
  span<char> span20 = span<char>(s20);
  n = in.readln(span20);
  EXPECT_EQ(n, 11);
  EXPECT_EQ(string_view(span20.data(), n), "Second line");
}

TEST(nio, StringSourceSeek) {
  string s128 = testString(128);
  StringSource in(s128);

  in.seek(0);
  EXPECT_EQ(in.tell(), 0);
  char c;
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

  in.seek(-1);
  EXPECT_EQ(in.tell(), 0);

  in.seek(0, SeekMode::end);
  EXPECT_EQ(in.tell(), 128);

  in.seek(-10, SeekMode::end);
  EXPECT_EQ(in.tell(), 118);

  in.seek(3, SeekMode::end);
  EXPECT_EQ(in.tell(), 128);

  in.seek(-200, SeekMode::end);
  EXPECT_EQ(in.tell(), 0);
}

// EOF
