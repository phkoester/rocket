/**
 * @file Subformat.h
 */

#pragma once

#include "rocket/type-traits.h"

#include <boost/algorithm/string.hpp>

#include <fmt/format.h>

#include <functional>
#include <map>
#include <string>

namespace rocket::format {

// #SubformatParams ------------------------------------------------------------------------------------------

/**
 * Parameters for the #rocket::format::Subformat class.
 *
 * @tparam C the character type
 */
template<typename C> requires IsChar<C>
struct SubformatParams {
  /// The formatted string.
  std::basic_string<C> formatted_;
  /// The tagged values.
  std::map<std::basic_string_view<C>, std::basic_string<C>> tagged_;

  /**
   * Sets the formatted string.
   *
   * @param formatted the formatted string
   */
  void set(const std::basic_string<C>& formatted) {
    formatted_ = formatted;
  }

  /**
   * Sets the formatted string.
   *
   * @param fmt the format string
   * @param args the arguments
   */
  template<typename... T>
  void set(fmt::format_string<T...> fmt, T&&... args) {
    formatted_ = fmt::format(fmt, std::forward<T>(args)...);
  }

  /**
   * Sets the formatted string.
   *
   * @param locale the locale
   * @param fmt the format string
   * @param args the arguments
   */
  template<typename... T>
  void set(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    formatted_ = fmt::format(locale, fmt, std::forward<T>(args)...);
  }

  /**
   * Tags a value.
   *
   * @param tag the tag
   * @param value the value
   */
  void tag(std::basic_string_view<C> tag, const std::basic_string<C>& value) {
    tagged_.emplace(tag, value);
  }

  /**
   * Tags a value.
   *
   * @param tag the tag
   * @param fmt the format string
   * @param args the arguments
   */
  template<typename... T>
  void tag(std::basic_string_view<C> tag, fmt::format_string<T...> fmt, T&&... args) {
    tagged_.emplace(tag, fmt::format(fmt, std::forward<T>(args)...));
  }

  /**
   * Tags a value.
   *
   * @param tag the tag
   * @param locale the locale
   * @param fmt the format string
   * @param args the arguments
   */
  template<typename... T>
  void tag(std::basic_string_view<C> tag, const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    tagged_.emplace(tag, fmt::format(locale, fmt, std::forward<T>(args)...));
  }
};

// #Subformat -----------------------------------------------------------------------------------------------

/**
 * A subformat that may be passed as an argument to `fmt::format`.
 *
 * @tparam C the character type
 */
template<typename C> requires IsChar<C>
struct Subformat {

  /// A function that produces #rocket::format::SubformatParams.
  using SubformatParamsProducer = std::function<SubformatParams<C>()>;

  /**
   * Makes empty parameters, resulting in an empty string.
   *
   * @return parameters
   */
  static SubformatParams<C> params() {
    return {};
  }

  /**
   * Makes parameters with an already formatted string.
   *
   * @param formatted the formatted string
   * @return parameters
   */
  static SubformatParams<C> params(const std::basic_string<C>& formatted) {
    SubformatParams<C> ret;
    ret.set(formatted);
    return ret;
  }

  /**
   * Makes parameters with a format string and its arguments.
   *
   * @param fmt the format string
   * @param args the arguments
   * @return parameters
   */
  template<typename... T>
  static SubformatParams<C> params(fmt::format_string<T...> fmt, T&&... args) {
    SubformatParams<C> ret;
    ret.set(fmt, std::forward<T>(args)...);
    return ret;
  }

  /**
   * Makes parameters with a locale, format string, and its arguments.
   *
   * @param locale the locale
   * @param fmt the format string
   * @param args the arguments
   * @return parameters
   */
  template<typename... T>
  static SubformatParams<C> params(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    SubformatParams<C> ret;
    ret.set(locale, fmt, std::forward<T>(args)...);
    return ret;
  }

  /**
   * @ctor
   *
   * @param fn a function that produces #rocket::format::SubformatParams
   */
  explicit Subformat(const SubformatParamsProducer& fn) : params_(fn()) {}

  /**
   * Returns the format's parameters.
   *
   * @return the format's parameters
   */
  const SubformatParams<C>& get() const { return params_; }

private:

  SubformatParams<C> params_;
};

} // namespace rocket::format

// #fmt::formatter<#rocket::format::Subformat> --------------------------------------------------------------

/// @spec_fmt_formatter{#rocket::format::Format)
template<typename C>
struct fmt::formatter<rocket::format::Subformat<C>, C> {
  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const rocket::format::Subformat<C>& val, FormatContext& ctx) const {
    const auto& params = val.get();
    auto formatted = params.formatted_;
    for (const auto& [tag, value] : params.tagged_) {
      boost::replace_all(formatted, tag, value);
    }
    return detail::write<C>(ctx.out(), formatted);
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return ctx.begin();
  }
};

// EOF
