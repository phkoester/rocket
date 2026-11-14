/*
 * test-io.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/filesystem/filesystem.h"
#include "rocket/io/IoError.h"
#include "rocket/io/io.h"

#include <cerrno>
#include <functional>

namespace {

// Local constants ------------------------------------------------------------------------------------------

// These have 39 digits, the same as the greatest `u128`, but are greater. Multiplying them out wraps around
// to a value that can look perfectly valid
const string FOUR_E38 = "4" + string(38, '0');
const string NINE_E38 = "9" + string(38, '0');

const string TWO_TO_128 = "340282366920938463463374607431768211456";

// Local functions ------------------------------------------------------------------------------------------

/**
 * Checks that reading @p input overflows: it fails, consumes all of the input, and yields @p expected, the
 * greatest or the lowest value, as the standard extractors do.
 */
template<typename T>
void
expectOverflow(const string& input, T expected) {
  SCOPED_TRACE(input);

  auto val = static_cast<T>(42);
  auto is = io::is(input);
  is >> val;
  EXPECT_ISTREAM(is, true, true, static_cast<i32>(input.size()));
  EXPECT_EQ(val, expected);
}

/// Checks that reading @p input succeeds, consumes all of it, and yields @p expected.
template<typename T>
void
expectValue(const string& input, T expected) {
  SCOPED_TRACE(input);

  auto val = static_cast<T>(42);
  auto is = io::is(input);
  is >> val;
  EXPECT_ISTREAM(is, false, true, static_cast<i32>(input.size()));
  EXPECT_EQ(val, expected);
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

// `FILE` functions ...........................................................................................

namespace {

/// Closes a `FILE`.
struct FileCloser {
  void operator()(FILE* file) const { fclose(file); }
};

using FilePtr = unique_ptr<FILE, FileCloser>;

} // namespace

TEST(io, open) {
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

TEST(io, seekAndTellFile) {
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

  // With seekability passed in, the stream is not asked. A stream said not to be seekable has no position
  EXPECT_TRUE(io::seek(file.get(), 5, SEEK_SET, true));
  EXPECT_EQ(io::tell(file.get(), true), 5);
  errno = 0;
  EXPECT_FALSE(io::seek(file.get(), 0, SEEK_SET, false));
  EXPECT_EQ(errno, ESPIPE);
  EXPECT_EQ(io::tell(file.get(), false), NPOS);
  EXPECT_EQ(io::tell(file.get()), 5); // Unchanged
}

#ifndef ROCKET_OS_WINDOWS
TEST(io, seekAndTellPipe) {
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

TEST(io, seekAndTellMemoryStream) {
  // A stream without a file descriptor is seekable if the C library reports a position for it
  string buf = "abcdef";
  const FilePtr file(fmemopen(buf.data(), buf.size(), "rb"));
  ASSERT_NE(file, nullptr);
  EXPECT_TRUE(io::seekable(file.get()));
  EXPECT_TRUE(io::seek(file.get(), 4, SEEK_SET));
  EXPECT_EQ(io::tell(file.get()), 4);
  EXPECT_EQ(fgetc(file.get()), 'e');
}
#endif // !ROCKET_OS_WINDOWS

TEST(io, i128OpInput) {
  using compareType = i32;
  compareType compare = 0;
  auto compareLimits = numeric_limits<compareType>();

  using type = i128;
  type val = 0;
  auto limits = numeric_limits<type>();

  // Empty input
  {
    const string input;

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 0);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 0);
  }

  // Invalid character
  {
    const string input = "x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 0);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, false, 0);
  }

  // Valid character, invalid input
  {
    const string input = "-";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 1);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 1);
  }

  // Valid character, invalid input
  {
    const string input = "-x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 1);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, false, 1);
  }

  // Valid input, EOF
  {
    const string input = "-1";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 2);
    EXPECT_EQ(compare, -1);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 2);
    EXPECT_EQ(val, -1);
  }

  // Valid input, no EOF
  {
    const string input = "-999999x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, false, 7);
    EXPECT_EQ(compare, -999999);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, false, 7);
    EXPECT_EQ(val, -999999);
  }

  // MIN
  {
    const string compareInput = "-2147483648";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 11);
    EXPECT_EQ(compare, compareLimits.min());

    const string input = "-170141183460469231731687303715884105728";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 40);
    EXPECT_EQ(val, limits.min());
  }

  // MIN - 1, overflow
  {
    const string compareInput = "-2147483649";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 11);

    const string input = "-170141183460469231731687303715884105729";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 40);
  }

  // MAX
  {
    const string compareInput = "2147483647";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 10);
    EXPECT_EQ(compare, compareLimits.max());

    const string input = "170141183460469231731687303715884105727";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 39);
    EXPECT_EQ(val, limits.max());
  }

  // MAX + 1, overflow
  {
    const string compareInput = "2147483648";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 10);

    const string input = "170141183460469231731687303715884105728";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 39);
  }

  // MAX * 10, overflow
  {
    const string compareInput = "21474836470";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 11);

    const string input = "1701411834604692317316873037158841057280";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 40);
  }

  // MAX * 100, overflow
  {
    const string compareInput = "214748364700";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 12);

    const string input = "17014118346046923173168730371588410572800";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 41);
  }
}

TEST(io, i128OpInputOverflow) {
  constexpr auto MAX = numeric_limits<i128>::max();
  constexpr auto MIN = numeric_limits<i128>::min();

  expectOverflow<i128>(FOUR_E38, MAX);
  expectOverflow<i128>("+" + FOUR_E38, MAX);
  expectOverflow<i128>("-" + FOUR_E38, MIN);
  expectOverflow<i128>(NINE_E38, MAX);
  expectOverflow<i128>("-" + NINE_E38, MIN);
  expectOverflow<i128>(TWO_TO_128, MAX);
  expectOverflow<i128>("-" + TWO_TO_128, MIN);
  expectOverflow<i128>("1" + string(39, '0'), MAX);
}

TEST(io, i128OpInputOverflowStopsAtNonDigit) {
  auto val = static_cast<i128>(42);
  const string input = FOUR_E38 + "x"; // `io::is` only keeps a view, so this must outlive the stream
  auto is = io::is(input);
  is >> val;
  EXPECT_ISTREAM(is, true, false, 39);
  EXPECT_EQ(val, numeric_limits<i128>::max());
}

TEST(io, i128OpInputLimits) {
  using limits = numeric_limits<i128>;

  expectValue<i128>("170141183460469231731687303715884105727", limits::max());
  expectValue<i128>("+170141183460469231731687303715884105727", limits::max());
  expectValue<i128>("-170141183460469231731687303715884105728", limits::min());
  expectValue<i128>("-170141183460469231731687303715884105727", limits::min() + 1);
}

TEST(io, i128OpInputLeadingZeros) {
  using limits = numeric_limits<i128>;

  expectValue<i128>(string(50, '0') + "1", static_cast<i128>(1));
  expectValue<i128>("-" + string(50, '0') + "1", static_cast<i128>(-1));
  expectValue<i128>(string(10, '0') + "170141183460469231731687303715884105727", limits::max());
  expectValue<i128>("-" + string(10, '0') + "170141183460469231731687303715884105728", limits::min());
  expectValue<i128>(string(50, '0'), static_cast<i128>(0));
  expectValue<i128>("-0", static_cast<i128>(0));
}

TEST(io, i128OpOutput) {
  using type = i128;

  using limits = numeric_limits<type>;

  {
    ostringstream os;
    os << limits::min();
    EXPECT_EQ(os.str(), "-170141183460469231731687303715884105728");
  }

  {
    ostringstream os;
    os << limits::max();
    EXPECT_EQ(os.str(), "170141183460469231731687303715884105727");
  }
}

TEST(io, u128OpInput) {
  using compareType = u32;
  compareType compare = 0;
  auto compareLimits = numeric_limits<compareType>();

  using type = u128;
  type val = 0;
  using limits = numeric_limits<type>;

  // Empty input
  {
    const string input;

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 0);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 0);
  }

  // Invalid character
  {
    const string input = "x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 0);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, false, 0);
  }

  // Valid character, invalid input
  {
    const string input = "-";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 1);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 1);
  }

  // Valid character, invalid input
  {
    const string input = "-x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, false, 1);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, false, 1);
  }

  // Valid input, EOF
  {
    const string input = "1";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 1);
    EXPECT_EQ(compare, 1);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 1);
    EXPECT_EQ(val, 1U);
  }

  // Valid input, no EOF
  {
    const string input = "999999x";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, false, 6);
    EXPECT_EQ(compare, 999999);

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, false, 6);
    EXPECT_EQ(val, 999999U);
  }

  // MIN
  {
    const string input = "0";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 1);
    EXPECT_EQ(compare, compareLimits.min());

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 1);
    EXPECT_EQ(val, limits::min());
  }

  // MIN - 1, e.g "-1", which is accepted
  {
    const string input = "-1";

    auto isCompare = io::is(input);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 2);
    EXPECT_EQ(compare, compareLimits.max());

    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 2);
    EXPECT_EQ(val, limits::max());
  }

  // MAX
  {
    const string compareInput = "4294967295";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, false, true, 10);
    EXPECT_EQ(compare, compareLimits.max());

    const string input = "340282366920938463463374607431768211455";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, false, true, 39);
    EXPECT_EQ(val, limits::max());
  }

  // MAX + 1, overflow
  {
    const string compareInput = "4294967296";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 10);

    const string input = "340282366920938463463374607431768211456";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 39);
  }

  // MAX * 10, overflow
  {
    const string compareInput = "42949672960";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 11);

    const string input = "3402823669209384634633746074317682114560";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 40);
  }

  // MAX * 100, overflow
  {
    const string compareInput = "429496729600";
    auto isCompare = io::is(compareInput);
    isCompare >> compare;
    EXPECT_ISTREAM(isCompare, true, true, 12);

    const string input = "34028236692093846346337460743176821145600";
    auto is = io::is(input);
    is >> val;
    EXPECT_ISTREAM(is, true, true, 41);
  }
}

TEST(io, u128OpInputOverflow) {
  // Like the standard extractors for unsigned types, the greatest value is the result even for a negative
  // value
  constexpr auto MAX = numeric_limits<u128>::max();

  expectOverflow<u128>(FOUR_E38, MAX); // Used to yield 59717633079061536536625392568231788544
  expectOverflow<u128>("+" + FOUR_E38, MAX);
  expectOverflow<u128>(NINE_E38, MAX);
  expectOverflow<u128>(TWO_TO_128, MAX); // Wraps around to 0
  expectOverflow<u128>("680564733841876926926749214863536422912", MAX); // 2^129, also wraps around to 0
  expectOverflow<u128>("1" + string(39, '0'), MAX);
  expectOverflow<u128>("-" + FOUR_E38, MAX);
  expectOverflow<u128>("-" + TWO_TO_128, MAX);
}

TEST(io, u128OpInputOverflowStopsAtNonDigit) {
  auto val = static_cast<u128>(42);
  const string input = FOUR_E38 + "x"; // `io::is` only keeps a view, so this must outlive the stream
  auto is = io::is(input);
  is >> val;
  EXPECT_ISTREAM(is, true, false, 39);
  EXPECT_EQ(val, numeric_limits<u128>::max());
}

TEST(io, u128OpInputNegative) {
  // Like the standard extractors for unsigned types, any magnitude up to the greatest value is negated, and
  // wraps around
  {
    u32 compare = 0;
    auto is = io::is("-4294967295");
    is >> compare;
    EXPECT_ISTREAM(is, false, true, 11);
    EXPECT_EQ(compare, 1);
  }

  constexpr auto MAX = numeric_limits<u128>::max();
  expectValue<u128>("-1", MAX);
  expectValue<u128>("-0", 0);
  expectValue<u128>("-340282366920938463463374607431768211455", 1); // Used to fail beyond the `i128` range
  expectValue<u128>("-170141183460469231731687303715884105728", static_cast<u128>(1) << 127);
  expectValue<u128>("-170141183460469231731687303715884105729", MAX >> 1);
}

TEST(io, u128OpInputLimits) {
  using limits = numeric_limits<u128>;

  expectValue<u128>("340282366920938463463374607431768211455", limits::max());
  expectValue<u128>("+340282366920938463463374607431768211455", limits::max());
  expectValue<u128>("340282366920938463463374607431768211454", limits::max() - 1);
}

TEST(io, u128OpInputLeadingZeros) {
  using limits = numeric_limits<u128>;

  expectValue<u128>(string(50, '0') + "1", static_cast<u128>(1));
  expectValue<u128>(string(10, '0') + "340282366920938463463374607431768211455", limits::max());
  expectValue<u128>(string(50, '0'), static_cast<u128>(0));
}

TEST(io, u128OpOutput) {
  using type = u128;

  using limits = numeric_limits<type>;

  {
    ostringstream os;
    os << limits::min();
    EXPECT_EQ(os.str(), "0");
  }

  {
    ostringstream os;
    os << limits::max();
    EXPECT_EQ(os.str(), "340282366920938463463374607431768211455");
  }
}

// Format flags, compared with the standard extractors and inserters ........................................

namespace {

/// A named setup of the format flags, the width, and the fill character of a stream.
struct Setup {
  string name;
  function<void(ios&)> apply;
};

/// A numeric punctuation facet with a given thousands separator, grouping, and decimal point.
struct TestNumpunct : numpunct<char> {
  TestNumpunct(char thousandsSep, string grouping, char decimalPoint) :
    thousandsSep_(thousandsSep),
    grouping_(std::move(grouping)),
    decimalPoint_(decimalPoint) {}

protected:

  char do_thousands_sep() const override { return thousandsSep_; }
  string do_grouping() const override { return grouping_; }
  char do_decimal_point() const override { return decimalPoint_; }

private:

  char thousandsSep_;
  string grouping_;
  char decimalPoint_;
};

/// Returns a function that imbues a stream with a locale using a #TestNumpunct.
function<void(ios&)>
imbue(char thousandsSep, const string& grouping, char decimalPoint = '.') {
  const locale loc(locale::classic(), new TestNumpunct(thousandsSep, grouping, decimalPoint)); // NOLINT
  return [loc](ios& s) { s.imbue(loc); };
}

/// Returns setups with locales that group digits, which apply to both input and output.
vector<Setup>
groupingSetups() {
  const auto group3 = imbue('\'', "\3");
  return {
    { "group 3", group3 },
    { "group 3 2", imbue('\'', "\3\2") },
    { "group 1 2 3", imbue('\'', "\1\2\3") },
    { "group 3 then 0", imbue('\'', string("\3\0", 2)) },
    { "group 2 then 0 then 1", imbue('\'', string("\2\0\1", 3)) },
    { "group 2 then none (CHAR_MAX)", imbue('\'', string { '\2', numeric_limits<char>::max() }) },
    { "group none (CHAR_MAX)", imbue('\'', string(1, numeric_limits<char>::max())) },
    { "group none (negative)", imbue('\'', string(1, static_cast<char>(-1))) },
    { "group 3, German", imbue('.', "\3", ',') },
    { "group 3, space", imbue(' ', "\3") },
    { "group 3, minus", imbue('-', "\3") },
    { "group 3, plus as decimal point", imbue('\'', "\3", '+') },
    { "group 3 hex", [=](ios& s) { group3(s); s.setf(ios::hex, ios::basefield); } },
    { "group 3 oct", [=](ios& s) { group3(s); s.setf(ios::oct, ios::basefield); } },
    { "group 3 noskipws", [=](ios& s) { group3(s); s.unsetf(ios::skipws); } },
    // A thousands separator or a decimal point that is also a hexadecimal digit
    { "group 3 hex, separator c", [](ios& s) { imbue('c', "\3")(s); s.setf(ios::hex, ios::basefield); } },
    { "group 3 hex, decimal point b", [](ios& s) {
      imbue('\'', "\3", 'b')(s);
      s.setf(ios::hex, ios::basefield);
    } },
  };
}

/// Returns setups that affect input.
vector<Setup>
inputSetups() {
  vector<Setup> ret {
    { "dec", [](ios&) {} },
    { "hex", [](ios& s) { s.setf(ios::hex, ios::basefield); } },
    { "oct", [](ios& s) { s.setf(ios::oct, ios::basefield); } },
    { "detect", [](ios& s) { s.unsetf(ios::basefield); } },
    { "oct hex", [](ios& s) { s.setf(ios::oct | ios::hex, ios::basefield); } },
    { "noskipws", [](ios& s) { s.unsetf(ios::skipws); } },
    { "group 3 detect", [](ios& s) { imbue('\'', "\3")(s); s.unsetf(ios::basefield); } },
  };
  ranges::copy(groupingSetups(), back_inserter(ret));
  return ret;
}

/// Returns setups that affect output.
vector<Setup>
outputSetups() {
  const auto base = [](ios::fmtflags flags) { return [flags](ios& s) { s.setf(flags, ios::basefield); }; };
  const auto group3 = imbue('\'', "\3");
  vector<Setup> ret {
    { "default", [](ios&) {} },
    { "hex", base(ios::hex) },
    { "oct", base(ios::oct) },
    { "oct hex", base(ios::oct | ios::hex) },
    { "no base", [](ios& s) { s.unsetf(ios::basefield); } },
    { "hex uppercase", [=](ios& s) { base(ios::hex)(s); s.setf(ios::uppercase); } },
    { "hex showbase", [=](ios& s) { base(ios::hex)(s); s.setf(ios::showbase); } },
    { "hex showbase uppercase", [=](ios& s) { base(ios::hex)(s); s.setf(ios::showbase | ios::uppercase); } },
    { "oct showbase", [=](ios& s) { base(ios::oct)(s); s.setf(ios::showbase); } },
    { "showbase", [](ios& s) { s.setf(ios::showbase); } },
    { "showpos", [](ios& s) { s.setf(ios::showpos); } },
    { "hex showpos", [=](ios& s) { base(ios::hex)(s); s.setf(ios::showpos); } },
    { "width", [](ios& s) { s.width(8); } },
    { "width too narrow", [](ios& s) { s.width(1); } },
    { "width fill", [](ios& s) { s.width(8); s.fill('*'); } },
    { "width left", [](ios& s) { s.width(8); s.setf(ios::left, ios::adjustfield); } },
    { "width right", [](ios& s) { s.width(8); s.setf(ios::right, ios::adjustfield); } },
    { "width internal", [](ios& s) { s.width(8); s.setf(ios::internal, ios::adjustfield); } },
    { "width internal showpos", [](ios& s) {
      s.width(8);
      s.setf(ios::internal, ios::adjustfield);
      s.setf(ios::showpos);
    } },
    { "width internal hex showbase", [=](ios& s) {
      s.width(10);
      s.setf(ios::internal, ios::adjustfield);
      base(ios::hex)(s);
      s.setf(ios::showbase);
    } },
    { "width internal oct showbase", [=](ios& s) {
      s.width(10);
      s.setf(ios::internal, ios::adjustfield);
      base(ios::oct)(s);
      s.setf(ios::showbase);
    } },
    { "group 3 hex showbase uppercase", [=](ios& s) {
      group3(s);
      base(ios::hex)(s);
      s.setf(ios::showbase | ios::uppercase);
    } },
    { "group 3 width internal showpos", [=](ios& s) {
      group3(s);
      s.width(12);
      s.setf(ios::internal, ios::adjustfield);
      s.setf(ios::showpos);
    } },
    { "group 3 width left", [=](ios& s) {
      group3(s);
      s.width(12);
      s.setf(ios::left, ios::adjustfield);
    } },
  };
  ranges::copy(groupingSetups(), back_inserter(ret));
  return ret;
}

/**
 * Checks that reading @p input into a `T` yields the same state, position, and value as reading it into a
 * `Std`, a standard type of the same signedness.
 */
template<typename T, typename Std>
void
expectInputLikeStd(const string& input, const Setup& setup) {
  SCOPED_TRACE(fmt::format("{:?}, {}", input, setup.name));

  auto stdIs = io::is(input);
  setup.apply(stdIs);
  auto stdVal = static_cast<Std>(42);
  stdIs >> stdVal;

  auto is = io::is(input);
  setup.apply(is);
  auto val = static_cast<T>(42);
  is >> val;

  EXPECT_EQ(is.rdstate(), stdIs.rdstate());
  EXPECT_EQ(io::tell(is), io::tell(stdIs));
  // A negative value wraps around at a different width for unsigned types, so compare as signed values
  EXPECT_EQ(static_cast<i128>(val), static_cast<i128>(static_cast<i64>(stdVal)));
}

/**
 * Checks that writing @p val as a `T` yields the same as writing it as a `Std`, a standard type of the same
 * signedness. Writes the value twice, because the width only applies to the first output.
 */
template<typename T, typename Std>
void
expectOutputLikeStd(Std val, const Setup& setup) {
  SCOPED_TRACE(fmt::format("{}, {}", val, setup.name));

  ostringstream stdOs;
  setup.apply(stdOs);
  stdOs << val << '|' << val;

  ostringstream os;
  setup.apply(os);
  os << static_cast<T>(val) << '|' << static_cast<T>(val);

  EXPECT_EQ(os.str(), stdOs.str());
}

} // namespace

TEST(io, int128OpInputLikeStd) {
  const vector<string> inputs {
    "", "   ", "x", "+", "-", "-x", "42", "+42", "-42", " \t42 rest", "0", "-0", "007", "08", "17", "0x1f",
    "0X1F;", "0x", "0xg", "-0x10", "00x1", "ff", "FfZ", "g", "777", "1e3",
    // Thousands separators and decimal points of the locales in `groupingSetups`
    "1'234", "1'234'567", "12'34'567", "1'2'34'567", "12'34", "1'2", "1'23", "1234", "1234567", "'123",
    "1''234", "1'", "0'123", "00'1", "-1'234", "+1'234", "-'1", "0x1'f", "0'x1", "1'234 rest", "12'345'67",
    "922'337'203'685'477", "1'234'567'890'123'456", "1,5", "1.5", "1.234,5", "1.234.567", "1-234",
    "1 234", " 1 234", "1+5", "1'234+5", "-1-234", "ab'cd", "7'7'7", "1234'567", "12345'678", "1'2345",
    "1c234", "1cc2", "c12", "1c23c456", "1b2", "0b1", "b1",
  };
  for (const auto& setup : inputSetups()) {
    for (const auto& input : inputs) {
      expectInputLikeStd<i128, i64>(input, setup);
      expectInputLikeStd<u128, u64>(input, setup);
    }
  }
}

TEST(io, int128OpInputExceptions) {
  {
    // A stream buffer that throws sets `badbit`. The exception is only rethrown if `badbit` is in the
    // exception mask
    for (const bool rethrow : { false, true }) {
      SCOPED_TRACE(rethrow);

      io::testing::FailingBuf stdBuf("12");
      istream stdIs(&stdBuf);
      auto stdVal = static_cast<i64>(42);

      io::testing::FailingBuf buf("12");
      istream is(&buf);
      auto val = static_cast<i128>(42);

      if (rethrow) {
        stdIs.exceptions(ios::badbit);
        is.exceptions(ios::badbit);
        EXPECT_THROW(stdIs >> stdVal, runtime_error);
        EXPECT_THROW(is >> val, runtime_error);
      } else {
        stdIs >> stdVal;
        is >> val;
      }
      EXPECT_EQ(is.rdstate(), stdIs.rdstate());
      EXPECT_TRUE(is.bad());
      EXPECT_EQ(val, stdVal);
    }
  }

  {
    // With `failbit` in the exception mask, a failure throws, after the value has been set
    auto stdIs = io::is("x");
    stdIs.exceptions(ios::failbit);
    auto stdVal = static_cast<i64>(42);
    EXPECT_THROW(stdIs >> stdVal, ios::failure);

    auto is = io::is("x");
    is.exceptions(ios::failbit);
    auto val = static_cast<u128>(42);
    EXPECT_THROW(is >> val, ios::failure);

    EXPECT_EQ(is.rdstate(), stdIs.rdstate());
    EXPECT_EQ(val, static_cast<u128>(stdVal));
  }
}

TEST(io, int128OpOutputLikeStd) {
  const vector<i64> signedValues {
    0, 1, 42, 255, 4'096, 12'345, 1'234'567, -1, -42, -1'234'567, numeric_limits<i64>::max(),
    numeric_limits<i64>::min(),
  };
  const vector<u64> unsignedValues { 0, 1, 42, 255, 4'096, 12'345, 1'234'567, numeric_limits<u64>::max() };

  for (const auto& setup : outputSetups()) {
    ostringstream probe;
    setup.apply(probe);
    const auto basefield = probe.flags() & ios::basefield;
    const bool decimal = basefield != ios::oct && basefield != ios::hex;

    for (const auto val : signedValues) {
      // Negative values in octal and hexadecimal are written in two's complement, so they differ in width
      if (val >= 0 || decimal) {
        expectOutputLikeStd<i128>(val, setup);
      }
    }
    for (const auto val : unsignedValues) {
      expectOutputLikeStd<u128>(val, setup);
    }
  }
}

TEST(io, int128OpGroupingBeyond64Bits) {
  // Digit grouping also works for values that do not fit into 64 bits, and round-trips
  const auto group3 = imbue('\'', "\3");
  const auto group32 = imbue('\'', "\3\2");

  const auto format = [](auto val, const function<void(ios&)>& setup) {
    ostringstream os;
    setup(os);
    os << val;
    return os.str();
  };
  EXPECT_EQ(format(numeric_limits<i128>::max(), group3), "170'141'183'460'469'231'731'687'303'715'884'105'727");
  EXPECT_EQ(
    format(numeric_limits<i128>::min(), group3), "-170'141'183'460'469'231'731'687'303'715'884'105'728");
  EXPECT_EQ(
    format(numeric_limits<u128>::max(), group32),
    "34'02'82'36'69'20'93'84'63'46'33'74'60'74'31'76'82'11'455");

  for (const auto& [text, setup] : vector<pair<string, function<void(ios&)>>> {
         { "-170'141'183'460'469'231'731'687'303'715'884'105'728", group3 },
         { "34'02'82'36'69'20'93'84'63'46'33'74'60'74'31'76'82'11'455", group32 },
       }) {
    SCOPED_TRACE(text);
    auto is = io::is(text);
    setup(is);
    if (text.starts_with('-')) {
      i128 val = 0;
      is >> val;
      EXPECT_ISTREAM(is, false, true, static_cast<i32>(text.size()));
      EXPECT_EQ(val, numeric_limits<i128>::min());
    } else {
      u128 val = 0;
      is >> val;
      EXPECT_ISTREAM(is, false, true, static_cast<i32>(text.size()));
      EXPECT_EQ(val, numeric_limits<u128>::max());
    }
  }

  // A mismatched grouping still yields the value, but sets `failbit`
  auto is = io::is("170'141'183'460'469'231'731'687'303'715'884'1'05'727");
  group3(is);
  i128 val = 0;
  is >> val;
  EXPECT_TRUE(is.fail());
  EXPECT_EQ(val, numeric_limits<i128>::max());
}

TEST(io, int128OpOutputTwosComplement) {
  // As with the standard inserters, a negative value is written as its two's complement in octal and
  // hexadecimal
  const auto format = [](auto val, auto... manipulators) {
    ostringstream os;
    (os << ... << manipulators) << val;
    return os.str();
  };

  EXPECT_EQ(format(static_cast<i128>(-1), hex), string(32, 'f'));
  EXPECT_EQ(format(static_cast<i128>(-1), oct), "3" + string(42, '7'));
  EXPECT_EQ(format(numeric_limits<i128>::min(), hex, showbase, uppercase), "0X8" + string(31, '0'));
  EXPECT_EQ(format(numeric_limits<i128>::min(), showpos), "-170141183460469231731687303715884105728");
  EXPECT_EQ(format(numeric_limits<i128>::max(), showpos), "+170141183460469231731687303715884105727");

  EXPECT_EQ(format(numeric_limits<u128>::max(), hex), string(32, 'f'));
  EXPECT_EQ(format(numeric_limits<u128>::max(), oct, showbase), "03" + string(42, '7'));
  EXPECT_EQ(format(numeric_limits<u128>::max(), showpos), "340282366920938463463374607431768211455");
}

// EOF
