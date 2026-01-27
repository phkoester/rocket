/**
 * @file literal.h
 *
 * Literal operators.
 */

#pragma once

#include "rocket/rocket.h"

#include <limits>
#include <type_traits>

namespace rocket {

namespace internal {

template<typename T>
struct SignedLimit {
  static_assert(std::is_signed_v<T>);
  static constexpr T value = std::numeric_limits<T>::max();
};

template<typename T>
struct UnsignedLimit {
  static_assert(std::is_unsigned_v<T>);
  static constexpr T value = std::numeric_limits<T>::max();
};

// Returns the value of the hexadecimal character
constexpr unsigned __int128 CharValue(char c) {
  return (c >= '0' && c <= '9')
              ? (c - '0')
              : ((c >= 'a' && c <= 'f') ? (10 + (c - 'a')) : (10 + (c - 'A')));
}

// ValidateU128Helper<BASE, c1, c2, ... , cn>(v) returns true iff cn + ... + c2
// * BASE^(n-2) + c1 * BASE^(n-1) + v * BASE^n is a valid 128-bit unsigned
// number when interpreted in base BASE.
template <int BASE>
constexpr bool ValidateU128Helper(unsigned __int128) {
  return true;
}

template <int BASE, char C, char... CS>
constexpr bool ValidateU128Helper(unsigned __int128 accumulate) {
  return (C == '\'') ? ValidateU128Helper<BASE, CS...>(accumulate)
                      : ((accumulate <= UnsignedLimit<u128>::value / BASE) &&
                        (BASE * accumulate <= UnsignedLimit<u128>::value - CharValue(C)) &&
                        ValidateU128Helper<BASE, CS...>(accumulate * BASE +
                                                        CharValue(C)));
}

// ValidateU128<BASE, c1, c2, ... , cn>(v) returns true iff cn + ... + c2 *
// BASE^(n-2) + c1 * BASE^(n-1) is a valid 128-bit unsigned number when
// interpreted in base BASE.
template <int BASE, char... CS>
constexpr bool ValidateU128() {
  return ValidateU128Helper<BASE, CS...>(0);
}

// MakeU128Helper<BASE, c1, c2, ... , cn>(v) returns cn + ... + c2 *
// BASE^(n-2) + c1 * BASE^(n-1) + result * BASE^n.
template <int BASE>
constexpr unsigned __int128 MakeU128Helper(unsigned __int128 result) {
  return result;
}

template <int BASE, char C, char... CS>
constexpr unsigned __int128 MakeU128Helper(unsigned __int128 result) {
  return MakeU128Helper<BASE, CS...>(
      (C == '\'') ? result : (result * BASE + CharValue(C)));
}

// MakeU128<BASE, c1, c2, ... , cn>(v) returns cn + ... + c2 * BASE^(n-2) + c1 *
// BASE^(n-1).
template <int BASE, char... CS>
constexpr unsigned __int128 MakeU128() {
  return MakeU128Helper<BASE, CS...>(0);
}

template <char... CS>
struct StaticUnsigned128 {
  static constexpr bool IS_VALID = ValidateU128<10, CS...>();
  static constexpr unsigned __int128 PAYLOAD = MakeU128<10, CS...>();
};

template <char... CS>
struct StaticUnsigned128<'0', 'x', CS...> {
  static constexpr bool IS_VALID = ValidateU128<16, CS...>();
  static constexpr unsigned __int128 PAYLOAD = MakeU128<16, CS...>();
};

template <char... CS>
struct StaticUnsigned128<'0', 'X', CS...> {
  static constexpr bool IS_VALID = ValidateU128<16, CS...>();
  static constexpr unsigned __int128 PAYLOAD = MakeU128<16, CS...>();
};

template <char... CS>
struct StaticUnsigned128<'0', 'b', CS...> {
  static constexpr bool IS_VALID = ValidateU128<2, CS...>();
  static constexpr unsigned __int128 PAYLOAD = MakeU128<2, CS...>();
};

template <char... CS>
struct StaticUnsigned128<'0', 'B', CS...> {
  static constexpr bool IS_VALID = ValidateU128<2, CS...>();
  static constexpr unsigned __int128 PAYLOAD = MakeU128<2, CS...>();
};

template <char... CS>
struct StaticUnsigned128<'0', CS...> {
  static constexpr bool IS_VALID = ValidateU128<8, CS...>();
  static constexpr unsigned __int128 PAYLOAD = MakeU128<8, CS...>();
};

template <char... CS>
struct StaticSigned128 {
  static constexpr bool IS_VALID =
      ValidateU128<10, CS...>() && (MakeU128<10, CS...>() <= SignedLimit<__int128>::value);
  static constexpr __int128 PAYLOAD =
      static_cast<__int128>(MakeU128<10, CS...>());
};

template <char... CS>
struct StaticSigned128<'0', 'x', CS...> {
  static constexpr bool IS_VALID =
      ValidateU128<16, CS...>() && (MakeU128<16, CS...>() <= SignedLimit<__int128>::value);
  static constexpr __int128 PAYLOAD =
      static_cast<__int128>(MakeU128<16, CS...>());
};

template <char... CS>
struct StaticSigned128<'0', 'X', CS...> {
  static constexpr bool IS_VALID =
      ValidateU128<16, CS...>() && (MakeU128<16, CS...>() <= SignedLimit<__int128>::value);
  static constexpr __int128 PAYLOAD =
      static_cast<__int128>(MakeU128<16, CS...>());
};

template <char... CS>
struct StaticSigned128<'0', 'b', CS...> {
  static constexpr bool IS_VALID =
      ValidateU128<2, CS...>() && (MakeU128<2, CS...>() <= SignedLimit<__int128>::value);
  static constexpr __int128 PAYLOAD =
      static_cast<__int128>(MakeU128<2, CS...>());
};

template <char... CS>
struct StaticSigned128<'0', 'B', CS...> {
  static constexpr bool IS_VALID =
      ValidateU128<2, CS...>() && (MakeU128<2, CS...>() <= SignedLimit<__int128>::value);
  static constexpr __int128 PAYLOAD =
      static_cast<__int128>(MakeU128<2, CS...>());
};

template <char... CS>
struct StaticSigned128<'0', CS...> {
  static constexpr bool IS_VALID =
      ValidateU128<8, CS...>() && (MakeU128<8, CS...>() <= SignedLimit<__int128>::value);
  static constexpr __int128 PAYLOAD =
      static_cast<__int128>(MakeU128<8, CS...>());
};

} // namespace internal

template <char... CS>
constexpr i128 operator""_i128() {
  static_assert(internal::StaticSigned128<CS...>::IS_VALID,
                "Invalid characters or number too large");
  return internal::StaticSigned128<CS...>::PAYLOAD;
}

template <char... CS>
constexpr u128 operator""_u128() {
  static_assert(internal::StaticUnsigned128<CS...>::IS_VALID,
                "Invalid characters or number too large");
  return internal::StaticUnsigned128<CS...>::PAYLOAD;
}

} // namespace rocket

// EOF
