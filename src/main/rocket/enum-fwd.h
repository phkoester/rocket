/// @file enum-fwd.h
///
/// `enum` forward declarations: the #rocket::Enum class template, without the macros that specialize it.

#pragma once

#include "rocket/Result.h"
#include "rocket/rocket.h"

#include <string_view>
#include <type_traits>
#include <utility>

namespace rocket {

// `Enum` ---------------------------------------------------------------------------------------------------

/// A class template for Rocket enums, providing some additional information about an enum.
template<typename E> requires std::is_enum_v<E>
struct Enum : std::false_type {
  /// Converts an enum value to a string.
  ///
  /// @param val the enum value
  /// @return a string
  /// @throw #rocket::InvalidState if the operation fails
  static std::string_view toString(E val);

  /// Scans a string to an enum value.
  ///
  /// @param str the string to scan
  /// @param strict whether the string must strictly match in its entirety. If this value is `true`, the
  ///   string can be scanned in a more efficient way
  /// @return a pair of the size of the scanned portion of the string and the enum value, or an error if the
  ///   operation fails
  static Result<std::pair<u64, E>> toType(std::string_view str, bool strict);
};

} // namespace rocket

// EOF
