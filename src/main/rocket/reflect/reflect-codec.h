/**
 * @file reflect-codec.h
 *
 * Codec support for C++ reflection.
 */

#pragma once

#include "rocket/codec/HashEncoder.h"
#include "rocket/nio/nio.h"
#include "rocket/reflect/reflect.h"
#include "rocket/unicode/ConvertTo.h"

#include <fmt/format.h>

#include <tuple>
#include <utility>

namespace rocket::reflect {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

template<typename T, typename C, u64 Index, typename FormatContext, typename Tuple>
constexpr FormatContext::iterator
formatElemImpl(const T& val, FormatContext& ctx, bool debug, const Tuple& refs) {
  using namespace fmt;

  // Write separator
  auto out = ctx.out();
  if constexpr (Index > 0) {
    out = detail::write<C>(out, static_cast<C>(','));
    out = detail::write<C>(out, static_cast<C>(' '));
  }

  // Get ref at index
  const auto& ref = std::get<Index>(refs);
  static_assert(IsMemberRef<decltype(ref)>::value);

  // Write name
  out = detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(ref.name()));
  out = detail::write<C>(out, static_cast<C>('='));

  // Write value
  const auto& value = ref.get(val);
  using ValueType = decltype(value);
  fmt::formatter<rocket::PurgeType<ValueType>, C> underlying;
  detail::maybe_set_debug_format(underlying, debug); // NOLINT
  ctx.advance_to(out);
  out = underlying.format(value, ctx);
  return out;
}

template<typename T, typename C, typename FormatContext, typename Tuple, u64... Index>
constexpr FormatContext::iterator
formatImpl(
  const T& val,
  FormatContext& ctx,
  bool debug,
  const Tuple& refs,
  std::index_sequence<Index...>) { // NOLINT
  using namespace fmt;

  // Write outer parentheses, inner members
  auto out = ctx.out();
  out = detail::write<C>(out, static_cast<C>('('));
  (..., (out = formatElemImpl<T, C, Index>(val, ctx, debug, refs)));
  return detail::write<C>(out, static_cast<C>(')'));
}

template<typename T, typename C, typename FormatContext, typename... Ref> requires
    (... && IsMemberRef<Ref>::value)
constexpr FormatContext::iterator
format(const T& val, FormatContext& ctx, bool debug, const std::tuple<Ref...>& refs) {
  return internal::formatImpl<T, C>(val, ctx, debug, refs, std::make_index_sequence<sizeof...(Ref)>());
}

template<u64 Index, typename T, typename Tuple>
constexpr auto&
refGet(T& val, const Tuple& refs) noexcept {
  auto& ref = std::get<Index>(refs);
  static_assert(IsMemberRef<decltype(ref)>::value);
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

template<typename T, u64 Index, typename Tuple>
u64
writeElemImpl(nio::Sink& out, const T& val, const Tuple& refs) {
  // Write separator
  u64 ret = 0;
  if constexpr (Index > 0) {
    ret += out.write(", ");
  }

  // Get ref at index
  const auto& ref = std::get<Index>(refs);
  static_assert(IsMemberRef<decltype(ref)>::value);

  // Write name
  ret += out.write(ref.name());
  ret += out.write('=');

  // Write value
  const auto& value = ref.get(val);
  ret += out.print("{}", value);
  return ret;
}

template<typename T, typename Tuple, u64... Index>
u64
writeImpl(
  nio::Sink& out,
  const T& val,
  const Tuple& refs,
  std::index_sequence<Index...>) { // NOLINT
  u64 ret = out.write('(');
  (..., (ret += writeElemImpl<T, Index>(out, val, refs)));
  ret += out.write(')');
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
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>::value)
inline bool
eq(const T& lhs, const T& rhs, const std::tuple<Ref...>& refs) {
  return internal::eqImpl(lhs, rhs, refs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * `hash` function for member references.
 *
 * @param val the instance
 * @param refs the references
 * @return a hash value
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>::value)
inline u64
hash(const T& val, const std::tuple<Ref...>& refs) {
  u64 ret = std::tuple_size<PurgeType<decltype(refs)>>::value;
  std::apply([&](auto&&... arg) {
    (..., (hash::combine(ret, rocket::codec::HashEncoder().encode(arg.get(val)))));
  }, refs);
  return ret;
}

/**
 * `ne`function for member references.
 *
 * @param lhs the left-hand side
 * @param rhs the right-hand side
 * @param refs the references
 * @return whether @p lhs is not equal to @p rhs as defined by #std::not_equal_to
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>::value)
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
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>::value)
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
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>::value)
inline bool
gt(const T& lhs, const T& rhs, const std::tuple<Ref...>& refs) {
  return internal::gtImpl(lhs, rhs, refs, std::make_index_sequence<sizeof...(Ref)>());
}

/**
 * `write` function for member references.
 *
 * @param out the sink to write to
 * @param val the instance
 * @param refs the references
 * @return the number of bytes written
 */
template<typename T, typename... Ref> requires (... && IsMemberRef<Ref>::value)
inline u64
write(nio::Sink& out, const T& val, const std::tuple<Ref...>& refs) {
  return internal::writeImpl(out, val, refs, std::make_index_sequence<sizeof...(Ref)>());
}

} // namespace rocket::reflect

// EOF
