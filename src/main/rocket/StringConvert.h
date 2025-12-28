/**
 * @file StringConvert.h
 *
 * String-to-type and type-to-string conversions.
 */

#pragma once

#include "base.h"
#include "Exception.h"
#include "io-decl.h"
#include "message.h"

namespace rocket {

// `StringConvert` ------------------------------------------------------------------------------------------

/**
 * A general `StringConvert` class template.
 */
template<typename T>
struct StringConvert;

/// @spec_rocket_string_convert{`bool`}
template<>
struct StringConvert<bool> {
  using Type = bool; ///< @type_alias

  /**
   * Converts @p s to a `bool` value.
   *
   * @param s the string to convert
   * @return a `bool` value
   *
   * @throw #rocket::InvalidState if @p s cannot be parsed
   */
  Type
  stringToType(std::string_view s) const {
    if (s == "false" || s == "0") {
      return false;
    } else if (s == "true" || s == "1") {
      return true;
    } else {
      throw InvalidState(message::cannotParseAs(s, rocket::Type::of<Type>()));
    }
  }

  /**
   * Converts @p v to a string.
   *
   * @param v the value to convert
   * @return a string
   */
  std::string typeToString(Type v) const { return v ? "true" : "false"; }
};

/// @spec_rocket_string_convert{`char`}
template<>
struct StringConvert<char> {
  using Type = char; ///< @type_alias

  /**
   * Converts @p s to a `char` value.
   *
   * @param s the string to convert
   * @return a `char` value
   *
   * @throw #rocket::InvalidState if @p s cannot be parsed
   */
  Type
  stringToType(std::string_view s) const {
    if (s.size() != 1) {
      throw InvalidState(message::cannotParseAs(s, rocket::Type::of<Type>()));
    }
    return s[0];
  }

  /**
   * Converts @p v to a string.
   *
   * @param v the value to convert
   * @return a string
   */
  std::string typeToString(Type v) const { return { v }; }
};

/// @spec_rocket_string_convert{#Integer}
template<typename I> requires Integer<I>
struct StringConvert<I> {
  using Type = I; ///< @type_alias

  /**
   * Converts @p s to an integer value.
   *
   * @param s the string to convert
   * @return an integer value
   * @throw #rocket::InvalidState if @p s cannot be parsed
   */
  Type
  stringToType(std::string_view s) const {
    auto is = io::is(s);
    Type ret;
    is >> ret;
    if (is.fail()) {
      throw InvalidState(message::cannotParseAs(s, rocket::Type::of<Type>()));
    }
    return ret;
  }

  /**
   * Converts @p v to a string.
   *
   * @param v the value to convert
   * @return a string
   */
  std::string
  typeToString(Type v) const {
    return fmt::format("{}", v);
  }
};

/// @spec_rocket_string_convert{#FloatingPoint}
template<typename F> requires FloatingPoint<F>
struct StringConvert<F> {
  using Type = F; ///< @type_alias

  /**
   * Converts @p s to a floating-point value.
   *
   * @param s the string to convert
   * @return a floating-point value
   * @throw #rocket::InvalidState if @p s cannot be parsed
   */
  Type
  stringToType(std::string_view s) const {
    auto is = io::is(s);
    Type ret;
    is >> ret;
    if (is.fail()) {
      throw InvalidState(message::cannotParseAs(s, rocket::Type::of<Type>()));
    }
    return ret;
  }

  /**
   * Converts @p v to a string.
   *
   * @param v the value to convert
   * @return a string
   */
  std::string typeToString(Type v) const { return fmt::format("{}", v); }
};

/// @spec_rocket_string_convert{enums}
template<typename E> requires std::is_enum_v<E>
struct StringConvert<E> {
  using Type = E; ///< @type_alias

  /**
   * Converts @p s to an enum value.
   *
   * @param s the string to convert
   * @return an enum value
   * @throw #rocket::InvalidState if @p s cannot be parsed
   */
  Type
  stringToType(std::string_view s) const {
    auto is = io::is(s);
    Type ret;
    is >> ret;
    if (is.fail()) {
      throw InvalidState(message::cannotParseAs(s, rocket::Type::of<Type>()));
    }
    return ret;
  }

  /**
   * Converts @p v to a string.
   *
   * @param v the value to convert
   * @return a string
   */
  std::string typeToString(Type v) const { return fmt::format("{}", v); }
};

/// @spec_rocket_string_convert{`const char*`}
template<>
struct StringConvert<const char*> {
  using Type = const char*; ///< @type_alias

  /**
   * Converts @p s to a `const char*` value.
   *
   * @param s the string to convert
   * @return a `const char*` value
   */
  Type stringToType(std::string_view s) const { data_ = s; return data_.c_str(); }

  /**
   * Converts @p v to a string.
   *
   * @param v the value to convert
   * @return a string
   */
  std::string typeToString(Type v) const { return v; }

private:

  mutable std::string data_;
};

/// @spec_rocket_string_convert{`std::string`}
template<>
struct StringConvert<std::string> {
  using Type = std::string; ///< @type_alias

  /**
   * Converts @p s to a `std::string`.
   *
   * @param s the string to convert
   * @return a `std::string` value
   */
  Type stringToType(std::string_view s) const { return std::string(s); }

  /**
   * Converts @p v to a string.
   *
   * @param v the value to convert
   * @return a string
   */
  std::string typeToString(Type v) const { return v; }
};

/// @spec_rocket_string_convert{`std::string_view`}
template<>
struct StringConvert<std::string_view> {
  using Type = std::string_view; ///< @type_alias

  /**
   * Converts @p s to a `std::string_view`.
   *
   * @param s the string to convert
   * @return a `std::string_view` value
   */
  Type stringToType(std::string_view s) const { return s; }

  /**
   * Converts @p v to a string.
   *
   * @param v the value to convert
   * @return a string
   */
  std::string typeToString(Type v) const { return std::string(v); }
};

} // namespace rocket

// EOF
