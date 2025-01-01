/**
 * @file basic.h
 *
 * Basic types and concepts.
 *
 * Size in bytes of basic types on all supported target systems:
 *
 * | Type                 | @c linux |
 * |----------------------|---------:|
 * | @c char              |        1 |
 * | @c std::byte         |        1 |
 * | @c short             |        2 |
 * | @c wchar_t           |        4 |
 * | @c char32_t          |        4 |
 * | @c int               |        4 |
 * | @c float             |        4 |
 * | @c long              |        8 |
 * | <tt>long long</tt>   |        8 |
 * | @c size_t            |        8 |
 * | @c double            |        8 |
 * | @c void*             |        8 |
 * | #int128_t            |       16 |
 * | <tt>long double</tt> |       16 |
 *
 * In Rocket, C strings of type @c char* and instances of @c std::string or @c std::string_view are assumed
 * to be UTF-8-encoded. This is already true at compile time: A string literal like "ä" must expand to
 * @c "\xc3\xa4". Therefore, the @c char8_t from C++20 is never in use.
 *
 * The size of @c wchar_t is platform-dependent. On Linux, a @c wchar_t is usually 4 bytes wide, whereas on
 * Windows, it is only 2 bytes wide. So there is no guarantee a @c wchar_t can hold a Unicode code point. For
 * UTF-32 encoding, Rocket uses the @c char32_t type from C++11.
 *
 * The only Unicode encodings that Rocket supports are UTF-8 and UTF-32.
 */

#pragma once

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

// 'int128_t' -----------------------------------------------------------------------------------------------

/**
 * A signed 128-bit integer type.
 */
using int128_t = __int128;

/// @op_input{#int128_t}
std::istream& operator>>(std::istream& lhs, int128_t& rhs);

/// @op_output{#int128_t}
std::ostream& operator<<(std::ostream& lhs, int128_t rhs);

// 'uint128_t' ----------------------------------------------------------------------------------------------

/**
 * An unsigned 128-bit integer type.
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

// 'Char' ---------------------------------------------------------------------------------------------------

template<int N> struct Char;

/**
 * One-byte character: @c char.
 */
template<> struct Char<1> {
  using Type = char; ///< @type_alias
};

/**
 * Four-byte character: @c char32_t.
 */
template<> struct Char<4> {
  using Type = char32_t; ///< @type_alias
};

// 'Int' ----------------------------------------------------------------------------------------------------

template<int N> struct Int;

/**
 * One-byte signed integer: @c int8_t.
 */
template<> struct Int<1> {
  using Type = int8_t; ///< @type_alias
};

/**
 * Two-byte signed integer: @c int16_t.
 */
template<> struct Int<2> {
  using Type = int16_t; ///< @type_alias
};

/**
 * Four-byte signed integer: @c int32_t.
 */
template<> struct Int<4> {
  using Type = int32_t; ///< @type_alias
};

/**
 * Eight-byte signed integer: @c int64_t.
 */
template<> struct Int<8> {
  using Type = int64_t; ///< @type_alias
};

/**
 * Sixteen-byte signed integer: @c int128_t.
 */
template<> struct Int<16> {
  using Type = int128_t; ///< @type_alias
};

// 'Uint' ---------------------------------------------------------------------------------------------------

template<int N> struct Uint;

/**
 * One-byte unsigned integer: @c uint8_t.
 */
template<> struct Uint<1> {
  using Type = uint8_t; ///< @type_alias
};

/**
 * Two-byte unsigned integer: @c uint16_t.
 */
template<> struct Uint<2> {
  using Type = uint16_t; ///< @type_alias
};

/**
 * Four-byte unsigned integer: @c uint32_t.
 */
template<> struct Uint<4> {
  using Type = uint32_t; ///< @type_alias
};

/**
 * Eight-byte unsigned integer: @c uint64_t.
 */
template<> struct Uint<8> {
  using Type = uint64_t; ///< @type_alias
};

/**
 * Sixteen-byte unsigned integer: @c uint128_t.
 */
template<> struct Uint<16> {
  using Type = uint128_t; ///< @type_alias
};

// 'Float' --------------------------------------------------------------------------------------------------

template<int N> struct Float;

/**
 * Four-byte floating point: @c float.
 *
 * @todo Find a portable way to express a four-byte floating point.
 */
template<> struct Float<4> {
  using Type = float; ///< @type_alias
};

static_assert(sizeof(Float<4>::Type) == 4);

/**
 * Eight-byte floating point: @c double.
 *
 * @todo Find a portable way to express an eight-byte floating point.
 */
template<> struct Float<8> {
  using Type = double; ///< @type_alias
};

static_assert(sizeof(Float<8>::Type) == 8);

/**
 * Sixteen-byte floating point: <tt>long double</tt>.
 *
 * @todo Find a portable way to express a sixteen-byte floating point.
 */
template<> struct Float<16> {
  using Type = long double; ///< @type_alias
};

static_assert(sizeof(Float<16>::Type) == 16);

// 'Character' ----------------------------------------------------------------------------------------------

namespace internal {

template<typename> struct IsCharacterImpl : std::false_type {};

template<> struct IsCharacterImpl<Char<1>::Type> : std::true_type {};
template<> struct IsCharacterImpl<Char<4>::Type> : std::true_type {};

} // namespace internal

/**
 * @c IsCharacter template.
 */
template<typename C> struct IsCharacter : internal::IsCharacterImpl<std::decay_t<C>>::type {};

/**
 * A basic concept for types that are onsidered characters.
 *
 * These are
 *
 * - @c char (UTF-8)
 * - @c char32_t (UTF-32)
 *
 * For more information, read the documentation for this file.
 *
 * @tparam C the type to test
 */
template<typename C> concept Character = IsCharacter<C>::value;

// 'Integer' ------------------------------------------------------------------------------------------------

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
 * @c IsInteger template.
 *
 * @tparam I the type to test
 */
template<typename I> struct IsInteger : internal::IsIntegerImpl<std::decay_t<I>>::type {};

/**
 * A basic concept for types that are considered integers.
 *
 * These are:
 *
 * - @c @c int16_t, @c int32_t, @c int64_t, @c int128_t
 * - @c @c uint16_t, @c uint32_t, @c uint64_t, @c uint128_t
 *
 * @tparam I the type to test
 */
template<typename I> concept Integer = IsInteger<I>::value;

// 'FloatingPoint' ------------------------------------------------------------------------------------------

namespace internal {

template<typename> struct IsFloatingPointImpl : std::false_type {};

template<> struct IsFloatingPointImpl<Float<4>::Type> : std::true_type {};
template<> struct IsFloatingPointImpl<Float<8>::Type> : std::true_type {};
template<> struct IsFloatingPointImpl<Float<16>::Type> : std::true_type {};

} // namespace internal

/**
 * @c IsFloatingPoint template.
 *
 * @tparam F the type to test
 */
template<typename F> struct IsFloatingPoint : internal::IsFloatingPointImpl<std::decay_t<F>>::type {};

/**
 * A basic concept for types that are considered floating-points.
 *
 * These are:
 *
 * - @c float
 * - @c double
 * - <tt>long double</tt>
 *
 * @tparam F the type to test
 */
template<typename F> concept FloatingPoint = IsFloatingPoint<F>::value;

// Functions ------------------------------------------------------------------------------------------------

/**
 * A NOP function.
 */
inline void nop() {}

/**
 * The @c option() function has several overloads to work with values either of type @p T or
 * @c std::optional<T>.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or @c std::optional<T>
 * @return an optional value. If the value is to be taken from a @c std::optional<T>, that optional is
 *    returned directly, otherwise a nonnull optional
 */
template<typename T>
constexpr std::optional<T>
option(const T& v) {
  return std::optional<T>(v);  
}

/**
 * The @c option() function has several overloads to work with values either of type @p T or
 * @c std::optional<T>.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or @c std::optional<T>
 * @return an optional value. If the value is to be taken from a @c std::optional<T>, that optional is
 *    returned directly, otherwise a nonnull optional
 */
template<typename T>
constexpr std::optional<T>&
option(std::optional<T>& v) {
  return v;
}

/**
 * The @c option() function has several overloads to work with values either of type @p T or
 * @c std::optional<T>.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or @c std::optional<T>
 * @return an optional value. If the value is to be taken from a @c std::optional<T>, that optional is
 *    returned directly, otherwise a nonnull optional
 */
template<typename T>
constexpr const std::optional<T>&
option(const std::optional<T>& v) {
  return v;
}

/**
 * Tells if @p v is a quiet not-a-number.
 *
 * @tparam F the floating-point type
 * @param v a floating-point value
 * @return @c true iff @p v is a quiet not-a-number
 */
template<typename F> requires FloatingPoint<F>
constexpr bool
quietNan(F v) {
  return std::isnan(v) && not issignaling(v);
}

/**
 * Tells if @p v is a signaling not-a-number.
 *
 * @tparam F the floating-point type
 * @param v a floating-point value
 * @return @c true iff @p v is a signaling not-a-number
 */
template<typename F> requires FloatingPoint<F>
constexpr bool
signalingNan(F v) {
  return std::isnan(v) && issignaling(v);
}

/**
 * Obtains an unsigned integer value for @p v that has the same size in bytes as @p v.
 *
 * This may be useful to quickly print hex values:
 *
 * @code{.cc}
 * auto v = ...
 * cout << hex << rocket::uint(v) << '\n';
 * @endcode
 *
 * @tparam T the type of @p v
 * @param v a value
 * @return an unsigned integer value for @p that has the same size in bytes as @p v
 */
template<typename T>
constexpr typename Uint<sizeof(T)>::Type
uint(T v) {
  typename Uint<sizeof(T)>::Type result;
  std::memcpy(&result, &v, sizeof(T));
  return result;
}

/**
 * A NOP function that helps to suppress warnings about unused variables.
 *
 * @tparam T the types of @p args
 * @param args the variables to suprress warnings for
 */
template<typename... T>
constexpr void use(T&&... args) {}

/**
 * The @c value() function has several overloads to work with values either of type @p T or
 * @c std::optional<T>.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or @c std::optional<T>
 * @return a value. If the value is to be taken from a @c std::optional<T>, that optional is dereferenced,
 *     otherwise the value is returned directly
 */
template<typename T>
constexpr T&
value(T& v) {
  return v;
}

/**
 * The @c value() function has several overloads to work with values either of type @p T or
 * @c std::optional<T>.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or @c std::optional<T>
 * @return a value. If the value is to be taken from a @c std::optional<T>, that optional is dereferenced,
 *     otherwise the value is returned directly
 */
template<typename T>
constexpr const T&
value(const T& v) {
  return v;
}

/**
 * The @c value() function has several overloads to work with values either of type @p T or
 * @c std::optional<T>.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or @c std::optional<T>
 * @return a value. If the value is to be taken from a @c std::optional<T>, that optional is dereferenced,
 *     otherwise the value is returned directly
 */
template<typename T>
constexpr T&
value(std::optional<T>& v) {
  return *v;
}

/**
 * The @c value() function has several overloads to work with values either of type @p T or
 * @c std::optional<T>.
 *
 * @tparam T the value type
 * @param v a value; either of type @p T or @c std::optional<T>
 * @return a value. If the value is to be taken from a @c std::optional<T>, that optional is dereferenced,
 *     otherwise the value is returned directly
 */
template<typename T>
constexpr const T&
value(const std::optional<T>& v) {
  return *v;
}

} // namespace rocket

// EOF
