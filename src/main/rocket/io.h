/**
 * @file io.h
 *
 * I/O utilities.
 */

#pragma once

#include "io-decl.h"

#include "Process.h"
#include "S.h"
#include "assert.h"
#include "basic.h"
#include "except.h"
#include "noun.h"
#include "scoped.h"
#include "unicode.h"
#include "unicode-iterator.h"

#include <memory>
#include <optional>
#include <set>
#include <spanstream>
#include <string>

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Makes a scope guard that automatically restores the stream's flags upon scope exit.
 *
 * @param stream the stream the flags of which are to restore upon scope exit
 */
#define ROCKET_IO_SCOPED_STREAM_FLAGS(stream) \
    const auto rocketIoScopedStreamFlags__ = stream.flags(); \
    ROCKET_SCOPED([&] { stream.flags(rocketIoScopedStreamFlags__); })

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

// `Symbols` ------------------------------------------------------------------------------------------------

template<typename C> requires Character<C>
struct Symbols;

/**
 * Predefined `char` symbols.
 */
template<>
struct Symbols<char> {
  /// Character sets.
  struct Chars {
    /**
     * Hexadecimal digits: <code>'0'</code> to <code>'9'</code>, <code>'a'</code> to <code>'f'</code>,
     * <code>'A'</code> to <code>'F'</code>.
     */
    static const std::set<char> HexDigits;
  };
};

/**
 * Predefined `char32_t` symbols.
 */
template<>
struct Symbols<char32_t> {
  /// Character sets.
  struct Chars {
    /**
     * Hexadecimal digits: <code>U'0'</code> to <code>>U'9'</code>, <code>U'a'</code> to <code>U'f'</code>,
     * <code>U'A'</code> to <code>U'F'</code>.
     */
    static const std::set<char32_t> HexDigits;
  };
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Checks the state of the input stream @p is.
 *
 * @tparam C the character type
 * @param is the input stream
 * @throw #rocket::except::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::except::ParseFailure if `is.eof()` returns `true`
 */
template<typename C> requires Character<C>
void
check(std::basic_istream<C>& is) {
  if (is.eof())
    throw except::ParseFailure<C>(is, tellg(is), "EOF");
  if (is.fail())
    throw except::InputFailure<C>(is);
}

/**
 * Reads a character from the input stream @p is.
 *
 * @tparam C the character type
 * @param is the input stream
 * @return a character
 */
template<typename C> requires Character<C>
C
getChar(std::basic_istream<C>& is) {
  C c(0);
  is.read(&c, 1);
  return c;
}

/**
 * Reads a character matching @p v from the input stream @p is.
 *
 * @tparam C the character type
 * @param is the input stream
 * @param v the expected character. This must be an ASCII character in the range [0,127]
 * @return @p v
 * @throw #rocket::except::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::except::ParseFailure if `is.eof()` returns `true` or if the read character is not @p v
 */
template<typename C> requires Character<C>
C
getChar(std::basic_istream<C>& is, C v) {
  ROCKET_CHECK(v, isascii(v));

  size_t inputPos = tellg(is);

  C c = getChar(is);
  if (is.eof())
    throw except::ParseFailure<C>(is, inputPos, S << "Expected " << v << ", got EOF");
  check(is);
  if (c != v)
    throw except::ParseFailure<C>(is, inputPos, S << "Expected " << v << ", got " << c);
  return c;
}

/**
 * Reads a character contained in @p values from the input stream @p is.
 *
 * @tparam C the character type
 * @param is the input stream
 * @param values expected characters. These must be ASCII characters in the range [0,127]
 * @return a character contained in @p values
 * @throw #rocket::except::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::except::ParseFailure if `is.eof()` returns` true` or if the read character is not
 *     contained in @p values
 */
template<typename C> requires Character<C>
C
getChar(std::basic_istream<C>& is, const std::set<C>& values) {
  size_t inputPos = tellg(is);

  C c = getChar(is);
  if (is.eof())
    throw except::ParseFailure<C>(is, inputPos, S << "Expected any of " << values << ", got EOF");
  check(is);
  if (not values.contains(c))
    throw except::ParseFailure<C>(is, inputPos, S << "Expected any of " << values << ", got " << c);
  return c;
}

/**
 * Reads @p n hexadecimal characters from the input stream @p is and converts them to an integer value.
 *
 * @tparam I the integer type
 * @tparam C the character type
 * @param is the input stream
 * @param n the number of hexadecimal characters to read
 * @param input after the function returns, @p input holds the input that was read
 * @return a value of type @p I
 * @throw #rocket::except::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::except::ParseFailure if `is.eof()` returns `true` or if less than @p n characters were
 *     read
 */
template<typename I, typename C> requires Integer<I> && Character<C>
I
getHex(std::basic_istream<C>& is, size_t n, std::basic_string<C>& input) {
  size_t inputPos = tellg(is);
  input.clear();

  for (size_t i = 0; i < n; ++i) {
    C c = getChar(is, Symbols<C>::Chars::HexDigits);
    input.push_back(c);
  }

  // Use type-specific `operator>>`
  auto localIs = io::is(input);
  I result;
  localIs >> std::hex >> result;
  if (localIs.fail() || tellg(localIs) != input.size()) {
    std::string msg;
    if constexpr (std::is_same_v<C, char>)
      msg = except::message::cannotParseAs(input, Type::of<I>());
    else
      msg = except::message::cannotParseAs(unicode::utf32To8(input), Type::of<I>());
   throw except::ParseFailure<C>(is, inputPos, { inputPos, inputPos + input.size() }, msg);
  }
  return result;
}

/**
 * Reads an optional character matching @p v from the input stream @p is.
 *
 * @tparam C the character type
 * @param is the input stream
 * @param v the expected character. This must be an ASCII character in the range [0,127]
 * @return @p v if the expected character @p v was read, otherwise null
 * @throw #rocket::except::InputFailure if `is.fail()` returns `true`
 */
template<typename C> requires Character<C>
std::optional<C>
getOptionalChar(std::basic_istream<C>& is, C v) {
  ROCKET_CHECK(v, isascii(v));

  size_t inputPos = tellg(is);

  C c = getChar(is);
  if (is.eof()) {
    seekg(is, inputPos);
    return std::nullopt;
  }
  check(is);
  
  if (c != v) {
    seekg(is, inputPos);
    return std::nullopt;
  } else
    return c;
}

/**
 * Reads an optional character contained in @p values from the input stream @p is.
 *
 * @tparam C the character type
 * @param is the input stream
 * @param values expected characters. These must be ASCII characters in the range [0,127]
 * @return a character contained in @p values if such a character was read, otherwise null
 * @throw #rocket::except::InputFailure if `is.fail()` returns `true`
 */
template<typename C> requires Character<C>
std::optional<C>
getOptionalChar(std::basic_istream<C>& is, const std::set<C>& values) {
  size_t inputPos = tellg(is);

  C c = getChar(is);
  if (is.eof()) {
    seekg(is, inputPos);
    return std::nullopt;
  }
  check(is);
  
  if (not values.contains(c)) {
    seekg(is, inputPos);
    return std::nullopt;
  } else
    return c;
}

/**
 * Reads the string value @p v from the input stream @p is.
 *
 * @tparam C the character type
 * @param is the input stream
 * @param v the expected string. May not be empty
 * @return @p v
 * @throw #rocket::except::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::except::ParseFailure if `is.eof()` returns `true` or if the read string is not @p v
 */
template<typename C> requires Character<C>
std::basic_string_view<C>
getString(std::basic_istream<C>& is, std::basic_string_view<C> v) {
  ROCKET_CHECK(v, not v.empty());

  size_t inputPos = tellg(is);

  auto it = unicode::CodePointIterator<C>(v), end = unicode::CodePointIterator<C>(v, v.size());
  for (; it != end; ++it) {
    // Read one code point
    size_t pos = io::tellg(is);
    unicode::CodePoint cp;
    is >> cp;
    if (is.eof()) {
      throw except::ParseFailure<C>(
          is, pos, { inputPos, pos },
          S << v.substr(0, pos - inputPos) << " does not match " << v << ", got EOF");
    }
    check(is);

    if (cp != *it) {
      throw except::ParseFailure<C>(
         is, pos, { inputPos, tellg(is) },
         S << v.substr(0, pos - inputPos) << " does not match " << v);
    }
  }
  return v;
}

/**
 * Reads a string value contained in @p values from the input stream @p is.
 *
 * @tparam C the character type
 * @param is the input stream
 * @param values expected strings. May not contain an empty string
 * @return a string value contained in @p values
 * @throw #rocket::except::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::except::ParseFailure if `is.eof()` returns `true` or if a string contained in @p values
 *     cannot be read
 */
template<typename C> requires Character<C>
std::basic_string<C>
getString(std::basic_istream<C>& is, const std::set<std::basic_string_view<C>>& values) {
  std::basic_string<C> result;

  auto localValues(values);

  size_t inputPos = tellg(is);
  std::basic_string<C> input; // Input so far
  
  while (true) {
    // Read one code point
    size_t pos = tellg(is);
    unicode::CodePoint cp;
    is >> cp;
    if (is.eof()) {
      if (not result.empty()) {
        seekg(is, pos);
        return result;
      } else {
        throw except::ParseFailure<C>(
            is, pos, { inputPos, tellg(is) },
            S << input << " does not match any of " << values << ", got EOF");
      }
    }
    check(is);

    // Advance by one code point
    input.append(static_cast<std::basic_string<C>>(cp));

    // Remove nonmatching values and fully matched value from set
    bool match = false;
    for (auto it = localValues.begin(), end = localValues.end(); it != end;) {
      auto value = *it;
      if (value.substr(0, input.size()) != input)
        it = localValues.erase(it);
      else if (value.size() == input.size()) {
        match = true;
        result = input;
        it = localValues.erase(it);
      } else {
       match = true;
       ++it;
      }
    }

    // Finished?
    if (localValues.empty()) {
      if (not result.empty()) {
        if (not match)
          seekg(is, pos);
        return result;
      } else {
        throw except::ParseFailure<C>(
            is, pos, { inputPos, tellg(is) },
            S << input << " does not match any of " << values);
      }
    }
  }
}

/**
 * Reads from the input stream @p is until the delimiter @p delimiter is read.
 *
 * At least @p min characters must be read for the function to succeed.
 *
 * @tparam C the character type
 * @param is the input stream
 * @param delimiter the delimiter. This must be an ASCII character in the range [0,127]
 * @param consumeDelimiter if `true`, then the delimiter is consumed after the function successfully returns,
 *     otherwise the delimiter is put back to the input stream
 * @param min minimum amount of characters to be read
 * @return a string value. The delimiter ist not part of the result
 * @throw #rocket::except::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::except::ParseFailure if `is.eof()` returns `true`
 */
template<typename C> requires Character<C>
std::basic_string<C>
getUntil(std::basic_istream<C>& is, C delimiter, bool consumeDelimiter, size_t min) {
  ROCKET_CHECK(delimiter, isascii(delimiter));

  size_t inputPos = tellg(is);
  std::basic_string<C> input;

  while (true) {
    size_t pos = tellg(is);
    C c = getChar(is);
    if (is.eof())
      throw except::ParseFailure<C>(is, pos, S << "Seeking " << delimiter << ", got EOF");
    check(is);

    if (c == delimiter) {
      if (not consumeDelimiter)
        is.putback(c);
      if (input.size() < min) {
        throw except::ParseFailure<C>(
            is, pos, { inputPos, inputPos + min },
            S << "Expected at least " << raw(noun::character(min)) << " before " << delimiter << ", got " << input.size());
      }
      return input;
    }
    input.push_back(c);
  }
}

/**
 * Reads from the input stream @p is until the function @p delimiter returns `true`.
 *
 * At least @p min characters must be read for the function to succeed.
 *
 * @tparam C the character type
 * @param is the input stream
 * @param delimiter unary function that returns `true` if its argument is considered a delimiter
 * @param delimiterDescription a description for @p delimiter, e.g. `"whitespace"`
 * @param consumeDelimiter if `true`, then the delimiter is consumed after the function successfully returns,
 *     otherwise the delimiter is put back to the input stream
 * @param min minimum amount of characters to be read
 * @return a string value. The delimiter ist not part of the result
 * @throw #rocket::except::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::except::ParseFailure if `is.eof()` returns `true`
 */
template<typename C> requires Character<C>
std::string
getUntil(
    std::basic_istream<C>& is,
    std::function<bool(C)> delimiter,
    std::basic_string_view<C> delimiterDescription,
    bool consumeDelimiter,
    size_t min) {
  size_t inputPos = tellg(is);
  std::basic_string<C> input;

  while (true) {
    size_t pos = tellg(is);
    C c = getChar(is);
    if (is.eof())
      throw except::ParseFailure<C>(is, pos, S << "Seeking " << raw(delimiterDescription) << ", got EOF");
    check(is);

    if (delimiter(c)) {
      if (not consumeDelimiter)
        is.putback(c);
      if (input.size() < min) {
        throw except::ParseFailure<C>(
            is, pos, { inputPos, inputPos + min },
            S << "Expected at least " << raw(noun::character(min)) << " before " << raw(delimiterDescription) << ", got " << input.size());
      }
      return input;
    }
    input.push_back(c);
  }
}

/**
 * Reads from the input stream @p is while the read characters are contained in @p values.
 *
 * At least @p min characters must be read for the function to succeed.
 *
 * @tparam C the character type
 * @param is the input stream
 * @param values expected characters. These must be ASCII characters in the range [0,127]
 * @param min minimum amount of characters to be read
 * @return a string value
 * @throw #rocket::except::InputFailure if `is.fail()` returns `true`
 * @throw #rocket::except::ParseFailure if `is.eof()` returns `true` or if less than @p min characters were
 *     read
 */
template<typename C> requires Character<C>
std::string
getWhile(std::basic_istream<C>& is, const std::set<C>& values, size_t min) {
  size_t inputPos = tellg(is);
  std::basic_string<C> input;
  
  while (true) {
    size_t pos = tellg(is);
    C c = getChar(is);
    if (is.eof()) {
      if (input.size() >= min) {
        seekg(is, pos);
        return input;
      } else {
        throw except::ParseFailure<C>(
            is, pos, { inputPos, inputPos + min },
            S << "Expected at least " << raw(noun::character(min)) << " contained in " << values << ", got " << input.size() << " and EOF");
      }
    }
    check(is);

    if (values.contains(c)) {
      input.push_back(c);
    } else {
      if (input.size() >= min) {
        seekg(is, pos);
        return input;
      } else {
          throw except::ParseFailure<C>(
              is, pos, { inputPos, inputPos + min },
              S << "Expected at least " << raw(noun::character(min)) << " contained in " << values << ", got " << input.size() << " and " << c);
      }
    }
  }
}

/**
 * Makes an input stream for a span.
 *
 * @tparam C the character type
 * @param v the span
 * @param mode the open mode
 * @param exceptions the exception mask
 * @return an input stream
 */
template<typename C> requires Character<C>
std::basic_ispanstream<C>
is(std::span<C> v, std::ios::openmode mode, std::ios::iostate exceptions) {
  auto result = std::basic_ispanstream<C>(v, mode);
  result.exceptions(exceptions);
  return result;
}

/**
 * Makes an input stream for a C string.
 *
 * @tparam C the character type
 * @param p a C string
 * @param mode the open mode
 * @param exceptions the exception mask
 * @return an input stream
 */
template<typename C> requires Character<C>
inline std::basic_ispanstream<C>
is(const C* p, std::ios::openmode mode, std::ios::iostate exceptions) {
  return is(std::span<C>(const_cast<C*>(p), std::basic_string_view<C>(p).size()), mode, exceptions);
}

/**
 * Makes an input stream for a string.
 *
 * @tparam C the character type
 * @param s a string
 * @param mode the open mode
 * @param exceptions the exception mask
 * @return an input stream
 */
template<typename C> requires Character<C>
inline std::basic_ispanstream<C>
is(const std::basic_string<C>& s, std::ios::openmode mode, std::ios::iostate exceptions) {
  return is(std::span<C>(const_cast<C*>(s.c_str()), s.size()), mode, exceptions);
}

/**
 * Makes an input stream for a string view.
 *
 * @tparam C the character type
 * @param s a string view
 * @param mode the open mode
 * @param exceptions the exception mask
 * @return an input stream
 */
template<typename C> requires Character<C>
inline std::basic_ispanstream<C>
is(std::basic_string_view<C> s, std::ios::openmode mode, std::ios::iostate exceptions) {
  return is(std::span<C>(const_cast<C*>(s.data()), s.size()), mode, exceptions);
}

/**
 * Returns `true` if the stream @p ios is connected to a terminal.
 *
 * @tparam C the character type
 * @param ios the stream
 * @return `true` if @p ios is connected to a terminal
 */
template<typename C> requires Character<C>
bool
isatty(const std::basic_ios<C>& ios) {
  if constexpr (std::is_same_v<C, char>) {
    if (&ios == &std::cout)
      return ::isatty(STDOUT_FILENO);
    if (&ios == &std::cerr)
      return ::isatty(STDERR_FILENO);
    if (&ios == &std::cin)
      return ::isatty(STDIN_FILENO);
  }
  return false;
}

/**
 * `resetg(is)` is equivalent to `seekg(is, 0)`.
 *
 * @tparam C the character type
 * @param is the input stream
 * @return @p is
 */
template<typename C> requires Character<C>
inline std::basic_istream<C>&
resetg(std::basic_istream<C>& is) noexcept {
  return seekg(is, 0);
}

/**
 * Similar to `std::istream::seekg`, but clears `std::ios::eofbit` and `std::ios::failbit` in advance.
 *
 * Because the position can never be negative, a value of type `size_t` is expected.
 *
 * @tparam C the character type
 * @param is the input stream
 * @param position the position to seek as a `size_t`
 * @throw #rocket::except::InvalidArgument if there is a position overflow
 * @throw std::ios::failure from `std::istream::seekg`
 * @return @p is
 */
template<typename C> requires Character<C>
std::basic_istream<C>&
seekg(std::basic_istream<C>& is, size_t position) {
  const auto state = is.rdstate();
  // Clear `eofbit` and `failbit`, leave `badbit` unchanged
  is.clear(state & ~(std::ios::eofbit | std::ios::failbit));
  typename std::basic_istream<C>::pos_type seekg = position;
  ROCKET_CHECK(position, seekg >= 0, except::message::overflow(Type::of(seekg)));
  // This might throw due to `badbit`, which is okay
  return is.seekg(seekg);
}

/**
 * Similar to `std::istream::seekg`, but clears `std::ios::eofbit` and `std::ios::failbit` in advance.
 *
 * Because the position can never be negative, a value of type `size_t` is expected.
 *
 * @tparam C the character type
 * @param is the input stream
 * @param position the position to seek as a `size_t`
 * @param dir the seek direction
 * @throw #rocket::except::InvalidArgument if there is a position overflow
 * @throw std::ios::failure from `std::istream::seekg`
 * @return @p is
 */
template<typename C> requires Character<C>
std::istream&
seekg(std::basic_istream<C>& is, size_t position, std::ios::seekdir dir) {
  const auto state = is.rdstate();
  // Clear `eofbit` and `failbit`, leave `badbit` unchanged
  is.clear(state & ~(std::ios::eofbit | std::ios::failbit));
  typename std::basic_istream<C>::pos_type seekg = position;
  ROCKET_CHECK(position, seekg >= 0, except::message::overflow(Type::of(seekg)));
  // This might throw due to `badbit`, which is okay
  return is.seekg(seekg, dir);
}

/**
 * Similar to `std::basic_istream<C>::tellg`, but leaves @p is unchanged and returns the actual current
 * position rather than -1 if `is.fail()` returns `true`.
 *
 * Because the result can never be negative, a value of type `size_t` is returned.
 *
 * @tparam C the character type
 * @param is the input stream
 * @return the actual current position as a `size_t`
 */
template<typename C> requires Character<C>
size_t
tellg(std::basic_istream<C>& is) noexcept {
  const auto state = is.rdstate();
  
  // Clear all bits
  is.clear();
  // This is expected to never throw, otherwise this implementation is flawed
  auto tellg = is.tellg();
  ROCKET_ASSERT(tellg >= 0);

  // Restore the state
  if ((is.exceptions() & state) == 0) {
    // Restore the state without exception
    is.clear(state);
  } else {
    // Restore the state with exception
    try {
      is.clear(state);
    } catch (const std::ios::failure&) {
      // Nothing to do, we want to catch this silently
    } catch (...) {
      ROCKET_PROCESS_ERROR("`is.clear()` failed");
    }
  }

  return tellg;
}

/**
 * Similar to `std::basic_ostream<C>::tellp`, but leaves @p os unchanged and returns the actual current
 * position rather than -1 if `os.fail()` returns `true`.
 *
 * Because the result can never be negative, a value of type `size_t` is returned.
 *
 * @tparam C the character type
 * @param os the output stream
 * @return the actual current position as a `size_t`
 */
template<typename C> requires Character<C>
size_t
tellp(std::basic_ostream<C>& os) noexcept {
  const auto state = os.rdstate();
  
  // Clear all bits
  os.clear();
  // This is expected to never throw, otherwise this implementation is flawed
  auto tellp = os.tellp();
  ROCKET_ASSERT(tellp >= 0);

  // Restore the state
  if ((os.exceptions() & state) == 0) {
    // Restore the state without exception
    os.clear(state);
  } else {
    // Restore the state with exception
    try {
      os.clear(state);
    } catch (const std::ios::failure&) {
      // Nothing to do, we want to catch this silently
    } catch (...) {
      ROCKET_PROCESS_ERROR("`os.clear()` failed");
    }
  }

  return tellp;
}

} // namespace rocket::io

// `std` ----------------------------------------------------------------------------------------------------

namespace std {

/**
 * Appends the entire input of the input stream @p rhs to the string @p lhs.
 *
 * @tparam C the character type
 * @param lhs the string to append to
 * @param rhs the input stream to read
 * @return lhs
 */
template<typename C> requires rocket::Character<C>
basic_string<C>&
operator<<(std::basic_string<C>& lhs, const basic_istream<C>& rhs) {
  basic_ostringstream<C> os;
  os << rhs.rdbuf();
  lhs.append(os.str());
  return lhs;
}

} // namespace std

// EOF
