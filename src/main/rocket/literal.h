/**
 * @file literal.h
 *
 * Literal operators.
 *
 * This integer implementation is based on https://github.com/jbapple/128-bit-literals.
 */

#pragma once

#include "rocket/rocket.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace rocket {

namespace internal {

template<typename T, typename U>
struct SignedLimit {
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_unsigned_v<U>);

  static constexpr U value = std::numeric_limits<T>::max();
};

template<typename T>
struct UnsignedLimit {
  static_assert(std::is_unsigned_v<T>);

  static constexpr T value = std::numeric_limits<T>::max();
};

/// Returns the value of the hexadecimal character @p c.
template<typename T>
constexpr T
charValue(char c) {
  return c >= '0' && c <= '9' ?
    (c - '0') :
    ((c >= 'a' && c <= 'f') ? (10 + (c - 'a')) : (10 + (c - 'A')));
}

/// Checks whether c_n + ... + c_2 * BASE^(n-2) + c_1 * BASE^(n-1) + v * BASE^n is a valid number when
// interpreted in base @p BASE.
template<typename T, int BASE>
constexpr bool
validateUnsignedImpl(T) {
  return true;
}

template<typename T, int BASE, char C, char... Chars>
constexpr bool
validateUnsignedImpl(T accumulate) {
  return C == '\'' ?
    validateUnsignedImpl<T, BASE, Chars...>(accumulate) :
    (accumulate <= UnsignedLimit<T>::value / BASE) &&
      (BASE * accumulate <= UnsignedLimit<T>::value - charValue<T>(C)) &&
      validateUnsignedImpl<T, BASE, Chars...>(accumulate * BASE + charValue<T>(C));
}

/// Checks whether c_n + ... + c_2 * BASE^(n-2) + c_1 * BASE^(n-1) is a valid unsigned number when
// interpreted in base @p BASE.
template<typename T,int BASE, char... Chars>
constexpr bool validateUnsigned() {
  static_assert(std::is_unsigned_v<T>);
  return validateUnsignedImpl<T, BASE, Chars...>(0);
}

/// Returns c_n + ... + c_2 * BASE^(n-2) + c_1 * BASE^(n-1) + result * BASE^n.
template<typename T, int BASE>
constexpr T
makeUnsignedImpl(T result) {
  return result;
}

template<typename T, int BASE, char C, char... Chars>
constexpr T
makeUnsignedImpl(T result) {
  return makeUnsignedImpl<T, BASE, Chars...>(C == '\'' ? result : (result * BASE + charValue<T>(C)));
}

/// Returns c_n + ... + c_2 * BASE^(n-2) + c_1 * BASE^(n-1).
template<typename T, int BASE, char... Chars>
constexpr T
makeUnsigned() {
  static_assert(std::is_unsigned_v<T>);
  return makeUnsignedImpl<T, BASE, Chars...>(0);
}

// `StaticSigned` -------------------------------------------------------------------------------------------

template<typename T, typename U, char... Chars>
struct StaticSigned {
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_unsigned_v<U>);

  static constexpr bool valid =
    validateUnsigned<U, 10, Chars...>() && (makeUnsigned<U, 10, Chars...>() <= SignedLimit<T, U>::value);

  static constexpr T payload = static_cast<T>(makeUnsigned<U, 10, Chars...>());
};

template<typename T, typename U, char... Chars>
struct StaticSigned<T, U, '0', 'x', Chars...> {
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_unsigned_v<U>);

  static constexpr bool valid =
      validateUnsigned<U, 16, Chars...>() && (makeUnsigned<U, 16, Chars...>() <= SignedLimit<T, U>::value);

  static constexpr T payload = static_cast<T>(makeUnsigned<U, 16, Chars...>());
};

template<typename T, typename U, char... Chars>
struct StaticSigned<T, U, '0', 'X', Chars...> {
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_unsigned_v<U>);

  static constexpr bool valid =
      validateUnsigned<T, 16, Chars...>() && (makeUnsigned<U, 16, Chars...>() <= SignedLimit<T, U>::value);

  static constexpr T payload = static_cast<T>(makeUnsigned<U,16, Chars...>());
};

template<typename T, typename U, char... Chars>
struct StaticSigned<T, U, '0', 'b', Chars...> {
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_unsigned_v<U>);

  static constexpr bool valid =
    validateUnsigned<T, 2, Chars...>() && (makeUnsigned<U, 2, Chars...>() <= SignedLimit<T, U>::value);

  static constexpr T payload = static_cast<T>(makeUnsigned<U, 2, Chars...>());
};

template<typename T, typename U, char... Chars>
struct StaticSigned<T, U, '0', 'B', Chars...> {
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_unsigned_v<U>);

  static constexpr bool valid =
      validateUnsigned<U, 2, Chars...>() && (makeUnsigned<U, 2, Chars...>() <= SignedLimit<T, U>::value);

  static constexpr T payload = static_cast<T>(makeUnsigned<U, 2, Chars...>());
};

template<typename T, typename U, char... Chars>
struct StaticSigned<T, U, '0', Chars...> {
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_unsigned_v<U>);

  static constexpr bool valid =
      validateUnsigned<U, 8, Chars...>() && (makeUnsigned<U, 8, Chars...>() <= SignedLimit<T, U>::value);

  static constexpr T payload = static_cast<T>(makeUnsigned<U, 8, Chars...>());
};

// `StaticUnsigned` -----------------------------------------------------------------------------------------

template<typename T, char... Chars>
struct StaticUnsigned {
  static_assert(std::is_unsigned_v<T>);

  static constexpr bool valid = validateUnsigned<T, 10, Chars...>();
  static constexpr T payload = makeUnsigned<T,10, Chars...>();
};

template<typename T, char... Chars>
struct StaticUnsigned<T, '0', 'x', Chars...> {
  static_assert(std::is_unsigned_v<T>);

  static constexpr bool valid = validateUnsigned<T,16, Chars...>();
  static constexpr T payload = makeUnsigned<T,16, Chars...>();
};

template<typename T, char... Chars>
struct StaticUnsigned<T, '0', 'X', Chars...> {
  static_assert(std::is_unsigned_v<T>);

  static constexpr bool valid = validateUnsigned<T, 16, Chars...>();
  static constexpr T payload = makeUnsigned<T, 16, Chars...>();
};

template<typename T, char... Chars>
struct StaticUnsigned<T, '0', 'b', Chars...> {
  static_assert(std::is_unsigned_v<T>);

  static constexpr bool valid = validateUnsigned<T, 2, Chars...>();
  static constexpr T payload = makeUnsigned<T, 2, Chars...>();
};

template<typename T, char... Chars>
struct StaticUnsigned<T, '0', 'B', Chars...> {
  static_assert(std::is_unsigned_v<T>);

  static constexpr bool valid = validateUnsigned<T, 2, Chars...>();
  static constexpr T payload = makeUnsigned<T,2, Chars...>();
};

template<typename T, char... Chars>
struct StaticUnsigned<T, '0', Chars...> {
  static_assert(std::is_unsigned_v<T>);

  static constexpr bool valid = validateUnsigned<T, 8, Chars...>();
  static constexpr T payload = makeUnsigned<T, 8, Chars...>();
};

} // namespace internal

/// 8-bit signed integer literal.
template<char... Chars>
constexpr i8 operator""_i8() {
  using type = internal::StaticSigned<i8, u8, Chars...>;
  static_assert(type::valid, "Invalid characters or number too large");
  return type::payload;
}

/// 8-bit unsigned integer literal.
template<char... Chars>
constexpr u8 operator""_u8() {
  using type = internal::StaticUnsigned<u8, Chars...>;
  static_assert(type::valid, "Invalid characters or number too large");
  return type::payload;
}

/// 16-bit signed integer literal.
template<char... Chars>
constexpr i16 operator""_i16() {
  using type = internal::StaticSigned<i16, u16, Chars...>;
  static_assert(type::valid, "Invalid characters or number too large");
  return type::payload;
}

/// 16-bit unsigned integer literal.
template<char... Chars>
constexpr u16 operator""_u16() {
  using type = internal::StaticUnsigned<u16, Chars...>;
  static_assert(type::valid, "Invalid characters or number too large");
  return type::payload;
}

/// 32-bit signed integer literal.
template<char... Chars>
constexpr i32 operator""_i32() {
  using type = internal::StaticSigned<i32, u32, Chars...>;
  static_assert(type::valid, "Invalid characters or number too large");
  return type::payload;
}

/// 32-bit unsigned integer literal.
template<char... Chars>
constexpr u32 operator""_u32() {
  using type = internal::StaticUnsigned<u32, Chars...>;
  static_assert(type::valid, "Invalid characters or number too large");
  return type::payload;
}

/// 64-bit signed integer literal.
template<char... Chars>
constexpr i64 operator""_i64() {
  using type = internal::StaticSigned<i64, u64, Chars...>;
  static_assert(type::valid, "Invalid characters or number too large");
  return type::payload;
}

/// 64-bit unsigned integer literal.
template<char... Chars>
constexpr u64 operator""_u64() {
  using type = internal::StaticUnsigned<u64, Chars...>;
  static_assert(type::valid, "Invalid characters or number too large");
  return type::payload;
}

#ifdef ROCKET_HAS_128

/// 128-bit signed integer literal.
template<char... Chars>
constexpr i128 operator""_i128() {
  using type = internal::StaticSigned<i128, u128, Chars...>;
  static_assert(type::valid, "Invalid characters or number too large");
  return type::payload;
}

/// 128-bit unsigned integer literal.
template<char... Chars>
constexpr u128 operator""_u128() {
  using type = internal::StaticUnsigned<u128, Chars...>;
  static_assert(type::valid, "Invalid characters or number too large");
  return type::payload;
}

#endif // ROCKET_HAS_128

/// 32-bit floating point literal.
f32 operator""_f32(std_long_double val);

/// 64-bit floating point literal.
f64 operator""_f64(std_long_double val);

#ifdef ROCKET_HAS_128

/// 128-bit floating point literal.
f128 operator""_f128(std_long_double val);

#endif // ROCKET_HAS_128

} // namespace rocket

// EOF
