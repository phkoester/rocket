/**
 * @file VarRef.h
 *
 * C++ reflection: variable references.
 */

#pragma once

#include "rocket/unicode/ConvertTo.h"

#include <fmt/std.h>

#include <ostream>

namespace rocket::reflect {

// #VarRef --------------------------------------------------------------------------------------------------

/**
 * References on variables that need need no instance to evaluate.
 *
 * Instances of this class are returned by #ROCKET_REFLECT_VARS.
 *
 * @param T the type of the variable
 */
template<typename T>
struct VarRef {
  using ValueType = T; ///< @type_alias

  /**
   * @ctor
   *
   * @param name the name of the variable
   * @param ref the reference to the variable
   */
  constexpr VarRef(const char* name, T& ref) : name_(name), ptr_(&ref) {}

  /// @member_op_eq
  // XXX Alles weg?
  bool operator==(const VarRef& rhs) const { return *ptr_ == *rhs.ptr_; }

  /// @member_op_ne
  bool operator!=(const VarRef& rhs) const { return *ptr_ != *rhs.ptr_; }

  /// @member_op_lt
  bool operator<(const VarRef& rhs) const { return *ptr_ < *rhs.ptr_; }

  /// @member_op_le
  bool operator<=(const VarRef& rhs) const { return *ptr_ <= *rhs.ptr_; }

  /// @member_op_gt
  bool operator>(const VarRef& rhs) const { return *ptr_ > rhs.*ptr_; }

  /// @member_op_ge
  bool operator>=(const VarRef& rhs) const { return *ptr_ >= *rhs.ptr_; }

  /**
   * Returns the value of the variable.
   *
   * @return the value of the variable
   */
  [[nodiscard]] constexpr T& get() { return *ptr_; }

  /**
   * Returns the value of the variable.
   *
   * @return the value of the variable
   */
  [[nodiscard]] constexpr const T& get() const { return *ptr_; }

  /**
   * Returns the name of the variable.
   *
   * @return the name of the variable
   */
  [[nodiscard]] constexpr std::string_view name() const { return name_; }

private:

  std::string_view name_;
  T* ptr_;
};

/// @op_output{#rocket::reflect::VarRef}
template<typename T>
inline std::ostream&
operator<<(std::ostream& lhs, const VarRef<T>& rhs) {
  return lhs << fmt::format("{}", rhs);
}

} // namespace rocket::reflect

// #fmt::formatter<#rocket::reflect::VarRef> ----------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::reflect::VarRef}
 *
 * This formatter uses the same format specifiers as the underlying formatter for type @ T.
 */
template<typename T, typename C> requires fmt::is_formattable<T, C>::value
struct fmt::formatter<rocket::reflect::VarRef<T>, C> {
  /// @cond undocumented

  template<typename FormatContext>
  constexpr FormatContext::iterator
  format(const rocket::reflect::VarRef<T>& val, FormatContext& ctx) const{
    auto out = ctx.out();
    out = detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(val.name()));
    out = detail::write<C>(out, static_cast<C>('='));
    ctx.advance_to(out);
    out = underlying_.format(val.get(), ctx);
    return out;
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    return underlying_.parse(ctx); // NOLINT
  }

  constexpr void
  set_debug_format(bool val = true) {
    detail::maybe_set_debug_format(underlying_, val); // NOLINT
  }

  /// @endcond

private:

  formatter<rocket::PurgeType<T>, C> underlying_;
};

// EOF
