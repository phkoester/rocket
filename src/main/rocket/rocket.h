/**
 * @file rocket.h
 *
 * Rocket base header. Contains basic types and concepts.
 *
 * Size in bytes of basic types on all supported target systems:
 *
 * | Type          | `linux`
 * | :------------ | ------:
 * | `char`        |       1
 * | `std::byte`   |       1
 * | `short`       |       2
 * | `wchar_t`     |       4
 * | `char32_t`    |       4
 * | `int`         |       4
 * | `float`       |       4
 * | `long`        |       8
 * | `long long`   |       8
 * | `size_t`      |       8
 * | `double`      |       8
 * | `void*`       |       8
 * | `int128_t`    |      16
 * | `long double` |      16
 *
 * In Rocket, C strings of type `char*` and instances of `std::string` or `std::string_view` are assumed to
 * be UTF-8-encoded. This is already true at compile time: A string literal like `"ä"` must expand to
 * `"\xc3\xa4"`. Therefore, the `char8_t` from C++20 is never in use.
 *
 * The size of `wchar_t` is platform-dependent. On Linux, a `wchar_t` is usually 4 bytes wide, whereas on
 * Windows, it is only 2 bytes wide. So there is no guarantee a `wchar_t` can hold a Unicode code point. For
 * UTF-32 encoding, Rocket uses the `char32_t` type from C++11.
 *
 * The only Unicode encodings that Rocket supports are UTF-8 and UTF-32.
 */

#pragma once

// No Rocket includes allowed here!

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iosfwd>
#include <optional>
#include <type_traits>

// Macros ---------------------------------------------------------------------------------------------------

#ifdef GAIA_CXX_TOOLCHAIN_GNU
  #define ROCKET_COMPILER_GNU 1
#endif

#ifdef GAIA_CXX_TOOLCHAIN_LLVM
  #define ROCKET_COMPILER_LLVM 1
#endif

#if not defined(ROCKET_COMPILER_GNU) && not defined(ROCKET_COMPILER_LLVM)
  #error Unknown compiler
#endif

// `int128_t` -----------------------------------------------------------------------------------------------

/**
 * A signed 128-bit integer type.
 *
 * - Minimum value: -170141183460469231731687303715884105728
 * - Maximum value:  170141183460469231731687303715884105727
 */
using int128_t = __int128;

/// @op_output{#int128_t}
std::ostream& operator<<(std::ostream& lhs, int128_t rhs);

/// @op_input{#int128_t}
std::istream& operator>>(std::istream& lhs, int128_t& rhs);

// `uint128_t` ----------------------------------------------------------------------------------------------

/**
 * An unsigned 128-bit integer type.
 *
 * - Maximum value: 340282366920938463463374607431768211455
 */
using uint128_t = unsigned __int128;

/// @op_input{#uint128_t}
std::istream& operator>>(std::istream& lhs, uint128_t& rhs);

/// @op_output{#uint128_t}
std::ostream& operator<<(std::ostream& lhs, uint128_t rhs);

namespace rocket {

// Constants ------------------------------------------------------------------------------------------------

/**
 * The default precision for floating-point I/O.
 */
constexpr int DEFAULT_PRECISION = 20;

/**
 * A constant for "not a position".
 */
constexpr size_t NPOS = -1L;

// `Char` ---------------------------------------------------------------------------------------------------

template<int N> struct Char;

/**
 * One-byte character: `char`.
 */
template<> struct Char<1> {
  using Type = char; ///< @type_alias
};

/**
 * Four-byte character: `char32_t`.
 */
template<> struct Char<4> {
  using Type = char32_t; ///< @type_alias
};

// `Int` ----------------------------------------------------------------------------------------------------

template<int N> struct Int;

/**
 * One-byte signed integer: `int8_t`.
 */
template<> struct Int<1> {
  using Type = int8_t; ///< @type_alias
};

/**
 * Two-byte signed integer: `int16_t`.
 */
template<> struct Int<2> {
  using Type = int16_t; ///< @type_alias
};

/**
 * Four-byte signed integer: `int32_t`.
 */
template<> struct Int<4> {
  using Type = int32_t; ///< @type_alias
};

/**
 * Eight-byte signed integer: `int64_t`.
 */
template<> struct Int<8> {
  using Type = int64_t; ///< @type_alias
};

/**
 * Sixteen-byte signed integer: `int128_t`.
 */
template<> struct Int<16> {
  using Type = int128_t; ///< @type_alias
};

// `Uint` ---------------------------------------------------------------------------------------------------

template<int N> struct Uint;

/**
 * One-byte unsigned integer: `uint8_t`.
 */
template<> struct Uint<1> {
  using Type = uint8_t; ///< @type_alias
};

/**
 * Two-byte unsigned integer: `uint16_t`.
 */
template<> struct Uint<2> {
  using Type = uint16_t; ///< @type_alias
};

/**
 * Four-byte unsigned integer: `uint32_t`.
 */
template<> struct Uint<4> {
  using Type = uint32_t; ///< @type_alias
};

/**
 * Eight-byte unsigned integer: `uint64_t`.
 */
template<> struct Uint<8> {
  using Type = uint64_t; ///< @type_alias
};

/**
 * Sixteen-byte unsigned integer: `uint128_t`.
 */
template<> struct Uint<16> {
  using Type = uint128_t; ///< @type_alias
};

// `Float` --------------------------------------------------------------------------------------------------

template<int N> struct Float;

/**
 * Four-byte floating point: `float`.
 *
 * @todo Find a portable way to express a four-byte floating point.
 */
template<> struct Float<4> {
  using Type = float; ///< @type_alias
};

static_assert(sizeof(Float<4>::Type) == 4);

/**
 * Eight-byte floating point: `double`.
 *
 * @todo Find a portable way to express an eight-byte floating point.
 */
template<> struct Float<8> {
  using Type = double; ///< @type_alias
};

static_assert(sizeof(Float<8>::Type) == 8);

/**
 * Sixteen-byte floating point: `long double`.
 *
 * @todo Find a portable way to express a sixteen-byte floating point.
 */
template<> struct Float<16> {
  using Type = long double; ///< @type_alias
};

static_assert(sizeof(Float<16>::Type) == 16);

// `Character` ----------------------------------------------------------------------------------------------

namespace internal {

template<typename> struct IsCharacterImpl : std::false_type {};

template<> struct IsCharacterImpl<Char<1>::Type> : std::true_type {};
template<> struct IsCharacterImpl<Char<4>::Type> : std::true_type {};

} // namespace internal

/**
 * IsCharacter template.
 */
template<typename C> struct IsCharacter : internal::IsCharacterImpl<std::decay_t<C>>::type {};

/**
 * A basic concept for types that are onsidered characters.
 *
 * These are
 *
 * - `char` (UTF-8)
 * - `char32_t` (UTF-32)
 *
 * For more information, read the documentation for this file.
 *
 * @tparam C the type to test
 */
template<typename C> concept Character = IsCharacter<C>::value;

// `Integer` ------------------------------------------------------------------------------------------------

namespace internal {

template<typename> struct IsIntegerImpl : std::false_type {};

template<> struct IsIntegerImpl<Int<1>::Type> : std::true_type {};
template<> struct IsIntegerImpl<Int<2>::Type> : std::true_type {};
template<> struct IsIntegerImpl<Int<4>::Type> : std::true_type {};
template<> struct IsIntegerImpl<Int<8>::Type> : std::true_type {};
template<> struct IsIntegerImpl<Int<16>::Type> : std::true_type {};

template<> struct IsIntegerImpl<Uint<1>::Type> : std::true_type {};
template<> struct IsIntegerImpl<Uint<2>::Type> : std::true_type {};
template<> struct IsIntegerImpl<Uint<4>::Type> : std::true_type {};
template<> struct IsIntegerImpl<Uint<8>::Type> : std::true_type {};
template<> struct IsIntegerImpl<Uint<16>::Type> : std::true_type {};

} // namespace internal

/**
 * `IsInteger` template.
 *
 * @tparam I the type to test
 */
template<typename I> struct IsInteger : internal::IsIntegerImpl<std::decay_t<I>>::type {};

/**
 * A basic concept for types that are considered integers.
 *
 * These are:
 *
 * - `int16_t`, `int32_t`, `int64_t`, `int128_t`
 * - `uint16_t`, `uint32_t`, `uint64_t`, `uint128_t`
 *
 * @tparam I the type to test
 */
template<typename I> concept Integer = IsInteger<I>::value;

// `FloatingPoint` ------------------------------------------------------------------------------------------

namespace internal {

template<typename> struct IsFloatingPointImpl : std::false_type {};

template<> struct IsFloatingPointImpl<Float<4>::Type> : std::true_type {};
template<> struct IsFloatingPointImpl<Float<8>::Type> : std::true_type {};
template<> struct IsFloatingPointImpl<Float<16>::Type> : std::true_type {};

} // namespace internal

/**
 * `IsFloatingPoint` template.
 *
 * @tparam F the type to test
 */
template<typename F> struct IsFloatingPoint : internal::IsFloatingPointImpl<std::decay_t<F>>::type {};

/**
 * A basic concept for types that are considered floating-points.
 *
 * These are:
 *
 * - `float`
 * - `double`
 * - `long double`
 *
 * @tparam F the type to test
 */
template<typename F> concept FloatingPoint = IsFloatingPoint<F>::value;

// Functions ------------------------------------------------------------------------------------------------

/**
 * A NOP function that helps to suppress warnings about unused variables.
 *
 * @tparam T the types of @p args
 * @param args the variables to suprress warnings for
 */
template<typename... T>
constexpr void nop(T&&... args) {}

/**
 * The `option` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or `std::optional<T>`
 * @return an optional value. If the value is to be taken from a `std::optional<T>`, that optional is
 *    returned directly, otherwise a nonnull optional
 */
template<typename T>
constexpr std::optional<T>
option(const T& v) {
  return std::optional<T>(v);
}

/**
 * The `option` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or `std::optional<T>`
 * @return an optional value. If the value is to be taken from a `std::optional<T>`, that optional is
 *    returned directly, otherwise a nonnull optional
 */
template<typename T>
constexpr std::optional<T>&
option(std::optional<T>& v) {
  return v;
}

/**
 * The `option` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or `std::optional<T>`
 * @return an optional value. If the value is to be taken from a `std::optional<T>`, that optional is
 *    returned directly, otherwise a nonnull optional
 */
template<typename T>
constexpr const std::optional<T>&
option(const std::optional<T>& v) {
  return v;
}

/**
 * Returns `true` if @p v is a quiet not-a-number.
 *
 * @tparam F the floating-point type
 * @param v a floating-point value
 * @return `true` if @p v is a quiet not-a-number
 */
template<typename F> requires FloatingPoint<F>
constexpr bool
quietNan(F v) {
  return std::isnan(v) && not issignaling(v);
}

/**
 * Returns `true` if @p v is a signaling not-a-number.
 *
 * @tparam F the floating-point type
 * @param v a floating-point value
 * @return `true` if @p v is a signaling not-a-number
 */
template<typename F> requires FloatingPoint<F>
constexpr bool
signalingNan(F v) {
  return std::isnan(v) && issignaling(v);
}

/**
 * The `value` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or `std::optional<T>`
 * @return a value. If the value is to be taken from a `std::optional<T>`, that optional is dereferenced,
 *     otherwise the value is returned directly
 */
template<typename T>
constexpr T&
value(T& v) {
  return v;
}

/**
 * The `value` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or `std::optional<T>`
 * @return a value. If the value is to be taken from a `std::optional<T>`, that optional is dereferenced,
 *     otherwise the value is returned directly
 */
template<typename T>
constexpr const T&
value(const T& v) {
  return v;
}

/**
 * The `value` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or `std::optional<T>`
 * @return a value. If the value is to be taken from a `std::optional<T>`, that optional is dereferenced,
 *     otherwise the value is returned directly
 */
template<typename T>
constexpr T&
value(std::optional<T>& v) {
  return *v;
}

/**
 * The `value` function has several overloads to work with values either of type @p T or `std::optional<T>`.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or `std::optional<T>`
 * @return a value. If the value is to be taken from a `std::optional<T>`, that optional is dereferenced,
 *     otherwise the value is returned directly
 */
template<typename T>
constexpr const T&
value(const std::optional<T>& v) {
  return *v;
}

} // namespace rocket

// EOF
