/**
 * @file Subformat.h
 */

#pragma once

#include "rocket/type-traits.h"

#include <fmt/format.h>

#include <functional>
#include <map>
#include <string>
#include <string_view>

namespace rocket::format {

// `SubformatParams` ----------------------------------------------------------------------------------------

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
  std::map<std::basic_string<C>, std::basic_string<C>, std::less<>> tagged_;

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
   * @param val the value
   */
  void tag(std::basic_string_view<C> tag, const std::basic_string<C>& val) {
    tagged_.emplace(tag, val);
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
  void tag(
    std::basic_string_view<C> tag,
    const std::locale& locale,
    fmt::format_string<T...> fmt, T&&... args) {
    tagged_.emplace(tag, fmt::format(locale, fmt, std::forward<T>(args)...));
  }
};

// `Subformat` ----------------------------------------------------------------------------------------------

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
   * Returns the format’s parameters.
   *
   * @return the format’s parameters
   */
  [[nodiscard]] const SubformatParams<C>& get() const { return params_; }

private:

  SubformatParams<C> params_;
};

} // namespace rocket::format

// `fmt::formatter<rocket::format::Subformat>` --------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::format::Subformat}
 *
 * Every occurrence of a tag in the formatted string is replaced with the tagged value. The replacement is
 * done in a single pass, so tags in the tagged values are not replaced in turn. If several tags occur at the
 * same position, then the longest one is replaced.
 *
 * This formatter uses the same format specifiers as the underlying string formatter; they apply to the whole
 * string after the replacement.
 */
template<typename C> requires rocket::IsChar<C>
struct fmt::formatter<rocket::format::Subformat<C>, C> {
  /// @cond undocumented

  constexpr const C*
  parse(fmt::parse_context<C>& ctx) {
    return underlying_.parse(ctx);
  }

  template<typename FormatContext>
  FormatContext::iterator
  format(const rocket::format::Subformat<C>& val, FormatContext& ctx) const {
    return underlying_.format(std::basic_string_view<C>(replaceTags(val.get())), ctx);
  }

  /// @endcond

private:

  fmt::formatter<std::basic_string_view<C>, C> underlying_;

  static std::basic_string<C>
  replaceTags(const rocket::format::SubformatParams<C>& params) {
    using Size = std::basic_string<C>::size_type;
    constexpr Size npos = std::basic_string<C>::npos;

    const std::basic_string<C>& formatted = params.formatted_;
    std::basic_string<C> ret;
    Size pos = 0;
    while (true) {
      // Find the leftmost tag, and the longest one if several tags occur at the same position
      Size matchPos = npos;
      const std::basic_string<C>* tag = nullptr;
      const std::basic_string<C>* value = nullptr;
      for (const auto& [t, v] : params.tagged_) {
        if (t.empty()) {
          continue;
        }
        const Size p = formatted.find(t, pos);
        if (p < matchPos || (p != npos && p == matchPos && t.size() > tag->size())) {
          matchPos = p;
          tag = &t;
          value = &v;
        }
      }
      if (tag == nullptr) {
        break;
      }
      ret.append(formatted, pos, matchPos - pos);
      ret.append(*value);
      pos = matchPos + tag->size();
    }
    ret.append(formatted, pos);
    return ret;
  }
};

// EOF
