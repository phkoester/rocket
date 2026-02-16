/**
 * @file reflect-codec.h
 *
 * Codec support for C++ reflection.
 */

#pragma once

#include "rocket/codec/HashEncoder.h"
#include "rocket/codec/FormattedCodec.h"
#include "rocket/nio/nio.h"
#include "rocket/reflect/reflect.h"
#include "rocket/unicode/ConvertTo.h"

#include <fmt/std.h> // #fmt::formatter<#std::type_info>

#include <tuple>
#include <utility>

namespace rocket::reflect {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------
// XXX Alles weg?

template<u64 Index, typename T, typename Tuple>
constexpr auto&
refGet(T& val, const Tuple& refs) noexcept {
  auto& ref = std::get<Index>(refs);
  static_assert(IsMemberRef<decltype(ref)>);
  return ref.get(val);
}

template<typename T, typename Tuple, u64... Index>
bool
eqImpl(
  const T& lhs,
  const T& rhs,
  const Tuple& refs,
  std::index_sequence<Index...>) { // NOLINT
  return (... && std::equal_to()(refGet<Index>(lhs, refs), refGet<Index>(rhs, refs)));
}

template<typename T, typename Tuple, u64... Index>
bool
neImpl(
  const T& lhs,
  const T& rhs,
  const Tuple& refs,
  std::index_sequence<Index...>) { // NOLINT
  return (... || std::not_equal_to()(refGet<Index>(lhs, refs), refGet<Index>(rhs, refs)));
}

template<typename T, typename Tuple, u64... Index>
bool
ltImpl(
  const T& lhs,
  const T& rhs,
  const Tuple& refs,
  std::index_sequence<Index...> indices) { // NOLINT
  bool ret = false;
  (... ||
    ((ret = std::less()(refGet<Index>(lhs, refs), refGet<Index>(rhs, refs))) == true ||
      ((Index + 1 < indices.size()) &&
      std::less()(refGet<Index>(rhs, refs), refGet<Index>(lhs, refs)))));
  return ret;
}

template<typename T, typename Tuple, u64... Index>
bool
gtImpl(
  const T& lhs,
  const T& rhs,
  const Tuple& refs,
  std::index_sequence<Index...> indices) { // NOLINT
  bool ret = false;
  (... ||
    ((ret = std::greater()(refGet<Index>(lhs, refs), refGet<Index>(rhs, refs))) == true ||
      ((Index + 1 < indices.size()) &&
      std::greater()(refGet<Index>(rhs, refs), refGet<Index>(lhs, refs)))));
  return ret;
}

} // namespace internal

// Functions ------------------------------------------------------------------------------------------------

/**
 * `eq` function for member references.
 *
 * @param lhs the left-hand side
 * @param rhs the right-hand side
 * @param refs the references
 * @return whether @p lhs is equal to @p rhs as defined by #std::equal_to
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>)
inline bool
eq(const T& lhs, const T& rhs, const std::tuple<Ref...>& refs) {
  return internal::eqImpl(lhs, rhs, refs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * `ne`function for member references.
 *
 * @param lhs the left-hand side
 * @param rhs the right-hand side
 * @param refs the references
 * @return whether @p lhs is not equal to @p rhs as defined by #std::not_equal_to
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>)
inline bool
ne(const T& lhs, const T& rhs, const std::tuple<Ref...>& refs) {
  return internal::neImpl(lhs, rhs, refs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * `le` function for member references.
 *
 * @param lhs the left-hand side
 * @param rhs the right-hand side
 * @param refs the references
 * @return whether @p lhs is less than @p rhs as defined by #std::less
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>)
inline bool
lt(const T& lhs, const T& rhs, const std::tuple<Ref...>& refs) {
  return internal::ltImpl(lhs, rhs, refs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * `gt` function for member references.
 *
 * @param lhs the left-hand side
 * @param rhs the right-hand side
 * @param refs the references
 * @return whether @p lhs is greater than @p rhs as defined by #std::greater
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>)
inline bool
gt(const T& lhs, const T& rhs, const std::tuple<Ref...>& refs) {
  return internal::gtImpl(lhs, rhs, refs, std::make_index_sequence<sizeof...(Ref)>());
}

// @op_output{#rocket::reflect::Instance}
template<typename T, typename Inner>
inline std::ostream&
operator<<(std::ostream& lhs, const Instance<T, Inner>& rhs) {
  return lhs << fmt::format("{}", rhs);
}

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
  FormatContext::iterator
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
  FormatContext::iterator
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
 * This formatter uses the same format specifiers as the underlying formatter for type @ T.
 */
template<typename T, typename C> requires fmt::is_formattable<T, C>::value && rocket::IsChar<C>
struct fmt::formatter<rocket::reflect::VarRef<T>, C> {
  /// @cond undocumented

  using Type = rocket::reflect::VarRef<T>;

  template<typename FormatContext>
  FormatContext::iterator
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

  u64 operator()(const T& val) const {
    return rocket::codec::HashEncoder<>().encode(val);
  }

  /// @endcond
};

// #std::hash<#rocket::reflect::Instance> -------------------------------------------------------------------

/// @spec_std_hash{#rocket::reflect::Instance}
template<typename T, typename Inner>
struct std::hash<rocket::reflect::Instance<T, Inner>> {
  /// @cond undocumented

  u64 operator()(const rocket::reflect::Instance<T, Inner>& val) const {
    return rocket::codec::HashEncoder<>().encode(val);
  }

  /// @endcond
};

// EOF
