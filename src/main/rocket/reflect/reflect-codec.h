/**
 * @file reflect-codec.h
 *
 * Codec support for C++ reflection.
 */

#pragma once

#include "rocket/codec/EqualToEncoder.h"
#include "rocket/codec/HashEncoder.h"
#include "rocket/codec/FormattedCodec.h"
#include "rocket/nio/nio.h"
#include "rocket/unicode/ConvertTo.h"

#include <fmt/std.h> // #fmt::formatter<#std::type_info>

#include <tuple>
#include <utility> // #std::hash

namespace rocket::reflect {

// #Instance ------------------------------------------------------------------------------------------------

template<typename T, typename Inner>
inline bool
operator==(const Instance<T, Inner>& lhs, const Instance<T, Inner>& rhs) {
  return codec::EqualToEncoder<>().encode(lhs, rhs);
}

// XXX operator<=>

// @op_output{#rocket::reflect::Instance}
template<typename T, typename Inner>
inline std::ostream&
operator<<(std::ostream& lhs, const Instance<T, Inner>& rhs) {
  return lhs << fmt::format("{}", rhs);
}

// #VarRef --------------------------------------------------------------------------------------------------

template<typename T>
inline bool
operator==(const VarRef<T>& lhs, const VarRef<T>& rhs) {
  return codec::EqualToEncoder<>().encode(lhs.get(), rhs.get());
}

// XXX operator<=>

// @op_output{#rocket::reflect::VarRef}
template<typename T>
inline std::ostream&
operator<<(std::ostream& lhs, const VarRef<T>& rhs) {
  return lhs << fmt::format("{}", rhs);
}

} // namespace rocket::reflect

// #fmt::formatter<#rocket::reflect::Declared> --------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::reflect::Declared}
 *
 * - If the `i` format specifier is used, then the output is indented.
 * - If the `t` format specifier is used, then the type name is included.
 */
template<typename T, typename C> requires rocket::reflect::Declared<T>::value && rocket::IsChar<C>
struct fmt::formatter<T, C> {
  /// @cond undocumented
  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const T& val, FormatContext& ctx) const {
    auto out = ctx.out();
    if (withType_) {
      const std::string typeName = fmt::format("{}", typeid(val));
      // GCC 13.3 needs `fmt::detail` here
      out = fmt::detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(typeName));
    }
    rocket::codec::FormattedCodec codec;
    rocket::nio::StringSink sink;
    codec.encode(val, sink, { .indent=indent_ });
    // GCC 13.3 needs `fmt::detail` here
    out = fmt::detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(sink.str()));
    return out;
  }

  constexpr const C*
  parse(fmt::parse_context<C>& ctx) { // GCC 13.3 needs `fmt::parse_context` here
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it == 'i') {
      indent_ = true;
      ++it;
    }
    if (it != end && *it == 't') {
      withType_ = true;
      ++it;
    }
    return it;
  }
  /// @endcond

private:

  bool indent_ = false;
  bool withType_ = false;
};

// #fmt::formatter<#rocket::reflect::Instance> --------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::reflect::Instance}
 *
 * - If the `i` format specifier is used, then the output is indented.
 * - If the `t` format specifier is used, then the type name is included.
 */
template<typename T, typename Inner, typename C> requires rocket::IsChar<C>
struct fmt::formatter<rocket::reflect::Instance<T, Inner>, C> {
  /// @cond undocumented
  using Type = rocket::reflect::Instance<T, Inner>;

  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const Type& val, FormatContext& ctx) const {
    auto out = ctx.out();
    if (withType_) {
      const std::string typeName = fmt::format("{}", typeid(typename Type::Type));
      out = detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(typeName));
    }
    rocket::codec::FormattedCodec codec;
    rocket::nio::StringSink sink;
    codec.encode(val, sink, { .indent=indent_ });
    out = detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(sink.str()));
    return out;
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it == 'i') {
      indent_ = true;
      ++it;
    }
    if (it != end && *it == 't') {
      withType_ = true;
      ++it;
    }
    return it;
  }
  /// @endcond

private:

  bool indent_ = false;
  bool withType_ = false;
};

// #fmt::formatter<#rocket::reflect::VarRef> ----------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::reflect::VarRef}
 *
 * - If the `i` format specifier is used, then the output is indented.
 */
template<typename T, typename C> requires fmt::is_formattable<T, C>::value && rocket::IsChar<C>
struct fmt::formatter<rocket::reflect::VarRef<T>, C> {
  /// @cond undocumented
  using Type = rocket::reflect::VarRef<T>;

  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const Type& val, FormatContext& ctx) const {
    auto out = ctx.out();
    rocket::codec::FormattedCodec codec;
    rocket::nio::StringSink sink;
    codec.encode(val, sink, { .indent=indent_ });
    out = detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(sink.str()));
    return out;
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it == 'i') {
      indent_ = true;
      ++it;
    }
    return it;
  }
  /// @endcond

private:

  bool indent_ = false;
};

// #std::hash<#rocket::reflect::Declared> -------------------------------------------------------------------

/// @spec_std_hash{#rocket::reflect::Declared}
template<typename T> requires rocket::reflect::Declared<T>::value
struct std::hash<T> {
  /// @cond undocumented
  u64
  operator()(const T& val) const {
    return rocket::codec::HashEncoder<>().encode(val);
  }
  /// @endcond
};

// #std::hash<#rocket::reflect::Instance> -------------------------------------------------------------------

/// @spec_std_hash{#rocket::reflect::Instance}
template<typename T, typename Inner>
struct std::hash<rocket::reflect::Instance<T, Inner>> {
  /// @cond undocumented
  using Type = rocket::reflect::Instance<T, Inner>;

  u64
  operator()(const Type& val) const {
    return rocket::codec::HashEncoder<>().encode(val);
  }
  /// @endcond
};

// #std::hash<#rocket::reflect::VarRef> ---------------------------------------------------------------------

/// @spec_std_hash{#rocket::reflect::VarRef}
template<typename T>
struct std::hash<rocket::reflect::VarRef<T>> {
  /// @cond undocumented
  using Type = rocket::reflect::VarRef<T>;

  u64
  operator()(const Type& val) const {
    return rocket::codec::HashEncoder<>().encode(val);
  }
  /// @endcond
};

// EOF
