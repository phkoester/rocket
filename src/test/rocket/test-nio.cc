/*
 * test-nio.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/nio.h"

using namespace rocket;
using namespace rocket::nio;
using namespace std;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(nio, FileSink) {
  FileSink sink("/does/not/exist", FileSink::Params { .append=true });

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

// EOF
