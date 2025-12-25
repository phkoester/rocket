/**
 * @file StringConvert.h
 *
 * String-to-type and type-to-string conversions.
 *
 * @attention Before including this header, appopriate codec-declaration headers must be included, e.g.
 * `codec-global.h` or `codec-std-decl.h` and `codec-std.h`.
 */

#pragma once

#include "codec.h"
#include "except.h"
#include "io-decl.h"

namespace rocket {

// `StringConvert` ------------------------------------------------------------------------------------------

/**
 * A general `StringConvert` class template.
 */
template<typename T>
struct StringConvert;

// `StringConvert<bool>` ....................................................................................

/**
 * `StringConvert` specialization for boolean values.
 */
template<>
struct StringConvert<bool> {
  using Type = bool; ///< @type_alias

  /**
   * Converts @p s to a boolean value
   *
   * @param s the string to convert
   * @return a boolean value
   *
   * @throw #rocket::except::InputFailure if `is.fail()` returns `true`
   * @throw #rocket::except::ParseFailure if @p s cannot be parsed as a boolean value
   */
  Type stringToType(std::string_view s) const;

  /**
   * Converts @p v to a string.
   *
   * @param v a boolean value
   * @return a string
   */
  std::string typeToString(Type v) const;
};

// `StringConvert<const char*>` .............................................................................

/**
 * `StringConvert` specialization for `const char*`.
 */
template<>
struct StringConvert<const char*> {
  using Type = const char*; ///< @type_alias

  /**
   * Converts @p s to a `const char*` value.
   *
   * @param s the string to convert
   * @return a `const char*` value
   */
  inline Type stringToType(std::string_view s) const { data_ = s; return data_.c_str(); }

  /**
   * Converts @p v to a string.
   *
   * @param v a `const char*` value
   * @return a string
   */
  inline std::string typeToString(Type v) const { return v; }

private:

  mutable std::string data_;
};

// `StringConvert<std::string>` .............................................................................

/**
 * `StringConvert` specialization for `std::string`.
 */
template<>
struct StringConvert<std::string> {
  using Type = std::string; ///< @type_alias

  /**
   * Converts @p s to a `std::string`.
   *
   * @param s the string to convert
   * @return a `std::string` value
   */
  inline Type stringToType(std::string_view s) const { return std::string(s); }

  /**
   * Converts @p v to a string.
   *
   * @param v a `std::string` value
   * @return a string
   */
  inline std::string typeToString(const Type& v) const { return v; }
};

// `StringConvert<std::string_view>` ........................................................................

/**
 * `StringConvert` specialization for `std::string_view`.
 */
template<>
struct StringConvert<std::string_view> {
  using Type = std::string_view; ///< @type_alias

  /**
   * Converts @p s to a `std::string_view`.
   *
   * @param s the string to convert
   * @return a `std::string_view` value
   */
  inline Type stringToType(std::string_view s) const { return s; }

  /**
   * Converts @p v to a string.
   *
   * @param v a `std::string_view` value
   * @return a string
   */
  inline std::string typeToString(const Type& v) const { return std::string(v); }
};

// `IntegerStringConvert` -----------------------------------------------------------------------------------

/**
 * Integer string conversions.
 *
 * @tparam I the integer type
 */
template<typename I> requires Integer<I>
struct IntegerStringConvert {
  using Type = I; ///< @type_alias

  /**
   * Converts @p s to an integer value.
   *
   * @param s the string to convert
   * @return an integer value
   * @throw #rocket::except::InputFailure if `is.fail()` returns `true`
   * @throw #rocket::except::ParseFailure if @p s cannot be parsed as an integer value
   */
  Type
  stringToType(std::string_view s) const {
    auto is = io::is(s);

    try {
      auto ret = codec::getInteger<Type>(is);
      if (io::tellg(is) == s.size())
        return ret;
    } catch (const std::exception& ex) {}

    throw except::ParseFailure<char>(
      is, 0, { 0, s.size() }, except::message::cannotParseAs(s, rocket::Type::of<Type>()));
  }

  /**
   * Converts @p v to a string.
   *
   * @param v an integer value
   * @return a string
   */
  std::string
  typeToString(Type v) const {
    return codec::ron::print(v);
  }
};

// `EnumStringConvert` --------------------------------------------------------------------------------------

/**
 * Enum string conversions.
 *
 * @tparam E the enum type
 */
template<typename E> requires std::is_enum_v<E>
struct EnumStringConvert {
  using Type = E; ///< @type_alias

  /**
   * Converts @p s to an enum.
   *
   * @param s the string to convert
   * @return an enum value
   * @throw #rocket::except::ParseFailure if @p s could not be parsed as a value of the appropriate type
   */
  Type
  stringToType(std::string_view s) const {
    Type ret;
    auto is = io::is(s);
    is >> ret;
    if (is.fail() || io::tellg(is) != s.size()) {
      throw except::ParseFailure<char>(
          is, 0, { 0, s.size() }, except::message::cannotParseAs(s, rocket::Type::of<Type>()));
    }
    return ret;
  }

  /**
   * Converts @p v to a string.
   *
   * @param v an enum value
   * @return a string
   */
  std::string
  typeToString(Type v) const {
    std::ostringstream os;
    os << v;
    return os.str();
  }
};

// `FloatingPointStringConvert` -----------------------------------------------------------------------------

/**
 * Floating-point string conversions.
 *
 * @tparam F the floating-point type
 */
template<typename F> requires FloatingPoint<F>
struct FloatingPointStringConvert {
  using Type = F; ///< @type_alias

  /**
   * Converts @p s to a floating-point value.
   *
   * @param s the string to convert
   * @param precision the floating-point precision to use
   * @return a floating-point value
   * @throw #rocket::except::InputFailure if `is.fail()` returns `true`
   * @throw #rocket::except::ParseFailure if @p s cannot be parsed as a floating-point value
   */
  Type
  stringToType(std::string_view s, int precision = rocket::DEFAULT_PRECISION) const {
    auto is = io::is(s);

    try {
      auto ret = codec::getFloatingPoint<Type>(is, precision);
      if (io::tellg(is) == s.size())
        return ret;
    } catch (const std::exception& ex) {}

    throw except::ParseFailure<char>(
        is, 0, { 0, s.size() }, except::message::cannotParseAs(s, rocket::Type::of<Type>()));
  }

  /**
   * Converts @p v to a string.
   *
   * @param v a floating-point value
   * @param precision the floating-point precision to use
   * @return a string
   */
  std::string
  typeToString(Type v, int precision = rocket::DEFAULT_PRECISION) const {
    std::ostringstream os;
    printRon(os, v, precision);
    return os.str();
  }
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Converts the string @p s to a value of type @p T. The string must fully match the type's grammar, no
 * invalid leading or trailing characters are allowed.
 *
 * @tparam T the type to convert a string to
 * @param s the string to parse as a value of type @p T
 * @return a value of type @p T
 * @throw std::exception if the conversion fails. The actual exception type depends on the type-specific
 *     string-conversion implementation
 */
template<typename T>
T
// cppcheck-suppress passedByValue
stringToType(std::string_view s) {
  if constexpr (IsInteger<T>::value)
    return IntegerStringConvert<T>().stringToType(s);
  else if constexpr (std::is_enum_v<T>)
    return EnumStringConvert<T>().stringToType(s);
  else if constexpr (IsFloatingPoint<T>::value)
    return FloatingPointStringConvert<T>().stringToType(s);
  else
    return StringConvert<T>().stringToType(s);
}

/**
 * Tries to convert the string @p s to a value of type @p T. The string must fully match the type's grammar,
 * no invalid leading or trailing characters are allowed.
 *
 * @tparam T the type to convert a string to
 * @param s the string to parse as a value of type @p T
 * @return a value of type @p T if the operation succeeds, otherwise null
 */
template<typename T>
std::optional<T>
// cppcheck-suppress passedByValue
tryStringToType(std::string_view s) {
  try {
    return stringToType<T>(s);
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

/**
 * Converts the value @p v of type @p T to a string.
 *
 * @tparam T the type of @p v
 * @param v the value to convert to a string
 * @return a string
 * @throw std::exception if the conversion fails. The actual exception type depends on the type-specific
 *     string-conversion implementation
 */
template<typename T>
std::string
typeToString(T&& v) {
  using U = std::decay_t<T>;
  if constexpr (IsInteger<U>::value)
    return IntegerStringConvert<U>().typeToString(std::forward<T>(v));
  else if constexpr (std::is_enum_v<U>)
    return EnumStringConvert<U>().typeToString(std::forward<T>(v));
  else if constexpr (IsFloatingPoint<U>::value)
    return FloatingPointStringConvert<U>().typeToString(std::forward<T>(v));
  else
    return StringConvert<U>().typeToString(std::forward<T>(v));
}

} // namespace rocket

// EOF
