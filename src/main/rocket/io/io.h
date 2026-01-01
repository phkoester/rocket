/**
 * @file io.h
 *
 * I/O utilities.
 */

#pragma once

#include "rocket/Exception.h"
#include "rocket/Guard.h"
#include "rocket/assert.h"
#include "rocket/io/io-decl.h"
#include "rocket/message/message.h"
#include "rocket/text/text.h"
#include "rocket/unicode/unicode.h"

#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <spanstream>
#include <string>

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Makes a guard that automatically restores the I/O stream's flags upon scope exit.
 *
 * @param ios the I/O stream the flags of which are to restore upon scope exit
 */
#define ROCKET_IO_FLAGS_GUARD(ios) \
    const auto rocketIoFlagsGuard__ = ios.flags(); \
    ROCKET_GUARD([&] { ios.flags(rocketIoFlagsGuard__); })

namespace rocket::io {

// `Buffer` -------------------------------------------------------------------------------------------------

/**
 * A byte buffer reading chunk-wise from an input stream.
 *
 * The next byte can be obtained using the #get function. If the returned value is null, the input is
 * exhausted.
 *
 * A byte can be put back using the #put function.
 */
struct Buffer {
  /**
   * @ctor
   *
   * @param is the input stram
   * @param size the byte size of the buffer
   */
  explicit Buffer(std::istream& is, size_t size = DEFAULT_BUFFER_SIZE);

  /**
   * Returns the next byte from the buffer, or null if the input stream is exhausted.
   *
   * @return a byte or null
   */
  std::optional<std::byte> get();

  /**
   * Returns the next code point from the buffer, or null if the input stream is exhausted.
   *
   * @param cp if nonnull, then this is assigned the read code point, if any
   *
   * @return a byte sequence or null
   */
  std::optional<std::vector<std::byte>> getCodePoint(unicode::CodePoint* cp = nullptr);

  /**
   * Returns the next grapheme from the buffer, or null if the input stream is exhausted.
   *
   * @param gr if nonnull, then this is assigned the read grapheme, if any
   *
   * @return a byte sequence or null
   */
  std::optional<std::vector<std::byte>> getGrapheme(unicode::Grapheme* gr = nullptr);

  /**
   * Returns the current position in the input stream.
   *
   * @return the current position in the input stream
   */
  inline size_t position() const { return pos_; }

  /**
   * Puts one byte back into the buffer.
   *
   * @param b the byte to put back
   */
  void put(std::byte b);

  /**
   * Puts bytes back into the buffer.
   *
   * @param bytes the bytes to put back.
   */
  void put(const std::vector<std::byte>& bytes);

private:

  std::istream& is_; // The input stream
  size_t size_; // The buffer size in bytes
  std::unique_ptr<std::byte[]> p_; // The buffer
  size_t index_ = 0; // The current index in the chunk
  size_t end_ = 0; // The end of the chunk
  size_t pos_; // The current position in the input stream
  std::vector<std::byte> intermediate_; // Bytes that were put back
};

// `InputFailure` -------------------------------------------------------------------------------------------

/**
 * Instances of this class are thrown when reading from an input stream failed.
 */
struct InputFailure : std::ios_base::failure, Exception {
  /// @type_base
  using Base = std::ios_base::failure;

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * The stored position is set to the value of `rocket::io::tellg(is)`.
   *
   * @param is the input stream
   * @param sl the source location
   * @param st the stack trace
   */
  explicit InputFailure(
      std::istream& is,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST) :
      InputFailure(is, io::tellg(is), std::move(sl), std::move(st)) {}

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * @param is the input stream
   * @param position the position to store
   * @param sl the source location
   * @param st the stack trace
   */
  explicit InputFailure(
      std::istream& is,
      size_t position,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST) :
      InputFailure(is, position, "Input failure", std::move(sl), std::move(st)) {}

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * The stored position is set to the value of `rocket::io::tellg(is)`.
   *
   * @param is the input stream
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  InputFailure(
      std::istream& is,
      std::string_view msg,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST) :
      InputFailure(is, io::tellg(is), msg, std::move(sl), std::move(st)) {}

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * @param is the input stream
   * @param position the position to store
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  InputFailure(
      std::istream& is,
      size_t position,
      std::string_view msg,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST) :
      Base(message::exceptionBase(msg, sl)),
      Exception(msg, std::move(sl), std::move(st)),
      pos_(position) {
    if (not is.fail())
      is.setstate(std::ios::failbit);
  }

  /**
   * Returns the stored position.
   *
   * @return the stored position
   */
  size_t position() const { return pos_; }

private:

  const size_t pos_;
};

// `ParseFailure` -------------------------------------------------------------------------------------------

/**
 * Instances of this class are thrown when parsing from an input stream fails.
 *
 */
struct ParseFailure : InputFailure {
  /// @type_base
  using Base = InputFailure;

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * @param is the input stream
   * @param position the position to store
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  ParseFailure(
      std::istream& is,
      size_t position,
      std::string_view msg,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST) :
      ParseFailure(is, position, {}, msg, std::move(sl), std::move(st)) {}

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * @param is the input stream
   * @param position the position to store
   * @param range the range to store
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  ParseFailure(
      std::istream& is,
      size_t position,
      text::Range range,
      std::string_view msg,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST) :
      ParseFailure(is, position, { range }, msg, std::move(sl), std::move(st)) {}

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * @param is the input stream
   * @param position the position to store
   * @param ranges the ranges to store
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  ParseFailure(
      std::istream& is,
      size_t position,
      std::initializer_list<text::Range> ranges,
      std::string_view msg,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST):
      Base(is, position, msg, std::move(sl), std::move(st)),
      ranges_(ranges) {}

  /**
   * Returns the stored position ranges.
   *
   * @return the stored position ranges
   */
  const text::Ranges& ranges() const { return ranges_; }

private:

  const text::Ranges ranges_;
};

// `Symbols` ------------------------------------------------------------------------------------------------

/**
 * Predefined symbols.
 */
struct Symbols {
  /**
    * Hexadecimal digits: `'0' to `'9'`, `'a'` to `'f'`, `'A'` to `'F'`.
    */
  static const std::set<char> HexDigits;
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Checks the state of the input stream @p is.
 *
 * @param is the input stream
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::io::ParseFailure if `is.eof()` returns `true`
 */
void check(std::istream& is);

/**
 * Returns a file descriptor for an I/O stream.
 *
 * @param ios the stream
 * @return `STDOUT_FILENO`, `STDERR_FILENO`, `STDIN_FILENO`, or -1 if a file descriptor cannot be determined
 */
int
fd(const std::ios& ios);

/**
 * Reads a character from the input stream @p is.
 *
 * @param is the input stream
 * @return a character
 */
char getChar(std::istream& is);

/**
 * Reads a character matching @p v from the input stream @p is.
 *
 * @param is the input stream
 * @param v the expected character. This must be an ASCII character in the range [0,127]
 * @return @p v
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::io::ParseFailure if `is.eof()` returns `true` or if the read character is not @p v
 */
char getChar(std::istream& is, char v);

/**
 * Reads a character contained in @p values from the input stream @p is.
 *
 * @param is the input stream
 * @param values expected characters. These must be ASCII characters in the range [0,127]
 * @return a character contained in @p values
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::io::ParseFailure if `is.eof()` returns` true` or if the read character is not
 *     contained in @p values
 */
char getChar(std::istream& is, const std::set<char>& values);

/**
 * Reads @p n hexadecimal characters from the input stream @p is and converts them to an integer value.
 *
 * @tparam I the integer type
 * @param is the input stream
 * @param n the number of hexadecimal characters to read
 * @param input after the function returns, @p input holds the input that was read
 * @return a value of type @p I
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::io::ParseFailure if `is.eof()` returns `true` or if less than @p n characters were
 *     read
 */
template<typename I> requires Integer<I>
I
getHex(std::istream& is, size_t n, std::string& input) {
  size_t inputPos = tellg(is);
  input.clear();

  for (size_t i = 0; i < n; ++i) {
    char c = getChar(is, Symbols::HexDigits);
    input.push_back(c);
  }

  // Use type-specific `operator>>`
  auto localIs = io::is(input);
  I ret;
  localIs >> std::hex >> ret;
  if (localIs.fail() || tellg(localIs) != input.size()) {
    std::string msg;
    msg = message::cannotParseAs(input, Type::of<I>());
    throw ParseFailure(is, inputPos, { inputPos, inputPos + input.size() }, msg);
  }
  return ret;
}

/**
 * Reads an optional character matching @p v from the input stream @p is.
 *
 * @param is the input stream
 * @param v the expected character. This must be an ASCII character in the range [0,127]
 * @return @p v if the expected character @p v was read, otherwise null
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 */
std::optional<char> getOptionalChar(std::istream& is, char v);

/**
 * Reads an optional character contained in @p values from the input stream @p is.
 *
 * @param is the input stream
 * @param values expected characters. These must be ASCII characters in the range [0,127]
 * @return a character contained in @p values if such a character was read, otherwise null
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 */
std::optional<char> getOptionalChar(std::istream& is, const std::set<char>& values);

/**
 * Reads the string value @p v from the input stream @p is.
 *
 * @param is the input stream
 * @param v the expected string. May not be empty
 * @return @p v
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::io::ParseFailure if `is.eof()` returns `true` or if the read string is not @p v
 */
std::string_view getString(std::istream& is, std::string_view v);

/**
 * Reads a string value contained in @p values from the input stream @p is.
 *
 * @param is the input stream
 * @param values expected strings. May not contain an empty string
 * @return a string value contained in @p values
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::io::ParseFailure if `is.eof()` returns `true` or if a string contained in @p values
 *     cannot be read
 */
std::string getString(std::istream& is, const std::set<std::string_view>& values);

/**
 * Reads from the input stream @p is until the delimiter @p delimiter is read.
 *
 * At least @p min characters must be read for the function to succeed.
 *
 * @param is the input stream
 * @param delimiter the delimiter. This must be an ASCII character in the range [0,127]
 * @param consumeDelimiter if `true`, then the delimiter is consumed after the function successfully returns,
 *     otherwise the delimiter is put back to the input stream
 * @param min minimum amount of characters to be read
 * @return a string value. The delimiter ist not part of the result
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::io::ParseFailure if `is.eof()` returns `true`
 */
std::string getUntil(std::istream& is, char delimiter, bool consumeDelimiter, size_t min);

/**
 * Reads from the input stream @p is until the function @p delimiter returns `true`.
 *
 * At least @p min characters must be read for the function to succeed.
 *
 * @param is the input stream
 * @param delimiter unary function that returns `true` if its argument is considered a delimiter
 * @param delimiterDescription a description for @p delimiter, e.g. `"whitespace"`
 * @param consumeDelimiter if `true`, then the delimiter is consumed after the function successfully returns,
 *     otherwise the delimiter is put back to the input stream
 * @param min minimum amount of characters to be read
 * @return a string value. The delimiter ist not part of the result
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::io::ParseFailure if `is.eof()` returns `true`
 */
std::string
getUntil(
    std::istream& is,
    std::function<bool(char)> delimiter,
    std::string_view delimiterDescription,
    bool consumeDelimiter,
    size_t min);

/**
 * Reads from the input stream @p is while the read characters are contained in @p values.
 *
 * At least @p min characters must be read for the function to succeed.
 *
 * @param is the input stream
 * @param values expected characters. These must be ASCII characters in the range [0,127]
 * @param min minimum amount of characters to be read
 * @return a string value
 * @throw #rocket::io::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::io::ParseFailure if `is.eof()` returns `true` or if less than @p min characters were
 *     read
 */
std::string getWhile(std::istream& is, const std::set<char>& values, size_t min);

/**
 * Makes an input stream for a span.
 *
 * @param v the span
 * @param mode the open mode
 * @param exceptions the exception mask
 * @return an input stream
 */
std::ispanstream is(std::span<char> v, std::ios::openmode mode, std::ios::iostate exceptions);

/**
 * Makes an input stream for a C string.
 *
 * @param p a C string
 * @param mode the open mode
 * @param exceptions the exception mask
 * @return an input stream
 */
inline std::ispanstream
is(const char* p, std::ios::openmode mode, std::ios::iostate exceptions) {
  return is(std::span(const_cast<char*>(p), std::string_view(p).size()), mode, exceptions);
}

/**
 * Makes an input stream for a string.
 *
 * @param s a string
 * @param mode the open mode
 * @param exceptions the exception mask
 * @return an input stream
 */
inline std::ispanstream
is(const std::string& s, std::ios::openmode mode, std::ios::iostate exceptions) {
  return is(std::span(const_cast<char*>(s.c_str()), s.size()), mode, exceptions);
}

/**
 * Makes an input stream for a string view.
 *
 * @param s a string view
 * @param mode the open mode
 * @param exceptions the exception mask
 * @return an input stream
 */
inline std::ispanstream
is(std::string_view s, std::ios::openmode mode, std::ios::iostate exceptions) {
  return is(std::span(const_cast<char*>(s.data()), s.size()), mode, exceptions);
}

/**
 * Returns `true` if the stream @p ios is connected to a terminal.
 *
 * @param ios the stream
 * @return `true` if @p ios is connected to a terminal
 */
bool
inline isatty(const std::ios& ios) {
  return ::isatty(fd(ios));
}

/**
 * `resetg(is)` is equivalent to `seekg(is, 0)`.
 *
 * @param is the input stream
 * @return @p is
 */
inline std::istream&
resetg(std::istream& is) noexcept {
  return seekg(is, 0);
}

/**
 * Similar to `std::istream::seekg`, but clears `std::ios::eofbit` and `std::ios::failbit` in advance.
 *
 * Because the position can never be negative, a value of type `size_t` is expected.
 *
 * @param is the input stream
 * @param position the position to seek as a `size_t`
 * @throw #rocket::InvalidArgument if there is a position overflow
 * @throw std::ios::failure from `std::istream::seekg`
 * @return @p is
 */
std::istream& seekg(std::istream& is, size_t position);

/**
 * Similar to `std::istream::seekg`, but clears `std::ios::eofbit` and `std::ios::failbit` in advance.
 *
 * Because the position can never be negative, a value of type `size_t` is expected.
 *
 * @param is the input stream
 * @param position the position to seek as a `size_t`
 * @param dir the seek direction
 * @throw #rocket::InvalidArgument if there is a position overflow
 * @throw std::ios::failure from `std::istream::seekg`
 * @return @p is
 */
#if 0 // XXX
std::istream& seekg(std::istream& is, size_t position, std::ios::seekdir dir);
#endif

/**
 * Similar to `std::istream::tellg`, but leaves @p is unchanged and returns the actual current
 * position rather than -1 if `is.fail()` returns `true`.
 *
 * Because the result can never be negative, a value of type `size_t` is returned.
 *
 * @param is the input stream
 * @return the actual current position as a `size_t`
 */
size_t tellg(std::istream& is) noexcept;

} // namespace rocket::io

// `std` ----------------------------------------------------------------------------------------------------

namespace std {

/**
 * Appends the entire input of the input stream @p rhs to the string @p lhs.
 *
 * @param lhs the string to append to
 * @param rhs the input stream to read
 * @return lhs
 */
string& operator<<(string& lhs, const istream& rhs);

} // namespace std

// EOF
