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

#include <iosfwd>

// Global namespace -----------------------------------------------------------------------------------------

/**
 * The fabulous furry `B` class.
 */
struct B {};

/**
 * The irresistable `C` class, packed with content.
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
 * - #myFunc()
 * - #myFunc(int)
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
using MyType = int;

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
  int a;
  /// Not the same as #a.
  int b;
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
 * myFunc() without arguments.
 */
void myFunc();

/**
 * myFunc() with one argument.
 *
 * @param n text
 */
void myFunc(int n);

/**
 * myFuncTemplate() with one argument.
 *
 * @param v text
 */
template<typename T>
void myFuncTemplate(T v);

/**
 * myFuncTemplate() with two arguments.
 *
 * @param v text
 * @param w text
 */
template<typename T>
void myFuncTemplate(T v, T w);

// Namespace `foo::bar` -------------------------------------------------------------------------------------

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
 * - #foo::bar::fooMyFunc()
 * - #foo::bar::fooMyFunc(int)
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
using FooMyType = int;

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
  int a;
  /// Not the same as #a.
  int b;
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
 * fooMyFunc() without arguments.
 */
void fooMyFunc();

/**
 * fooMyFunc() with one argument.
 *
 * @param n text
 */
void fooMyFunc(int n);

/**
 * fooMyFuncTemplate() with one argument.
 *
 * @param v text
 */
template<typename T>
void fooMyFuncTemplate(T v);

/**
 * fooMyFuncTemplate() with two arguments.
 *
 * @param v text
 * @param w text
 */
template<typename T>
void fooMyFuncTemplate(T v, T w);

} // namespace foo::bar

// EOF
