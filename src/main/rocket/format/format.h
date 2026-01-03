/**
 * @file format.h
 *
 * Formatting, built on top of {fmt}.
 */

#pragma once

#include "rocket/str/str.h"

#include <fmt/format.h>

#include <functional>
#include <string>
#include <unordered_map>

namespace rocket::format {

// `Format` -------------------------------------------------------------------------------------------------

struct Format {
  struct Params {
    std::string formatted_;
    std::unordered_map<std::string_view, std::string> tagged_;

    template<typename... T>
    void set(fmt::format_string<T...> fmt, T&&... args) {
      formatted_ = fmt::format(fmt, std::forward<T>(args)...);
    }

    template<typename... T>
    void set(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
      formatted_ = fmt::format(locale, fmt, std::forward<T>(args)...);
    }

    template<typename... T>
    void tag(std::string_view tag, fmt::format_string<T...> fmt, T&&... args) {
      tagged_.emplace(tag, fmt::format(fmt, std::forward<T>(args)...));
    }

    template<typename... T>
    void tag(std::string_view tag, const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
      tagged_.emplace(tag, fmt::format(locale, fmt, std::forward<T>(args)...));
    }
  };

  using ParamsProducer = std::function<Params()>;

  static Params params() {
    return {};
  }

  template<typename... T>
  static Params params(fmt::format_string<T...> fmt, T&&... args) {
    Params ret;
    ret.set(fmt, std::forward<T>(args)...);
    return ret;
  }

  template<typename... T>
  static Params params(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    Params ret;
    ret.set(locale, fmt, std::forward<T>(args)...);
    return ret;
  }

  explicit Format(ParamsProducer&& f) : params_(f()) {}

  const Params& get() const { return params_; }

private:

  Params params_;
};

} // namespace rocket::format

// `fmt::formatter<Format>`----------------------------------------------------------------------------------

/// @spec_fmt_formatter{#rocket::format::Format)
template<typename C>
struct fmt::formatter<rocket::format::Format, C> {
  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const rocket::format::Format& v, FormatContext& ctx) const {
    const auto& params = v.get();
    auto formatted = params.formatted_;
    for (const auto& [tag, value] : params.tagged_) {
      rocket::str::replaceIn<char>(formatted, tag, value);
    }
    return detail::write<C>(ctx.out(), formatted);
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return ctx.begin();
  }
};

namespace rocket::format {

// `NativeFormatter` ----------------------------------------------------------------------------------------

/**
 * This is essentially a copy of `fmt::native_formatter`, which gives us access to the `specs_` member, and
 * the flexibility to adapt the code.
 */
template<typename T, typename C>
struct NativeFormatter {
  using nonlocking = void;
  using type = fmt::detail::type;

  static constexpr type TYPE = fmt::detail::type_constant<T, C>::value;
  static_assert(TYPE != fmt::detail::type::custom_type, "NativeFormatter cannot be used for custom types");

  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const T& val, FormatContext& ctx) const {
    using namespace fmt;
    using namespace fmt::detail;

    if (not specs_.dynamic()) {
      return write<C>(ctx.out(), val, specs_, ctx.locale());
    }
    auto specs = format_specs(specs_);
    handle_dynamic_spec(specs.dynamic_width(), specs.width, specs_.width_ref, ctx);
    handle_dynamic_spec(specs.dynamic_precision(), specs.precision, specs_.precision_ref, ctx);
    return write<C>(ctx.out(), val, specs, ctx.locale());
  }

  constexpr const C*
  parse(fmt::parse_context<C>& ctx) {
    using namespace fmt::detail;

    if (ctx.begin() == ctx.end() || *ctx.begin() == '}') {
      return ctx.begin();
    }
    auto end = parse_format_specs(ctx.begin(), ctx.end(), specs_, ctx, TYPE);
    if (const_check(TYPE == type::char_type)) {
      check_char_specs(specs_);
    }
    return end;
  }

  auto& specs() { return specs_; }

  const auto& specs() const { return specs_; }

private:

  fmt::detail::dynamic_format_specs<C> specs_;
};

} // namespace rocket::format

// EOF
