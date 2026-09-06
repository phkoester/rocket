/**
 * @file StringConvert.h
 *
 * String-to-type and type-to-string conversions.
 */

#pragma once

#include "rocket/enum.h"
#include "rocket/Exception.h"
#include "rocket/type-traits.h"
#include "rocket/str/message/message.h"

#include <scn/scan.h>

namespace rocket::str {

namespace internal {

// `StringConvert` ------------------------------------------------------------------------------------------

template<typename T>
struct StringConvert;

template<>
struct StringConvert<bool> {
  using Type = bool;

  [[nodiscard]] static std::string toString(Type val) { return val ? "true" : "false"; }

  [[nodiscard]] static Type
  toType(std::string_view str) {
    // Anything that is not false is true, e.g. "42"!
    return not isFalse(str);
  }

private:

  static bool isFalse(std::string_view str);
};

template<>
struct StringConvert<char> {
  using Type = char;

  [[nodiscard]] static std::string toString(Type val) { return { val }; }

  [[nodiscard]] static Type
  toType(std::string_view str) {
    if (str.size() != 1) {
      throw InvalidState(message::cannotScanAs(str, typeid(Type)));
    }
    return str[0];
  }
};

template<typename I> requires IsInteger<I>
struct StringConvert<I> {
  using Type = I;

  [[nodiscard]] static std::string toString(Type val) { return fmt::format("{}", val); }

  [[nodiscard]] static Type
  toType(std::string_view str) {
    auto result = scn::scan<I>(str, "{}");
    if (not result || result->begin() != str.end()) {
      throw InvalidState(message::cannotScanAs(str, typeid(Type)));
    }
    return result->value();
  }
};

template<typename E> requires Enum<E>::value
struct StringConvert<E> {
  using Type = E;

  [[nodiscard]] static std::string toString(Type val) { return fmt::format("{}", val); }

  [[nodiscard]] static Type
  toType(std::string_view str) {
    const auto [_, val] = Enum<Type>::toType(str, true);
    return val;
  }
};

template<typename F> requires IsFloat<F>
struct StringConvert<F> {
  using Type = F;

  [[nodiscard]] static std::string toString(Type val) { return fmt::format("{}", val); }

  [[nodiscard]] static Type
  toType(std::string_view str) {
    auto result = scn::scan<F>(str, "{}");
    if (not result || result->begin() != str.end()) {
      throw InvalidState(message::cannotScanAs(str, typeid(Type)));
    }
    return result->value();
  }
};

template<>
struct StringConvert<std::string> {
  using Type = std::string;

  [[nodiscard]] static std::string toString(const Type& val) { return val; }

  [[nodiscard]] static Type toType(std::string_view str) { return std::string(str); }
};

template<>
struct StringConvert<std::string_view> {
  using Type = std::string_view;

  [[nodiscard]] static std::string toString(Type val) { return std::string(val); }

  [[nodiscard]] static Type toType(std::string_view str) { return str; }
};

} // namespace internal

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
  return internal::StringConvert<T>::toType(str);
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
    return internal::StringConvert<T>::toType(str);
  } catch (const std::exception&) {
    return {};
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
toString(T val) {
  return internal::StringConvert<T>::toString(val);
}

} // namespace rocket::str

// EOF
