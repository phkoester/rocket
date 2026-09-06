/**
 * @file test-doxygen.h
 *
 * This is a Doxygen test.
 *
 * Let's see a clickable dot graph:
 *
 * @dot
 * digraph example {
 *   node [shape=record, fontname=Helvetica, fontsize=10];
 *   b [label="class B" URL="@ref B"];
 *   c [label="class C" URL="@ref C"];
 *   c -> b [arrowhead="open", style="dashed"];
 * }
 * @enddot
 *
 * How about some math:
 *
 * @f[
 * |I_2|=\left| \int_{0}^T \psi(t)
 *         \left\{
 *           u(a,t)-
 *           \int_{\gamma(t)}^a
 *           \frac{d\theta}{k(\theta,t)}
 *           \int_{a}^\theta c(\xi)u_t(\xi,t)\,d\xi
 *         \right\} dt
 *       \right|
 * @f]
 *
 * @par Title
 * A paragraph with a title.
 *
 * Another paragraph.
 *
 * @attention An attention block.
 * @par
 * And some text. This should be part of the attention block.
 *
 * Yet another paragraph.
 */

#pragma once

#include "rocket/rocket.h"

#include <iosfwd>

// Global namespace -----------------------------------------------------------------------------------------

/**
 * The fabulous furry #B class.
 */
struct B {};

/**
 * The irresistable #C class, packed with content.
 */
struct C : B {};

/**
 * This comment tests Doxygen's link generation.
 *
 * - #MY_CONSTANT
 * - #MY_MACRO
 * - #MyType
 * - #MyEnum
 * - #red
 * - #MyEnumClass
 * - #MyEnumClass#one
 * - #MyClass
 * - #MyClass#a
 * - #MyClassTemplate
 * - #MyClassTemplate#a
 * - #myFunc
 * - #myFunc(i32)
 * - #myFuncTemplate(T)
 * - #myFuncTemplate(T, T)
 */
void before();

/**
 * A simple constant.
 */
constexpr auto MY_CONSTANT = 0;

/**
 * A simple macro.
 */
#define MY_MACRO myFunc

/**
 * A simple type.
 */
using MyType = i32;

/**
 * A simple enum.
 */
enum MyEnum {
  red, ///< Red.
  green, ///< Green.
  blue ///< Blue.
};

/**
 * A simple enum class.
 */
enum class MyEnumClass {
  one, ///< One.
  two, ///< Two.
  three ///< Three.
};

/**
 * A simple class.
 */
struct MyClass {
  /// Not the same as #b.
  i32 a;
  /// Not the same as #a.
  i32 b;
};

/// @op_output{#MyClass}
std::ostream& operator<<(std::ostream& lhs, const MyClass& rhs);

/**
 * A simple class template.
 */
template<typename T>
struct MyClassTemplate {
  /// A `T`.
  T a;
};

/**
 * #myFunc without arguments.
 */
void myFunc();

/**
 * #myFunc(i32) with one argument.
 *
 * @param n text
 */
void myFunc(i32 n);

/**
 * #myFuncTemplate(T) with one argument.
 *
 * @param val text
 */
template<typename T>
void myFuncTemplate(T val);

/**
 * #myFuncTemplate(T, T) with two arguments.
 *
 * @param val text
 * @param wal text
 */
template<typename T>
void myFuncTemplate(T val, T wal);

// `foo::bar` -----------------------------------------------------------------------------------------------

namespace foo::bar {

/**
 * This comment tests Doxygen's link generation.
 *
 * - #foo::bar::FOO_MY_CONSTANT
 * - #foo::bar::FooMyType
 * - #foo::bar::FooMyEnum
 * - #foo::bar::red
 * - #foo::bar::FooMyEnumClass
 * - #foo::bar::FooMyEnumClass#one
 * - #foo::bar::FooMyClass
 * - #foo::bar::FooMyClass#a
 * - #foo::bar::FooMyClassTemplate
 * - #foo::bar::FooMyClassTemplate#a
 * - #foo::bar::fooMyFunc
 * - #foo::bar::fooMyFunc(i32)
 * - #foo::bar::fooMyFuncTemplate(T)
 * - #foo::bar::fooMyFuncTemplate(T, T)
 */
void fooBefore();

/**
 * A simple constant.
 */
constexpr auto FOO_MY_CONSTANT = 0;

/**
 * A simple type.
 */
using FooMyType = i32;

/**
 * A simple enum.
 */
enum FooMyEnum {
  red, ///< Red.
  green, ///< Green.
  blue ///< Blue.
};

/**
 * A simple enum class.
 */
enum class FooMyEnumClass {
  one, ///< One.
  two, ///< Two.
  three ///< Three.
};

/**
 * A simple class.
 */
struct FooMyClass {
  /// Not the same as #b.
  i32 a;
  /// Not the same as #a.
  i32 b;
};

/// @op_output{#foo::bar::FooMyClass}
std::ostream& operator<<(std::ostream& lhs, const FooMyClass& rhs);

/**
 * A simple class template.
 */
template<typename T>
struct FooMyClassTemplate {
  /// A `T`.
  T a;
};

/**
 * #foo::bar::fooMyFunc without arguments.
 */
void fooMyFunc();

/**
 * #foo::bar::fooMyFunc(i32) with one argument.
 *
 * @param n text
 */
void fooMyFunc(i32 n);

/**
 * #foo::bar::fooMyFuncTemplate(T) with one argument.
 *
 * @param val text
 */
template<typename T>
void fooMyFuncTemplate(T val);

/**
 * #foo::bar::fooMyFuncTemplate(T, T) with two arguments.
 *
 * @param val text
 * @param wal text
 */
template<typename T>
void fooMyFuncTemplate(T val, T wal);

} // namespace foo::bar

// EOF
