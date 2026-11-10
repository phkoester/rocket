/**
 * @file spanstream.h
 *
 * Streams reading from and writing to a fixed buffer, given as a #std::span.
 *
 * If the standard library provides `<spanstream>` (C++23), #rocket::io::basic_spanbuf,
 * #rocket::io::basic_ispanstream, #rocket::io::basic_ospanstream, and #rocket::io::basic_spanstream are
 * `std::basic_spanbuf`, `std::basic_ispanstream`, `std::basic_ospanstream`, and `std::basic_spanstream`.
 * Otherwise, they are portable implementations that follow the specification of these classes in the C++23
 * standard.
 */

#pragma once

#include <version>

#if defined(ROCKET_OS_MACOS) && defined(ROCKET_CXX_COMPILER_LLVM)
  #ifdef __cpp_lib_spanstream
    #error This C++ version supports span streams
  #endif
#else
  #ifndef __cpp_lib_spanstream
    #error This C++ version does not support span streams
  #endif
  #define ROCKET_HAS_STD_SPANSTREAM
#endif

#include "rocket/rocket.h"

#ifdef ROCKET_HAS_STD_SPANSTREAM
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

#ifdef ROCKET_HAS_STD_SPANSTREAM

// `basic_spanbuf`, `basic_ispanstream`, `basic_ospanstream`, `basic_spanstream` ----------------------------

/// A stream buffer for a fixed character buffer.
template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_spanbuf = std::basic_spanbuf<CharT, Traits>;

/// A stream for reading from a fixed character buffer.
template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_ispanstream = std::basic_ispanstream<CharT, Traits>;

/// A stream for writing to a fixed character buffer.
template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_ospanstream = std::basic_ospanstream<CharT, Traits>;

/// A stream for reading from and writing to a fixed character buffer.
template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_spanstream = std::basic_spanstream<CharT, Traits>;

#else

// `basic_spanbuf` ------------------------------------------------------------------------------------------

/**
 * A stream buffer for a fixed character buffer, equivalent to C++23’s `std::basic_spanbuf`.
 *
 * The buffer is never reallocated. Writing past its end fails, as does reading past its end.
 *
 * @tparam CharT the character type
 * @tparam Traits the character traits
 *
 * @NotThreadSafe
 */
template<typename CharT, typename Traits = std::char_traits<CharT>>
struct basic_spanbuf : public std::basic_streambuf<CharT, Traits> {
  /// @type_base
  using Base = std::basic_streambuf<CharT, Traits>;

  /// @type_alias
  using char_type = CharT;
  /// @type_alias
  using int_type = Traits::int_type;
  /// @type_alias
  using pos_type = Traits::pos_type;
  /// @type_alias
  using off_type = Traits::off_type;
  /// @type_alias
  using traits_type = Traits;

  /// @ctor
  basic_spanbuf() : basic_spanbuf(std::ios_base::in | std::ios_base::out) {}

  /**
   * @ctor
   *
   * @param which the open mode
   */
  explicit basic_spanbuf(std::ios_base::openmode which) : basic_spanbuf(std::span<CharT>(), which) {}

  /**
   * @ctor
   *
   * @param s the buffer
   * @param which the open mode
   */
  explicit basic_spanbuf(
    std::span<CharT> s,
    std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) :
    mode_(which) {
    this->span(s);
  }

  basic_spanbuf(const basic_spanbuf&) = delete;

  /**
   * @ctor_move
   *
   * The sequence pointers of the new instance are those of @p rhs, which keeps referring to the same buffer.
   */
  basic_spanbuf(basic_spanbuf&& rhs) : Base(rhs), mode_(rhs.mode_), buf_(rhs.buf_) {}

  ~basic_spanbuf() override = default;

  basic_spanbuf& operator=(const basic_spanbuf&) = delete;

  /// @member_op_asgmt_move
  basic_spanbuf&
  operator=(basic_spanbuf&& rhs) {
    basic_spanbuf tmp(std::move(rhs));
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
        bump_output(s.size());
      }
    }
    if (mode_ & std::ios_base::in) {
      this->setg(s.data(), s.data(), s.data() + s.size());
    }
  }

  /// @member_fn_swap
  void
  swap(basic_spanbuf& rhs) {
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
      bump_output(static_cast<std::size_t>(newoff));
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
  bump_output(std::size_t n) {
    while (n > static_cast<std::size_t>(INT_MAX)) {
      this->pbump(INT_MAX);
      n -= static_cast<std::size_t>(INT_MAX);
    }
    this->pbump(static_cast<int>(n));
  }
};

/// @fn_swap{rocket::io::basic_spanbuf}
template<typename CharT, typename Traits>
void
swap(basic_spanbuf<CharT, Traits>& lhs, basic_spanbuf<CharT, Traits>& rhs) {
  lhs.swap(rhs);
}

// `basic_ispanstream` --------------------------------------------------------------------------------------

namespace internal {

// A borrowed range that converts to a span of `const` characters, but not to a span of mutable ones
template<typename ROS, typename CharT>
concept ReadOnlySpan =
  std::ranges::borrowed_range<ROS> &&
  not std::convertible_to<ROS, std::span<CharT>> &&
  std::convertible_to<ROS, std::span<const CharT>>;

} // namespace internal

/**
 * A stream for reading from a fixed character buffer, equivalent to C++23’s `std::basic_ispanstream`.
 *
 * @tparam CharT the character type
 * @tparam Traits the character traits
 *
 * @NotThreadSafe
 */
template<typename CharT, typename Traits = std::char_traits<CharT>>
struct basic_ispanstream : public std::basic_istream<CharT, Traits> {
  /// @type_base
  using Base = std::basic_istream<CharT, Traits>;

  /// @type_alias
  using char_type = CharT;
  /// @type_alias
  using int_type = Traits::int_type;
  /// @type_alias
  using pos_type = Traits::pos_type;
  /// @type_alias
  using off_type = Traits::off_type;
  /// @type_alias
  using traits_type = Traits;

  /**
   * @ctor
   *
   * @param s the buffer
   * @param which the open mode; `std::ios_base::in` is always added
   */
  explicit basic_ispanstream(std::span<CharT> s, std::ios_base::openmode which = std::ios_base::in) :
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
  explicit basic_ispanstream(ROS&& s) : basic_ispanstream(unconst(std::forward<ROS>(s))) {}

  basic_ispanstream(const basic_ispanstream&) = delete;

  /// @ctor_move
  basic_ispanstream(basic_ispanstream&& rhs) : Base(std::move(rhs)), sb_(std::move(rhs.sb_)) {
    Base::set_rdbuf(std::addressof(sb_));
  }

  ~basic_ispanstream() override = default;

  basic_ispanstream& operator=(const basic_ispanstream&) = delete;

  /// @member_op_asgmt_move
  basic_ispanstream& operator=(basic_ispanstream&& rhs) = default;

  /**
   * Returns the stream buffer.
   *
   * @return the stream buffer
   */
  [[nodiscard]] basic_spanbuf<CharT, Traits>*
  rdbuf() const noexcept {
    return const_cast<basic_spanbuf<CharT, Traits>*>(std::addressof(sb_));
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

  /// @member_fn_swap
  void
  swap(basic_ispanstream& rhs) {
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

  basic_spanbuf<CharT, Traits> sb_;
};

/// @fn_swap{rocket::io::basic_ispanstream}
template<typename CharT, typename Traits>
void
swap(basic_ispanstream<CharT, Traits>& lhs, basic_ispanstream<CharT, Traits>& rhs) {
  lhs.swap(rhs);
}

// `basic_ospanstream` --------------------------------------------------------------------------------------

/**
 * A stream for writing to a fixed character buffer, equivalent to C++23’s `std::basic_ospanstream`.
 *
 * @tparam CharT the character type
 * @tparam Traits the character traits
 *
 * @NotThreadSafe
 */
template<typename CharT, typename Traits = std::char_traits<CharT>>
struct basic_ospanstream : public std::basic_ostream<CharT, Traits> {
  /// @type_base
  using Base = std::basic_ostream<CharT, Traits>;

  /// @type_alias
  using char_type = CharT;
  /// @type_alias
  using int_type = Traits::int_type;
  /// @type_alias
  using pos_type = Traits::pos_type;
  /// @type_alias
  using off_type = Traits::off_type;
  /// @type_alias
  using traits_type = Traits;

  /**
   * @ctor
   *
   * @param s the buffer
   * @param which the open mode; `std::ios_base::out` is always added
   */
  explicit basic_ospanstream(std::span<CharT> s, std::ios_base::openmode which = std::ios_base::out) :
    Base(std::addressof(sb_)), // Only stores the pointer; `sb_` is constructed next
    sb_(s, which | std::ios_base::out) {}

  basic_ospanstream(const basic_ospanstream&) = delete;

  /// @ctor_move
  basic_ospanstream(basic_ospanstream&& rhs) : Base(std::move(rhs)), sb_(std::move(rhs.sb_)) {
    Base::set_rdbuf(std::addressof(sb_));
  }

  ~basic_ospanstream() override = default;

  basic_ospanstream& operator=(const basic_ospanstream&) = delete;

  /// @member_op_asgmt_move
  basic_ospanstream& operator=(basic_ospanstream&& rhs) = default;

  /**
   * Returns the stream buffer.
   *
   * @return the stream buffer
   */
  [[nodiscard]] basic_spanbuf<CharT, Traits>*
  rdbuf() const noexcept {
    return const_cast<basic_spanbuf<CharT, Traits>*>(std::addressof(sb_));
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


  /// @member_fn_swap
  void
  swap(basic_ospanstream& rhs) {
    Base::swap(rhs);
    sb_.swap(rhs.sb_);
  }

private:

  basic_spanbuf<CharT, Traits> sb_;
};

/// @fn_swap{rocket::io::basic_ospanstream}
template<typename CharT, typename Traits>
void
swap(basic_ospanstream<CharT, Traits>& lhs, basic_ospanstream<CharT, Traits>& rhs) {
  lhs.swap(rhs);
}

// `basic_spanstream` ---------------------------------------------------------------------------------------

/**
 * A stream for reading from and writing to a fixed character buffer, equivalent to C++23’s
 * `std::basic_spanstream`.
 *
 * @tparam CharT the character type
 * @tparam Traits the character traits
 *
 * @NotThreadSafe
 */
template<typename CharT, typename Traits = std::char_traits<CharT>>
struct basic_spanstream : public std::basic_iostream<CharT, Traits> {
  /// @type_base
  using Base = std::basic_iostream<CharT, Traits>;

  /// @type_alias
  using char_type = CharT;
  /// @type_alias
  using int_type = Traits::int_type;
  /// @type_alias
  using pos_type = Traits::pos_type;
  /// @type_alias
  using off_type = Traits::off_type;
  /// @type_alias
  using traits_type = Traits;

  /**
   * @ctor
   *
   * @param s the buffer
   * @param which the open mode
   */
  explicit basic_spanstream(
    std::span<CharT> s,
    std::ios_base::openmode which = std::ios_base::out | std::ios_base::in) :
    Base(std::addressof(sb_)), // Only stores the pointer; `sb_` is constructed next
    sb_(s, which) {}

  basic_spanstream(const basic_spanstream&) = delete;

  /// @ctor_move
  basic_spanstream(basic_spanstream&& rhs) : Base(std::move(rhs)), sb_(std::move(rhs.sb_)) {
    Base::set_rdbuf(std::addressof(sb_));
  }

  ~basic_spanstream() override = default;

  basic_spanstream& operator=(const basic_spanstream&) = delete;

  /// @member_op_asgmt_move
  basic_spanstream& operator=(basic_spanstream&& rhs) = default;

  /**
   * Returns the stream buffer.
   *
   * @return the stream buffer
   */
  [[nodiscard]] basic_spanbuf<CharT, Traits>*
  rdbuf() const noexcept {
    return const_cast<basic_spanbuf<CharT, Traits>*>(std::addressof(sb_));
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


  /// @member_fn_swap
  void
  swap(basic_spanstream& rhs) {
    Base::swap(rhs);
    sb_.swap(rhs.sb_);
  }

private:

  basic_spanbuf<CharT, Traits> sb_;
};

/// @fn_swap{rocket::io::basic_spanstream}
template<typename CharT, typename Traits>
void
swap(basic_spanstream<CharT, Traits>& lhs, basic_spanstream<CharT, Traits>& rhs) {
  lhs.swap(rhs);
}

#endif // ROCKET_HAS_STD_SPANSTREAM

// `spanbuf`, `ispanstream`, `ospanstream`, `spanstream` ----------------------------------------------------

/// A stream buffer for a fixed `char` buffer.
using spanbuf = basic_spanbuf<char>;

/// A stream for reading from a fixed `char` buffer.
using ispanstream = basic_ispanstream<char>;

/// A stream for writing to a fixed `char` buffer.
using ospanstream = basic_ospanstream<char>;

/// A stream for reading from and writing to a fixed `char` buffer.
using spanstream = basic_spanstream<char>;

} // namespace rocket::io

// EOF
