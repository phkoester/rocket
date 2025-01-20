/**
 * @file S.h
 *
 * A convenient ad-hoc string-buffer class.
 */

#pragma once

#include "codec-global.h"
#include "unicode-decl.h"

#include <optional>
#include <sstream>

namespace rocket {

// `S` ------------------------------------------------------------------------------------------------------

/**
 * An enum the only value of which serves as a unique tag.
 *
 * To implicitly construct a #rocket::StringBuffer, simply write
 *
 * ```
 * using rocket::S;
 * string s = S << ...;
 * ```
 */
enum StringBufferTag {
  S ///< The only value in this enum, serving as a unique tag.
};

// `Raw` ----------------------------------------------------------------------------------------------------

/**
 * A class template that is used by the #raw function to enforce unformatted output.
 *
 * @tparam T the type of the value to reference
 */
template<typename T>
struct Raw {
  /**
   * @ctor
   *
   * @param v the value to encapsulate
   */
  explicit Raw(const T& v) : v_(v) {}

  /**
   * Returns the encapsulated value.
   *
   * @return the encapsulated value
   */
  const T& operator*() const { return v_; }
  
  /**
   * Returns the encapsulated value.
   *
   * @return the encapsulated value
   */
  const T& get() const { return v_; }

private:

  const T& v_;
};

/**
 * Stores a reference to a value in a #rocket::Raw class template in order to enforce unformatted output.
 *
 * Usually, when passing a value to a #rocket::StringBuffer using `operator<<`, a suitable `printRon`
 * overload is used. In order to inhibit this and use the type's standard `operator<<`, this function may be
 * used. Example:
 *
 * ```
 * cout << (S << 10000) << '\n';      // Output: "10'000\n"
 * cout << (S << raw(10000)) << '\n'; // Output: "10000\n"
 * ```
 *
 * @tparam T the type of the value to reference
 * @param v the value to reference
 * @return a #rocket::Raw value
 * 
 */
template<typename T>
inline Raw<T>
raw(const T& v) {
  return Raw<T>(v);
}

// `StringBuffer` -------------------------------------------------------------------------------------------

/**
 * A convenient dynamic string buffer.
 *
 * Usually, you make a `StringBuffer` implicitly using `S << ...`, which gives you an object you can pass on
 * as a `std::string` or a `std::string_view`.
 *
 * A `StringBuffer` appends elements using a suitable #printRon overload, which writes RON (Rocket object
 * notation), except for `const char*` and `const char32_t*`. To enforce unformatted output, use #raw.
 */
struct StringBuffer {
  /// @ctor_default
  StringBuffer() {}

  /// @ctor_copy
  StringBuffer(const StringBuffer& rhs) { os_ << rhs.os_.str(); }

  /// @ctor_move
  StringBuffer(StringBuffer&& rhs) :
      os_(std::move(rhs.os_)),
      value_(std::move(rhs.value_)) {}

  /// @member_op_cast{`std::string&`}
  operator const std::string&() const { return value(); }

  /// @member_op_cast{`std::string_view`}
  operator std::string_view() const { return value(); }

  /// @member_op_eq
  // cppcheck-suppress passedByValue
  bool operator==(std::string_view rhs) const { return value() == rhs; }

  /**
   * Appends @p v to this string buffer, using a suitable #printRon overload.
   *
   * @tparam T the type of the value to appebd
   * @param v the value to append
   * @return_this
   */
  template<typename T>
  StringBuffer&
  print(T&& v) {
    using ::printRon;
    printRon(os_, std::forward<T>(v));
    value_ = std::nullopt;
    return *this;
  }

  /**
   * Appends @p v to this string buffer, using a suitable `operator<<` overload.
   *
   * @tparam T the type of the value to append
   * @param v the value to append
   * @return_this
   */
  template<typename T>
  StringBuffer&
  print(Raw<T>&& v) {
    return stream(v.get());
  }

  /**
   * Appends @p v to this string buffer, using a suitable `operator<<` overload.
   *
   * @param v the value to append. If @p v is null, then `null` is printed
   * @return_this
   */
  StringBuffer&
  print(const char* v) {
    return stream(v ? v : "null");
  }

  /**
   * Appends @p v to this string buffer, using a suitable `operator<<` overload.
   *
   * @param v the value to append. If @p v is null, then `null` is printed
   * @return_this
   */
  StringBuffer&
  print(const char32_t* v) {
    return stream(v ? rocket::unicode::utf32To8(v) : "null");
  }

private:

  std::ostringstream os_;
  mutable std::optional<std::string> value_;

  template<typename T>
  StringBuffer&
  stream(T&& v) {
    using ::operator<<;
    os_ << std::forward<T>(v);
    value_ = std::nullopt;
    return *this;
  }

  const std::string&
  value() const {
    if (not value_)
      value_ = os_.str();
    return *value_;
  }
};

/// @op_output{#rocket::StringBuffer}
inline std::ostream&
operator<<(std::ostream& lhs, const StringBuffer& rhs) {
  return lhs << static_cast<std::string_view>(rhs);
}

/**
 * Makes a #rocket::StringBuffer.
 *
 * @tparam T the type of the value to print
 * @param lhs the #rocket::StringBufferTag, which is #rocket::S
 * @param rhs the value to print
 * @return a new StringBuffer
 */
template<typename T>
inline StringBuffer
operator<<(StringBufferTag lhs, T&& rhs) {
  return StringBuffer().print(std::forward<T>(rhs));
}

/**
 * Prints to a #rocket::StringBuffer.
 *
 * @tparam T the type of the value to print
 * @param lhs the #rocket::StringBuffer to print to
 * @param rhs the value to print
 * @return @p lhs
 */
template<typename T>
inline const StringBuffer&
operator<<(const StringBuffer& lhs, T&& rhs) {
  return const_cast<StringBuffer&>(lhs).print(std::forward<T>(rhs));
}

} // namespace rocket

// EOF
