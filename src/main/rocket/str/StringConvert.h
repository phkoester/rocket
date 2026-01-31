/**
 * @file StringConvert.h
 *
 * String-to-type and type-to-string conversions.
 */

#pragma once

#include "rocket/enum.h"
#include "rocket/Exception.h"
#include "rocket/type-traits.h"
#include "rocket/scan/scan.h"
#include "rocket/str/message/message.h"

namespace rocket::str {

// #StringConvert -------------------------------------------------------------------------------------------

/**
 * A string-conversion class template.
 */
template<typename T>
struct StringConvert;

/// @spec_rocket_StringConvert{`bool`}
template<>
struct StringConvert<bool> {
  using Type = bool; ///< @type_alias

  /**
   * Converts @p val to a string.
   *
   * @param val the value to convert
   * @return a string
   */
  [[nodiscard]] std::string toString(Type val) const noexcept { return val ? "true" : "false"; }

  /**
   * Converts @p str to a `bool` value.
   *
   * @param str the string to convert
   * @return a `bool` value
   */
  [[nodiscard]] Type
  toType(std::string_view str) const {
    // Anything that is not false is true, e.g. "42"!
    return not isFalse(str);
  }

private:

  static bool isFalse(std::string_view str);
};

/// @spec_rocket_StringConvert{`char`}
template<>
struct StringConvert<char> {
  using Type = char; ///< @type_alias

  /**
   * Converts @p val to a string.
   *
   * @param val the value to convert
   * @return a string
   */
  [[nodiscard]] std::string toString(Type val) const noexcept { return { val }; }

  /**
   * Converts @p str to a `char` value.
   *
   * @param str the string to convert
   * @return a `char` value
   *
   * @throw #rocket::InvalidState if @p str cannot be scanned
   */
  [[nodiscard]] Type
  toType(std::string_view str) const {
    if (str.size() != 1) {
      throw InvalidState(message::cannotScanAs(str, typeid(Type)));
    }
    return str[0];
  }
};

/// @spec_rocket_StringConvert{integer types}
template<typename I> requires IsInteger<I>
struct StringConvert<I> {
  using Type = I; ///< @type_alias

  /**
   * Converts @p val to a string.
   *
   * @param val the value to convert
   * @return a string
   */
  [[nodiscard]] std::string toString(Type val) const noexcept{ return fmt::format("{}", val); }

  /**
   * Converts @p str to an integer value.
   *
   * @param str the string to convert
   * @return an integer value
   * @throw #rocket::InvalidState if @p str cannot be scanned
   */
  [[nodiscard]] Type
  toType(std::string_view str) const {
    auto result = scn::scan<I>(str, "{}");
    if (not result || result->begin() != str.end()) {
      throw InvalidState(message::cannotScanAs(str, typeid(Type)));
    }
    return result->value();
  }
};

/// @spec_rocket_StringConvert{enums}
template<typename E> requires Enum<E>::value
struct StringConvert<E> {
  using Type = E; ///< @type_alias

  /**
   * Converts @p val to a string.
   *
   * @param val the value to convert
   * @return a string
   */
  [[nodiscard]] std::string toString(Type val) const noexcept { return fmt::format("{}", val); }

  /**
   * Converts @p str to an enum value.
   *
   * @param str the string to convert
   * @return an enum value
   * @throw #rocket::InvalidState if @p str cannot be scanned
   */
  [[nodiscard]] Type
  toType(std::string_view str) const {
    return Enum<Type>::toType(str);
  }
};

/// @spec_rocket_StringConvert{floating-point types}
template<typename F> requires IsFloat<F>
struct StringConvert<F> {
  using Type = F; ///< @type_alias

  /**
   * Converts @p val to a string.
   *
   * @param val the value to convert
   * @return a string
   */
  [[nodiscard]] std::string toString(Type val) const noexcept { return fmt::format("{}", val); }

  /**
   * Converts @p str to a floating-point value.
   *
   * @param str the string to convert
   * @return a floating-point value
   * @throw #rocket::InvalidState if @p str cannot be scanned
   */
  [[nodiscard]] Type
  toType(std::string_view str) const {
    auto result = scn::scan<F>(str, "{}");
    if (not result || result->begin() != str.end()) {
      throw InvalidState(message::cannotScanAs(str, typeid(Type)));
    }
    return result->value();
  }
};

/// @spec_rocket_StringConvert{`const char*`}
template<>
struct StringConvert<const char*> {
  using Type = const char*; ///< @type_alias

  /**
   * Converts @p val to a string.
   *
   * @param val the value to convert
   * @return a string
   */
  [[nodiscard]] std::string toString(Type val) const noexcept { return val; }

  /**
   * Converts @p str to a `const char*` value.
   *
   * @param str the string to convert
   * @return a `const char*` value
   */
  [[nodiscard]] Type toType(std::string_view str) const { data_ = str; return data_.c_str(); }

private:

  mutable std::string data_;
};

/// @spec_rocket_StringConvert{#std::string}
template<>
struct StringConvert<std::string> {
  using Type = std::string; ///< @type_alias

  /**
   * Converts @p val to a string.
   *
   * @param val the value to convert
   * @return a string
   */
  [[nodiscard]] std::string toString(const Type& val) const noexcept { return val; }

  /**
   * Converts @p str to a #std::string.
   *
   * @param str the string to convert
   * @return a #std::string value
   */
  [[nodiscard]] Type toType(std::string_view str) const { return std::string(str); }
};

/// @spec_rocket_StringConvert{#std::string_view}
template<>
struct StringConvert<std::string_view> {
  using Type = std::string_view; ///< @type_alias

  /**
   * Converts @p val to a string.
   *
   * @param val the value to convert
   * @return a string
   */
  [[nodiscard]] std::string toString(Type val) const noexcept { return std::string(val); }

  /**
   * Converts @p str to a #std::string_view.
   *
   * @param str the string to convert
   * @return a #std::string_view value
   */
  [[nodiscard]] Type toType(std::string_view str) const { return str; }
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Converts a string to a value of type @p T.
 *
 * @tparam T the type to convert to
 * @param str the string to convert
 * @return a value of type @p T
 * @throw #rocket::InvalidState if @p str cannot be scanned
 */
template<typename T>
[[nodiscard]] T
toType(std::string_view str) {
  return StringConvert<T>().toType(str);
}

/**
 * Tries to convert a string to a value of type @p T.
 *
 * @tparam T the type to convert to
 * @param str the string to convert
 * @return a value of type @p T, or null if @p str cannot be scanned
 */
template<typename T>
[[nodiscard]] std::optional<T>
tryToType(std::string_view str) {
  try {
    return StringConvert<T>().toType(str);
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

/**
 * Converts a value of type @p T to a string.
 *
 * @tparam T the type to convert from
 * @param val the value to convert
 * @return a string
 */
template<typename T>
[[nodiscard]] std::string
toString(T val) noexcept {
  return StringConvert<T>().toString(val);
}

} // namespace rocket::str

// EOF
