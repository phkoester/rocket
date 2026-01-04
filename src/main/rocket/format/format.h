/**
 * @file format.h
 *
 * Formatting, built on top of {fmt}.
 */

#pragma once

#include "rocket/str/str.h"

#include <fmt/format.h>
#include <fmt/xchar.h>

#include <functional>
#include <string>
#include <unordered_map>

namespace rocket::format {

// `Format` -------------------------------------------------------------------------------------------------

template<typename C> requires Character<C>
struct FormatParams {
  std::basic_string<C> formatted_;
  std::unordered_map<std::basic_string_view<C>, std::basic_string<C>> tagged_;

  void set(const std::basic_string<C>& formatted) {
    formatted_ = formatted;
  }

  template<typename... T>
  void set(fmt::format_string<T...> fmt, T&&... args) {
    formatted_ = fmt::format(fmt, std::forward<T>(args)...);
  }

  template<typename... T>
  void set(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    formatted_ = fmt::format(locale, fmt, std::forward<T>(args)...);
  }

  void tag(std::basic_string_view<C> tag, const std::basic_string<C>& value) {
    tagged_.emplace(tag, value);
  }

  template<typename... T>
  void tag(std::basic_string_view<C> tag, fmt::format_string<T...> fmt, T&&... args) {
    tagged_.emplace(tag, fmt::format(fmt, std::forward<T>(args)...));
  }

  template<typename... T>
  void tag(std::basic_string_view<C> tag, const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    tagged_.emplace(tag, fmt::format(locale, fmt, std::forward<T>(args)...));
  }
};

template<typename C> requires Character<C>
struct Format {

  using FormatParamsProducer = std::function<FormatParams<C>()>;

  static FormatParams<C> params() {
    return {};
  }

  static FormatParams<C> params(const std::basic_string<C>& formatted) {
    FormatParams<C> ret;
    ret.set(formatted);
    return ret;
  }

  template<typename... T>
  static FormatParams<C> params(fmt::format_string<T...> fmt, T&&... args) {
    FormatParams<C> ret;
    ret.set(fmt, std::forward<T>(args)...);
    return ret;
  }

  template<typename... T>
  static FormatParams<C> params(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    FormatParams<C> ret;
    ret.set(locale, fmt, std::forward<T>(args)...);
    return ret;
  }

  explicit Format(FormatParamsProducer&& f) : params_(f()) {}

  const FormatParams<C>& get() const { return params_; }

private:

  FormatParams<C> params_;
};

} // namespace rocket::format

// `fmt::formatter<Format>`----------------------------------------------------------------------------------

/// @spec_fmt_formatter{#rocket::format::Format)
template<typename C>
struct fmt::formatter<rocket::format::Format<C>, C> {
  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const rocket::format::Format<C>& v, FormatContext& ctx) const {
    const auto& params = v.get();
    auto formatted = params.formatted_;
    for (const auto& [tag, value] : params.tagged_) {
      rocket::str::replaceIn<C>(formatted, tag, value);
    }
    return detail::write<C>(ctx.out(), formatted);
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return ctx.begin();
  }
};

// EOF
