/**
 * @file reflect-format.h
 *
 * Formatting support for C++ reflection: declared objects, instances, and variables.
 */

#pragma once

#include "rocket/type-traits.h"
#include "rocket/format/CodecFormatter.h"
#include "rocket/reflect/Declared.h"
#include "rocket/reflect/Instance.h"
#include "rocket/reflect/VarRef.h"

#include <fmt/format.h>

#include <typeinfo>

// `fmt::formatter<rocket::reflect::Declared>` --------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::reflect::Declared}
 *
 * - If the `i` format specifier is used, then the output is indented.
 * - If the `t` format specifier is used, then the name of the dynamic type is included.
 *
 * The format specifiers may appear in any order.
 */
template<typename T, typename C> requires rocket::reflect::Declared<T>::value && rocket::IsChar<C>
struct fmt::formatter<T, C> : rocket::format::internal::CodecFormatter<C, true> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(const T& val, FormatContext& ctx) const {
    return this->formatEncoded(val, typeid(val), ctx);
  }

  /// @endcond
};

// `fmt::formatter<rocket::reflect::Instance>` --------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::reflect::Instance}
 *
 * - If the `i` format specifier is used, then the output is indented.
 * - If the `t` format specifier is used, then the type name is included.
 *
 * The format specifiers may appear in any order.
 */
template<typename T, typename Inner, typename C> requires rocket::IsChar<C>
struct fmt::formatter<rocket::reflect::Instance<T, Inner>, C> :
  rocket::format::internal::CodecFormatter<C, true> {
  /// @cond undocumented

  using Type = rocket::reflect::Instance<T, Inner>;

  template<typename FormatContext>
  FormatContext::iterator
  format(const Type& val, FormatContext& ctx) const {
    return this->formatEncoded(val, typeid(typename Type::Type), ctx);
  }

  /// @endcond
};

// `fmt::formatter<rocket::reflect::VarRef>` ----------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::reflect::VarRef}
 *
 * - If the `i` format specifier is used, then the output is indented.
 */
template<typename T, typename C> requires fmt::is_formattable<T, C>::value && rocket::IsChar<C>
struct fmt::formatter<rocket::reflect::VarRef<T>, C> : rocket::format::internal::CodecFormatter<C> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(const rocket::reflect::VarRef<T>& val, FormatContext& ctx) const {
    return this->formatEncoded(val, ctx);
  }

  /// @endcond
};

// EOF
