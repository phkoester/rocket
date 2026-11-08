/**
 * @file TypeId.h
 *
 * A wrapper for #std::type_index that can be formatted with a human-readable type name.
 */

#pragma once

#include "rocket/format.h"
#include "rocket/type-traits.h"
#include "rocket/unicode/ConvertTo.h"

#include <string>
#include <type_traits>
#include <typeindex>

namespace rocket {

// `TypeId` -------------------------------------------------------------------------------------------------

/**
 * A wrapper for #std::type_index.
 *
 * The type can be given either as an object, whose dynamic type is used if it is polymorphic, or as a type
 * alone:
 *
 * ```
 * const TypeId a(ex);                        // Like `typeid(ex)`
 * const TypeId b(std::type_identity<Foo>()); // Like `typeid(Foo)`
 * const TypeId c = TypeId::of<Foo>();        // The same
 * ```
 *
 * @ThreadSafe
 */
struct TypeId {
  /**
   * Returns the type ID of a type.
   *
   * @tparam T the type
   * @return the type ID
   */
  template<typename T>
  [[nodiscard]] static TypeId of() { return TypeId(std::type_identity<T>()); }

  /// @ctor_copy
  TypeId(const TypeId& rhs) = default;

  /**
   * @ctor
   *
   * @param val the object whose type to use. If the type of @p val is polymorphic, this is the dynamic type
   */
  template<typename T>
  explicit TypeId(const T& val) : type_(typeid(val)) {}

  /**
   * @ctor
   *
   * @tparam T the type to use
   */
  template<typename T>
  explicit TypeId(std::type_identity<T>) : type_(typeid(T)) {}

  /**
   * @ctor
   *
   * @param type the type information
   */
  explicit TypeId(const std::type_info& type) : type_(type) {}

  /// @member_op_cmp_strong_ordering
  std::strong_ordering operator<=>(const TypeId& rhs) const noexcept = default;

  /// @member_fn_hash
  [[nodiscard]] u64 hash() const noexcept { return type_.hash_code(); }

  /**
   * Returns the implementation-defined name of the type, as returned by #std::type_info::name.
   *
   * @return the name, which may be mangled
   */
  [[nodiscard]] const char* name() const noexcept { return type_.name(); }

  /**
   * Returns the human-readable demangled name of the type.
   *
   * If the name cannot be demangled, or if the platform's names are not mangled (MSVC), this returns the
   * result of #name.
   *
   * @return the demangled name, for example `rocket::InvalidArgument`
   */
  [[nodiscard]] std::string prettyName() const;

private:

  const std::type_index type_;
};

} // namespace rocket

// `fmt::formatter<rocket::TypeId>` -------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::TypeId}
 *
 * The type ID is formatted as its demangled name in back ticks, for example
 * <code>`rocket::InvalidArgument`</code>.
 *
 * This formatter uses the same format specifiers as the underlying string formatter; they apply to the name
 * including the back ticks.
 */
template<typename C> requires rocket::IsChar<C>
struct fmt::formatter<rocket::TypeId, C> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(const rocket::TypeId& val, FormatContext& ctx) const {
    std::string str = '`' + val.prettyName() + '`';
    return underlying_.format(rocket::unicode::ConvertTo<C>::apply(str), ctx);
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return underlying_.parse(ctx);
  }

  constexpr void
  set_debug_format(bool val = true) {
    underlying_.set_debug_format(val);
  }

  /// @endcond

private:

  fmt::formatter<basic_string_view<C>, C> underlying_;
};

// `std::hash<rocket::TypeId>` ------------------------------------------------------------------------------

/// @spec_std_hash{#rocket::TypeId}
template<>
struct std::hash<rocket::TypeId> {
  /// @cond undocumented

  rocket::u64
  operator()(const rocket::TypeId& val) const noexcept {
    return val.hash();
  }

  /// @endcond
};

// EOF
