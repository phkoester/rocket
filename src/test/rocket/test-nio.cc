/*
 * test-nio.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/nio.h"

using namespace rocket;
using namespace rocket::nio;
using namespace std;

#include <ranges>

// `TEST` ---------------------------------------------------------------------------------------------------

// `Sink` ...................................................................................................

TEST(nio, BufferedSink) {
  StringSink s;
  BufferedSink buffered(s);
  EXPECT_EQ(buffered.size_, 64);

  for (auto s : views::repeat("x"sv, 32)) {
    buffered.write(s);
  }
  EXPECT_EQ(s.str(), string(32, 'x'));
  EXPECT_EQ(buffered.pos_, 32);
  for (auto s : views::repeat("x"sv, 33)) {
    buffered.write(s);
  }
  EXPECT_EQ(s.str(), string(65, 'x'));
  EXPECT_EQ(buffered.pos_, 1);
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

TEST(nio, StreamSink) {
  ostringstream os;
  StreamSink sink(os);
  sink.println("Hi {}", "there");
  EXPECT_EQ(os.str(), "Hi there\n");
}

// `Source` .................................................................................................

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

// EOF
