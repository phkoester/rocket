/**
 * @file SpanStream.h
 *
 * Streams reading from and writing to a fixed buffer, given as a #std::span.
 *
 * If the standard library provides `<spanstream>` (C++23), #rocket::io::BasicSpanBuf,
 * #rocket::io::BasicISpanStream, #rocket::io::BasicOSpanStream, and #rocket::io::BasicSpanStream are
 * #std::basic_spanbuf, #std::basic_ispanstream, #std::basic_ospanstream, and #std::basic_spanstream.
 * Otherwise, they are portable implementations that follow the specification of these classes in the C++23
 * standard.
 */

#pragma once

#if defined(ROCKET_OS_MACOS) && defined(ROCKET_CXX_COMPILER_LLVM)
  #define ROCKET_HAS_SPAN_STREAM_IMPL
#endif

#include "rocket/rocket.h"

#ifndef ROCKET_HAS_SPAN_STREAM_IMPL
  #include <spanstream>
#else
  #include <climits>
  #include <concepts>
  #include <cstddef>
  #include <ios>
  #include <istream>
  #include <memory>
  #include <ostream>
  #include <ranges>
  #include <span>
  #include <streambuf>
  #include <string>
  #include <utility>
#endif

namespace rocket::io {

#ifndef ROCKET_HAS_SPAN_STREAM_IMPL

// `BasicSpanBuf`, `BasicISpanStream`, `BasicOSpanStream`, `BasicSpanStream` --------------------------------

/// A stream buffer for a fixed character buffer.
template<typename CharT, typename Traits = std::char_traits<CharT>>
using BasicSpanBuf = std::basic_spanbuf<CharT, Traits>;

/// A stream for reading from a fixed character buffer.
template<typename CharT, typename Traits = std::char_traits<CharT>>
using BasicISpanStream = std::basic_ispanstream<CharT, Traits>;

/// A stream for writing to a fixed character buffer.
template<typename CharT, typename Traits = std::char_traits<CharT>>
using BasicOSpanStream = std::basic_ospanstream<CharT, Traits>;

/// A stream for reading from and writing to a fixed character buffer.
template<typename CharT, typename Traits = std::char_traits<CharT>>
using BasicSpanStream = std::basic_spanstream<CharT, Traits>;

#else

// `BasicSpanBuf` -------------------------------------------------------------------------------------------

/**
 * A stream buffer for a fixed character buffer, equivalent to C++23's #std::basic_spanbuf.
 *
 * The buffer is never reallocated. Writing past its end fails, as does reading past its end.
 *
 * @tparam CharT the character type
 * @tparam Traits the character traits
 *
 * @NotThreadSafe
 */
template<typename CharT, typename Traits = std::char_traits<CharT>>
struct BasicSpanBuf : public std::basic_streambuf<CharT, Traits> {
  using Base = std::basic_streambuf<CharT, Traits>;

  using char_type = CharT;
  using int_type = Traits::int_type;
  using pos_type = Traits::pos_type;
  using off_type = Traits::off_type;
  using traits_type = Traits;

  /// @ctor
  BasicSpanBuf() : BasicSpanBuf(std::ios_base::in | std::ios_base::out) {}

  /**
   * @ctor
   *
   * @param which the open mode
   */
  explicit BasicSpanBuf(std::ios_base::openmode which) : BasicSpanBuf(std::span<CharT>(), which) {}

  /**
   * @ctor
   *
   * @param s the buffer
   * @param which the open mode
   */
  explicit BasicSpanBuf(
    std::span<CharT> s,
    std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) :
    mode_(which) {
    this->span(s);
  }

  BasicSpanBuf(const BasicSpanBuf&) = delete;

  /**
   * @ctor_move
   *
   * The sequence pointers of the new instance are those of @p rhs, which keeps referring to the same buffer.
   */
  BasicSpanBuf(BasicSpanBuf&& rhs) : Base(rhs), mode_(rhs.mode_), buf_(rhs.buf_) {}

  ~BasicSpanBuf() override = default;

  BasicSpanBuf& operator=(const BasicSpanBuf&) = delete;

  /// @op_assign_move
  BasicSpanBuf&
  operator=(BasicSpanBuf&& rhs) {
    BasicSpanBuf tmp(std::move(rhs));
    swap(tmp);
    return *this;
  }

  /**
   * Returns the buffer.
   *
   * @return if opened for output, the part of the buffer written so far, otherwise the whole buffer
   */
  [[nodiscard]] std::span<CharT>
  span() const noexcept {
    if (mode_ & std::ios_base::out) {
      return std::span<CharT>(this->pbase(), this->pptr());
    }
    return buf_;
  }

  /**
   * Sets a new buffer, resetting the input and output positions.
   *
   * If opened with `std::ios_base::ate`, the output position is set to the end of the buffer.
   *
   * @param s the buffer
   */
  void
  span(std::span<CharT> s) noexcept {
    buf_ = s;
    if (mode_ & std::ios_base::out) {
      this->setp(s.data(), s.data() + s.size());
      if (mode_ & std::ios_base::ate) {
        bumpOutput(s.size());
      }
    }
    if (mode_ & std::ios_base::in) {
      this->setg(s.data(), s.data(), s.data() + s.size());
    }
  }

  /**
   * Swaps the state of this instance with that of @p rhs.
   *
   * @param rhs the other instance
   */
  void
  swap(BasicSpanBuf& rhs) {
    Base::swap(rhs);
    std::swap(mode_, rhs.mode_);
    std::swap(buf_, rhs.buf_);
  }

protected:

  /// @cond undocumented

  Base*
  setbuf(CharT* s, std::streamsize n) override {
    this->span(std::span<CharT>(s, static_cast<std::size_t>(n)));
    return this;
  }

  pos_type
  seekoff(
    off_type off,
    std::ios_base::seekdir way,
    std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override {
    const auto fail = pos_type(off_type(-1));
    const bool in = (which & std::ios_base::in) != 0;
    const bool out = (which & std::ios_base::out) != 0;

    // Positioning both sequences relative to the current position is ambiguous

    if ((!in && !out) || (in && out && way == std::ios_base::cur)) {
      return fail;
    }

    // Determine the base offset

    off_type baseval;
    if (way == std::ios_base::beg) {
      baseval = 0;
    } else if (way == std::ios_base::cur) {
      baseval = in ? this->gptr() - this->eback() : this->pptr() - this->pbase();
    } else if (way == std::ios_base::end) {
      const bool outOnly = (mode_ & std::ios_base::out) && !(mode_ & std::ios_base::in);
      baseval = outOnly ? this->pptr() - this->pbase() : static_cast<off_type>(buf_.size());
    } else {
      return fail;
    }

    // The new offset must lie within the buffer. Checking it this way cannot overflow, since `baseval` lies
    // within [0,`size`]

    const auto size = static_cast<off_type>(buf_.size());
    if (off < -baseval || off > size - baseval) {
      return fail;
    }
    const off_type newoff = baseval + off;

    // A sequence that has no buffer can only be positioned at 0

    if (newoff != 0 && ((in && this->gptr() == nullptr) || (out && this->pptr() == nullptr))) {
      return fail;
    }

    if (in) {
      this->setg(this->eback(), this->eback() + newoff, this->egptr());
    }
    if (out) {
      this->setp(this->pbase(), this->epptr());
      bumpOutput(static_cast<std::size_t>(newoff));
    }
    return pos_type(newoff);
  }

  pos_type
  seekpos(pos_type sp, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override {
    return seekoff(off_type(sp), std::ios_base::beg, which);
  }

  /// @endcond

private:

  std::ios_base::openmode mode_;
  std::span<CharT> buf_;

  // Advances the output position by @p n. `pbump` only takes an `int`, so large buffers need several steps
  void
  bumpOutput(std::size_t n) {
    while (n > static_cast<std::size_t>(INT_MAX)) {
      this->pbump(INT_MAX);
      n -= static_cast<std::size_t>(INT_MAX);
    }
    this->pbump(static_cast<int>(n));
  }
};

/**
 * Swaps the states of two span buffers.
 *
 * @param lhs the first buffer
 * @param rhs the second buffer
 */
template<typename CharT, typename Traits>
void
swap(BasicSpanBuf<CharT, Traits>& lhs, BasicSpanBuf<CharT, Traits>& rhs) {
  lhs.swap(rhs);
}

// `BasicISpanStream` ---------------------------------------------------------------------------------------

namespace internal {

// A borrowed range that converts to a span of `const` characters, but not to a span of mutable ones
template<typename ROS, typename CharT>
concept ReadOnlySpan =
  std::ranges::borrowed_range<ROS> &&
  not std::convertible_to<ROS, std::span<CharT>> &&
  std::convertible_to<ROS, std::span<const CharT>>;

} // namespace internal

/**
 * A stream for reading from a fixed character buffer, equivalent to C++23's #std::basic_ispanstream.
 *
 * @tparam CharT the character type
 * @tparam Traits the character traits
 *
 * @NotThreadSafe
 */
template<typename CharT, typename Traits = std::char_traits<CharT>>
struct BasicISpanStream : public std::basic_istream<CharT, Traits> {
  using Base = std::basic_istream<CharT, Traits>;

  using char_type = CharT;
  using int_type = Traits::int_type;
  using pos_type = Traits::pos_type;
  using off_type = Traits::off_type;
  using traits_type = Traits;

  /**
   * @ctor
   *
   * @param s the buffer
   * @param which the open mode; `std::ios_base::in` is always added
   */
  explicit BasicISpanStream(std::span<CharT> s, std::ios_base::openmode which = std::ios_base::in) :
    Base(std::addressof(sb_)), // Only stores the pointer; `sb_` is constructed next
    sb_(s, which | std::ios_base::in) {}

  /**
   * @ctor
   *
   * Reads from a read-only range, such as a #std::string_view or a #std::span of `const` characters. The
   * range is never written to.
   *
   * @tparam ROS the range type
   * @param s the range
   */
  template<typename ROS> requires internal::ReadOnlySpan<ROS, CharT>
  explicit BasicISpanStream(ROS&& s) : BasicISpanStream(unconst(std::forward<ROS>(s))) {}

  BasicISpanStream(const BasicISpanStream&) = delete;

  /// @ctor_move
  BasicISpanStream(BasicISpanStream&& rhs) : Base(std::move(rhs)), sb_(std::move(rhs.sb_)) {
    Base::set_rdbuf(std::addressof(sb_));
  }

  ~BasicISpanStream() override = default;

  BasicISpanStream& operator=(const BasicISpanStream&) = delete;

  /// @op_assign_move
  BasicISpanStream& operator=(BasicISpanStream&& rhs) = default;

  /**
   * Returns the stream buffer.
   *
   * @return the stream buffer
   */
  [[nodiscard]] BasicSpanBuf<CharT, Traits>*
  rdbuf() const noexcept {
    return const_cast<BasicSpanBuf<CharT, Traits>*>(std::addressof(sb_)); // NOLINT(*-const-cast)
  }

  /**
   * Returns the buffer.
   *
   * @return the buffer
   */
  [[nodiscard]] std::span<const CharT> span() const noexcept { return rdbuf()->span(); }

  /**
   * Sets a new buffer, resetting the input position.
   *
   * @param s the buffer
   */
  void span(std::span<CharT> s) noexcept { rdbuf()->span(s); }

  /**
   * Sets a new read-only buffer, resetting the input position.
   *
   * @tparam ROS the range type
   * @param s the range
   */
  template<typename ROS> requires internal::ReadOnlySpan<ROS, CharT>
  void
  span(ROS&& s) noexcept {
    rdbuf()->span(unconst(std::forward<ROS>(s)));
  }

  /**
   * Swaps the state of this instance with that of @p rhs.
   *
   * @param rhs the other instance
   */
  void
  swap(BasicISpanStream& rhs) {
    Base::swap(rhs);
    sb_.swap(rhs.sb_);
  }

private:

  // The stream buffer needs a mutable span, but an input stream never writes to it
  template<typename ROS>
  static std::span<CharT>
  unconst(ROS&& s) noexcept {
    const std::span<const CharT> cs(std::forward<ROS>(s));
    return std::span<CharT>(const_cast<CharT*>(cs.data()), cs.size()); // NOLINT(*-const-cast)
  }

  BasicSpanBuf<CharT, Traits> sb_;
};

/**
 * Swaps the states of two input span streams.
 *
 * @param lhs the first stream
 * @param rhs the second stream
 */
template<typename CharT, typename Traits>
void
swap(BasicISpanStream<CharT, Traits>& lhs, BasicISpanStream<CharT, Traits>& rhs) {
  lhs.swap(rhs);
}

// `BasicOSpanStream` ---------------------------------------------------------------------------------------

/**
 * A stream for writing to a fixed character buffer, equivalent to C++23's #std::basic_ospanstream.
 *
 * @tparam CharT the character type
 * @tparam Traits the character traits
 *
 * @NotThreadSafe
 */
template<typename CharT, typename Traits = std::char_traits<CharT>>
struct BasicOSpanStream : public std::basic_ostream<CharT, Traits> {
  using Base = std::basic_ostream<CharT, Traits>;

  using char_type = CharT;
  using int_type = Traits::int_type;
  using pos_type = Traits::pos_type;
  using off_type = Traits::off_type;
  using traits_type = Traits;

  /**
   * @ctor
   *
   * @param s the buffer
   * @param which the open mode; `std::ios_base::out` is always added
   */
  explicit BasicOSpanStream(std::span<CharT> s, std::ios_base::openmode which = std::ios_base::out) :
    Base(std::addressof(sb_)), // Only stores the pointer; `sb_` is constructed next
    sb_(s, which | std::ios_base::out) {}

  BasicOSpanStream(const BasicOSpanStream&) = delete;

  /// @ctor_move
  BasicOSpanStream(BasicOSpanStream&& rhs) : Base(std::move(rhs)), sb_(std::move(rhs.sb_)) {
    Base::set_rdbuf(std::addressof(sb_));
  }

  ~BasicOSpanStream() override = default;

  BasicOSpanStream& operator=(const BasicOSpanStream&) = delete;

  /// @op_assign_move
  BasicOSpanStream& operator=(BasicOSpanStream&& rhs) = default;

  /**
   * Returns the stream buffer.
   *
   * @return the stream buffer
   */
  [[nodiscard]] BasicSpanBuf<CharT, Traits>*
  rdbuf() const noexcept {
    return const_cast<BasicSpanBuf<CharT, Traits>*>(std::addressof(sb_)); // NOLINT(*-const-cast)
  }

  /**
   * Returns the buffer.
   *
   * @return the part of the buffer written so far
   */
  [[nodiscard]] std::span<CharT> span() const noexcept { return rdbuf()->span(); }

  /**
   * Sets a new buffer, resetting the output position.
   *
   * @param s the buffer
   */
  void span(std::span<CharT> s) noexcept { rdbuf()->span(s); }

  /**
   * Swaps the state of this instance with that of @p rhs.
   *
   * @param rhs the other instance
   */
  void
  swap(BasicOSpanStream& rhs) {
    Base::swap(rhs);
    sb_.swap(rhs.sb_);
  }

private:

  BasicSpanBuf<CharT, Traits> sb_;
};

/**
 * Swaps the states of two output span streams.
 *
 * @param lhs the first stream
 * @param rhs the second stream
 */
template<typename CharT, typename Traits>
void
swap(BasicOSpanStream<CharT, Traits>& lhs, BasicOSpanStream<CharT, Traits>& rhs) {
  lhs.swap(rhs);
}

// `BasicSpanStream` ----------------------------------------------------------------------------------------

/**
 * A stream for reading from and writing to a fixed character buffer, equivalent to C++23's
 * #std::basic_spanstream.
 *
 * @tparam CharT the character type
 * @tparam Traits the character traits
 *
 * @NotThreadSafe
 */
template<typename CharT, typename Traits = std::char_traits<CharT>>
struct BasicSpanStream : public std::basic_iostream<CharT, Traits> {
  using Base = std::basic_iostream<CharT, Traits>;

  using char_type = CharT;
  using int_type = Traits::int_type;
  using pos_type = Traits::pos_type;
  using off_type = Traits::off_type;
  using traits_type = Traits;

  /**
   * @ctor
   *
   * @param s the buffer
   * @param which the open mode
   */
  explicit BasicSpanStream(
    std::span<CharT> s,
    std::ios_base::openmode which = std::ios_base::out | std::ios_base::in) :
    Base(std::addressof(sb_)), // Only stores the pointer; `sb_` is constructed next
    sb_(s, which) {}

  BasicSpanStream(const BasicSpanStream&) = delete;

  /// @ctor_move
  BasicSpanStream(BasicSpanStream&& rhs) : Base(std::move(rhs)), sb_(std::move(rhs.sb_)) {
    Base::set_rdbuf(std::addressof(sb_));
  }

  ~BasicSpanStream() override = default;

  BasicSpanStream& operator=(const BasicSpanStream&) = delete;

  /// @op_assign_move
  BasicSpanStream& operator=(BasicSpanStream&& rhs) = default;

  /**
   * Returns the stream buffer.
   *
   * @return the stream buffer
   */
  [[nodiscard]] BasicSpanBuf<CharT, Traits>*
  rdbuf() const noexcept {
    return const_cast<BasicSpanBuf<CharT, Traits>*>(std::addressof(sb_)); // NOLINT(*-const-cast)
  }

  /**
   * Returns the buffer.
   *
   * @return the part of the buffer written so far if opened for output, otherwise the whole buffer
   */
  [[nodiscard]] std::span<CharT> span() const noexcept { return rdbuf()->span(); }

  /**
   * Sets a new buffer, resetting the input and output positions.
   *
   * @param s the buffer
   */
  void span(std::span<CharT> s) noexcept { rdbuf()->span(s); }

  /**
   * Swaps the state of this instance with that of @p rhs.
   *
   * @param rhs the other instance
   */
  void
  swap(BasicSpanStream& rhs) {
    Base::swap(rhs);
    sb_.swap(rhs.sb_);
  }

private:

  BasicSpanBuf<CharT, Traits> sb_;
};

/**
 * Swaps the states of two span streams.
 *
 * @param lhs the first stream
 * @param rhs the second stream
 */
template<typename CharT, typename Traits>
void
swap(BasicSpanStream<CharT, Traits>& lhs, BasicSpanStream<CharT, Traits>& rhs) {
  lhs.swap(rhs);
}

#endif // ROCKET_HAS_ROCKET_SPAN_STREAM

// `SpanBuf`, `ISpanStream`, `OSpanStream`, `SpanStream` -----------------------------------------------------

/// A stream buffer for a fixed `char` buffer.
using SpanBuf = BasicSpanBuf<char>;

/// A stream for reading from a fixed `char` buffer.
using ISpanStream = BasicISpanStream<char>;

/// A stream for writing to a fixed `char` buffer.
using OSpanStream = BasicOSpanStream<char>;

/// A stream for reading from and writing to a fixed `char` buffer.
using SpanStream = BasicSpanStream<char>;

} // namespace rocket::io

// EOF
