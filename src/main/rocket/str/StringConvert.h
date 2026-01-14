/**
 * @file StringConvert.h
 *
 * String-to-type and type-to-string conversions.
 */

#pragma once

#include "rocket/Exception.h"
#include "rocket/enum.h"
#include "rocket/type-traits.h"
#include "rocket/io/io.h"
#include "rocket/str/message/message.h"

#include <limits>

namespace rocket::str {

// `StringConvert` ------------------------------------------------------------------------------------------

/**
 * A general `StringConvert` class template.
 */
template<typename T>
struct StringConvert;

/// @spec_rocket_StringConvert{`bool`}
template<>
struct StringConvert<bool> {
  using Type = bool; ///< @type_alias

  /**
   * Converts @p v to a string.
   *
   * @param v the value to convert
   * @return a string
   */
  std::string toString(Type v) const noexcept { return v ? "true" : "false"; }

  /**
   * Converts @p s to a `bool` value.
   *
   * @param s the string to convert
   * @return a `bool` value
   *
   * @throw #rocket::InvalidState if @p s cannot be parsed
   */
  Type
  toType(std::string_view s) const {
    if (s == "false" || s == "0") {
      return false;
    } else if (s == "true" || s == "1") {
      return true;
    } else {
      throw InvalidState(message::cannotParseAs(s, rocket::Type::of<Type>()));
    }
  }
};

/// @spec_rocket_StringConvert{`char`}
template<>
struct StringConvert<char> {
  using Type = char; ///< @type_alias

  /**
   * Converts @p v to a string.
   *
   * @param v the value to convert
   * @return a string
   */
  std::string toString(Type v) const noexcept { return { v }; }

  /**
   * Converts @p s to a `char` value.
   *
   * @param s the string to convert
   * @return a `char` value
   *
   * @throw #rocket::InvalidState if @p s cannot be parsed
   */
  Type
  toType(std::string_view s) const {
    if (s.size() != 1) {
      throw InvalidState(message::cannotParseAs(s, rocket::Type::of<Type>()));
    }
    return s[0];
  }
};

/// @spec_rocket_StringConvert{#rocket::IsInteger<I>}
template<typename I> requires IsInteger<I>
struct StringConvert<I> {
  using Type = I; ///< @type_alias

  /**
   * Converts @p v to a string.
   *
   * @param v the value to convert
   * @return a string
   */
  std::string toString(Type v) const noexcept{ return fmt::format("{}", v); }

  /**
   * Converts @p s to an integer value.
   *
   * @param s the string to convert
   * @return an integer value
   * @throw #rocket::InvalidState if @p s cannot be parsed
   */
  Type
  toType(std::string_view s) const {
    auto is = io::is(s);
    Type ret;
    is >> ret;
    if (is.fail() || not is.eof()) {
      throw InvalidState(message::cannotParseAs(s, rocket::Type::of<Type>()));
    }
    return ret;
  }
};

/// @spec_rocket_StringConvert{enums}
template<typename E> requires Enum<E>::value
struct StringConvert<E> {
  using Type = E; ///< @type_alias

  /**
   * Converts @p v to a string.
   *
   * @param v the value to convert
   * @return a string
   */
  std::string toString(Type v) const noexcept { return fmt::format("{}", v); }

  /**
   * Converts @p s to an enum value.
   *
   * @param s the string to convert
   * @return an enum value
   * @throw #rocket::InvalidState if @p s cannot be parsed
   */
  Type
  toType(std::string_view s) const {
    return Enum<Type>::toType(s);
  }
};

/// @spec_rocket_StringConvert{#rocket::IsFloat<F>}
template<typename F> requires IsFloat<F>
struct StringConvert<F> {
  using Type = F; ///< @type_alias

  /**
   * Converts @p v to a string.
   *
   * @param v the value to convert
   * @return a string
   */
  std::string toString(Type v) const noexcept { return fmt::format("{}", v); }

  /**
   * Converts @p s to a floating-point value.
   *
   * @param s the string to convert
   * @return a floating-point value
   * @throw #rocket::InvalidState if @p s cannot be parsed
   */
  Type
  toType(std::string_view s) const {
    auto limits = std::numeric_limits<Type>();

    if (s == "inf" || s == "+inf") {
      return limits.infinity();
    } else if (s == "-inf") {
      return -limits.infinity();
    } else if (s == "nan") {
      return limits.quiet_NaN();
    } else if (s == "snan") {
      return limits.signaling_NaN();
    }

    auto is = io::is(s);
    Type ret;
    is >> ret;
    if (is.fail() || not is.eof()) {
      throw InvalidState(message::cannotParseAs(s, rocket::Type::of<Type>()));
    }
    return ret;
  }
};

/// @spec_rocket_StringConvert{`const char*`}
template<>
struct StringConvert<const char*> {
  using Type = const char*; ///< @type_alias

  /**
   * Converts @p v to a string.
   *
   * @param v the value to convert
   * @return a string
   */
  std::string toString(Type v) const noexcept { return v; }

   /**
   * Converts @p s to a `const char*` value.
   *
   * @param s the string to convert
   * @return a `const char*` value
   */
  Type toType(std::string_view s) const { data_ = s; return data_.c_str(); }

private:

  mutable std::string data_;
};

/// @spec_rocket_StringConvert{`std::string`}
template<>
struct StringConvert<std::string> {
  using Type = std::string; ///< @type_alias

  /**
   * Converts @p v to a string.
   *
   * @param v the value to convert
   * @return a string
   */
  std::string toString(const Type& v) const noexcept { return v; }

  /**
   * Converts @p s to a `std::string`.
   *
   * @param s the string to convert
   * @return a `std::string` value
   */
  Type toType(std::string_view s) const { return std::string(s); }
};

/// @spec_rocket_StringConvert{`std::string_view`}
template<>
struct StringConvert<std::string_view> {
  using Type = std::string_view; ///< @type_alias

  /**
   * Converts @p v to a string.
   *
   * @param v the value to convert
   * @return a string
   */
  std::string toString(Type v) const noexcept { return std::string(v); }

  /**
   * Converts @p s to a `std::string_view`.
   *
   * @param s the string to convert
   * @return a `std::string_view` value
   */
  Type toType(std::string_view s) const { return s; }
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Converts a string to a value of type @p T.
 *
 * @tparam T the type to convert to
 * @param s the string to convert
 * @return a value of type @p T
 * @throw #rocket::InvalidState if @p s cannot be parsed
 */
template<typename T>
T
toType(std::string_view s) {
  return StringConvert<T>().toType(s);
}

/**
 * Tries to convert a string to a value of type @p T.
 *
 * @tparam T the type to convert to
 * @param s the string to convert
 * @return a value of type @p T, or null if @p s cannot be parsed
 */
template<typename T>
std::optional<T>
tryToType(std::string_view s) {
  try {
    return StringConvert<T>().toType(s);
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

/**
 * Converts a value of type @p T to a string.
 *
 * @tparam T the type to convert from
 * @param v the value to convert
 * @return a string
 */
template<typename T>
std::string
toString(T v) noexcept {
  return StringConvert<T>().toString(v);
}

} // namespace rocket::str

// EOF
