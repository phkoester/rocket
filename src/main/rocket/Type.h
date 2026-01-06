/**
 * @file Type.h
 *
 * Run-time types.
 */

#pragma once

#include "rocket/Lazy.h"
#include "rocket/format/format.h"
#include "rocket/unicode/ConvertTo.h"

#include <iosfwd>
#include <string>
#include <typeindex>
#include <typeinfo>

namespace rocket {

// `Type` ---------------------------------------------------------------------------------------------------

/**
 * A #rocket::Type instance represents a run-time type.
 *
 * This class combines the functionality of `std::type_info` and `std::type_index`.
 *
 * The #name member function returns a pretty name.
 */
struct Type {
  /**
   * Makes a #rocket::Type of type @p T.
   *
   * @tparam T the type to obatain a #rocket::Type for
   * @return a new #rocket::Type
   */
  template<typename T>
  static inline Type of() { return typeid(T); }

  /**
   * Makes a #rocket::Type of value @p v.
   *
   * @tparam T the type of @p v
   * @param v a value of type @p T
   * @return a new #rocket::Type
   */
  template<typename T>
  static inline Type of(const T& v) { return typeid(v); }

  /**
   * @ctor
   *
   * @param info a `std::type_info` reference
   */
  // cppcheck-suppress noExplicitConstructor
  Type(const std::type_info& info);

  /// @member_op_cast{`std::type_info`}
  operator const std::type_info&() const { return info_; }

  /// @member_op_eq
  bool operator==(const Type& rhs) const { return index_ == rhs.index_; }

  /// @member_op_ne
  bool operator!=(const Type& rhs) const { return index_ != rhs.index_; }

  /// @member_op_lt
  bool operator<(const Type& rhs) const { return index_ < rhs.index_; }

  /// @member_op_le
  bool operator<=(const Type& rhs) const { return index_ <= rhs.index_; }

  /// @member_op_gt
  bool operator>(const Type& rhs) const { return index_ > rhs.index_; }

  /// @member_op_ge
  bool operator>=(const Type& rhs) const { return index_ >= rhs.index_; }

  /// @member_op_cmp_strong_ordering
  std::strong_ordering operator<=>(const Type& rhs) const { return index_ <=> rhs.index_; }

  /// @member_fn_hash
  inline size_t hash() const { return hash_.get(); }

  /**
   * Returns a pretty type name.
   *
   * @return a pretty type name
   */
  const std::string& name() const { return name_.get(); }

private:

  const std::type_info& info_;
  const Lazy<std::string> name_;
  const std::type_index index_;
  const Lazy<size_t> hash_;
};

/// @op_output{#rocket::Type}
std::ostream& operator<<(std::ostream& lhs, const Type& rhs);

} // namespace rocket

// `fmt::formatter<Type>` -----------------------------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::Type}
 *
 * This formatter uses the same format specifiers as the underlying formatter for type `std::string`.
 */
template<typename C>
struct fmt::formatter<rocket::Type, C> {
  /// @cond undocumented

  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const rocket::Type& v, FormatContext& ctx) const {
    return underlying_.format(rocket::unicode::ConvertTo<C>().apply(v.name()), ctx);
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return underlying_.parse(ctx);
  }

  constexpr void
  set_debug_format(bool v = true) {
    underlying_.set_debug_format(v);
  }

  /// @endcond

private:

  fmt::formatter<basic_string_view<C>, C> underlying_;
};

// `std::hash<Type>` ----------------------------------------------------------------------------------------

/// @spec_std_hash{#rocket::Type}
template<>
struct std::hash<rocket::Type> {
  /// @cond undocumented

  inline size_t operator()(const rocket::Type& v) const { return v.hash(); }

  /// @endcond
};

// EOF
