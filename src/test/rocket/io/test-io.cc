/*
 * test-io.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/filesystem/filesystem.h"
#include "rocket/io/io.h"

#include <cerrno>

#define MODULE rocketIo

namespace {

// `FileCloser` ---------------------------------------------------------------------------------------------

struct FileCloser {
  void operator()(FILE* file) const { fclose(file); }
};

using FilePtr = unique_ptr<FILE, FileCloser>;

} // namespace

// Tests ====================================================================================================

TEST(MODULE, open) {
  const auto temp = rocket::filesystem::tempFile();

  {
    const FilePtr file(io::open(temp, "wb"));
    ASSERT_NE(file, nullptr);
    EXPECT_EQ(fwrite("abc", 1, 3, file.get()), 3);
  }
  {
    const FilePtr file(io::open(temp, "ab")); // Appends
    ASSERT_NE(file, nullptr);
    EXPECT_EQ(fwrite("de", 1, 2, file.get()), 2);
  }
  {
    const FilePtr file(io::open(temp, "rb"));
    ASSERT_NE(file, nullptr);
    array<char, 8> buf {};
    EXPECT_EQ(fread(buf.data(), 1, buf.size(), file.get()), 5);
    EXPECT_EQ(string_view(buf.data(), 5), "abcde");
  }

  // A path with characters outside ASCII, which on Windows are passed to the C runtime as a wide string
  const auto unicode = temp.parent_path() / (temp.filename().string() + " ä€\U0001F600");
  {
    const FilePtr file(io::open(unicode, "wb"));
    ASSERT_NE(file, nullptr);
  }
  EXPECT_TRUE(std::filesystem::exists(unicode));
  std::filesystem::remove(unicode);

  // Errors
  EXPECT_EQ(io::open("/does/not/exist", "rb"), nullptr);
  EXPECT_EQ(io::open(temp.parent_path() / "does-not-exist", "rb"), nullptr);
}

TEST(MODULE, seekAndTellFile) {
  const auto temp = rocket::filesystem::tempFile();
  {
    const FilePtr file(io::open(temp, "wb"));
    ASSERT_NE(file, nullptr);
    fwrite("0123456789", 1, 10, file.get());
  }

  const FilePtr file(io::open(temp, "rb"));
  ASSERT_NE(file, nullptr);
  EXPECT_TRUE(io::seekable(file.get()));
  EXPECT_EQ(io::tell(file.get()), 0);

  EXPECT_TRUE(io::seek(file.get(), 3, SEEK_SET));
  EXPECT_EQ(io::tell(file.get()), 3);
  EXPECT_EQ(fgetc(file.get()), '3');
  EXPECT_EQ(io::tell(file.get()), 4);
  EXPECT_TRUE(io::seek(file.get(), -2, SEEK_CUR));
  EXPECT_EQ(io::tell(file.get()), 2);
  EXPECT_TRUE(io::seek(file.get(), -1, SEEK_END));
  EXPECT_EQ(io::tell(file.get()), 9);
  EXPECT_EQ(fgetc(file.get()), '9');
  EXPECT_EQ(fgetc(file.get()), EOF);
  EXPECT_TRUE(feof(file.get()));
  EXPECT_TRUE(io::seek(file.get(), 0, SEEK_SET)); // Clears the end-of-file indicator
  EXPECT_FALSE(feof(file.get()));
  EXPECT_EQ(io::tell(file.get()), 0);

  // Seeking before the beginning fails, and sets `errno`
  errno = 0;
  EXPECT_FALSE(io::seek(file.get(), -1, SEEK_SET));
  EXPECT_NE(errno, 0);
  EXPECT_EQ(io::tell(file.get()), 0);
}

#ifndef ROCKET_OS_WINDOWS

TEST(MODULE, seekAndTellPipe) {
  array<i32, 2> fds {};
  ASSERT_EQ(pipe(fds.data()), 0);
  ASSERT_EQ(write(fds[1], "abc", 3), 3);
  close(fds[1]);
  const FilePtr file(fdopen(fds[0], "rb"));
  ASSERT_NE(file, nullptr);

  // A pipe is not seekable, has no position, and seeking fails without consuming anything
  EXPECT_FALSE(io::seekable(file.get()));
  EXPECT_EQ(io::tell(file.get()), NPOS);
  errno = 0;
  EXPECT_FALSE(io::seek(file.get(), 0, SEEK_SET));
  EXPECT_EQ(errno, ESPIPE);
  EXPECT_EQ(fgetc(file.get()), 'a');
  EXPECT_EQ(io::tell(file.get()), NPOS);
}

TEST(MODULE, seekAndTellMemoryStream) {
  // A stream without a file descriptor is seekable if the C library reports a position for it
  string buf = "abcdef";
  const FilePtr file(fmemopen(buf.data(), buf.size(), "rb"));
  ASSERT_NE(file, nullptr);
  EXPECT_TRUE(io::seekable(file.get()));
  EXPECT_TRUE(io::seek(file.get(), 4, SEEK_SET));
  EXPECT_EQ(io::tell(file.get()), 4);
  EXPECT_EQ(fgetc(file.get()), 'e');
}

#endif // ROCKET_OS_WINDOWS

// EOF
