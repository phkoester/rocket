// test-Cxx.cc
//
// Tests related to the C++ language itself.

#include "rocket-test/rocket-test.h"

#include "rocket-test/TracingString.h"

#include <boost/preprocessor/cat.hpp>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <stdexcept>

#define MODULE rocketExternalCxx
#define MODULE_DEATH_TEST BOOST_PP_CAT(MODULE, DeathTest)

namespace {

// Local functions ------------------------------------------------------------------------------------------

// A terminate handler that leaves a marker on `stderr`, proving that `std::terminate` was called
[[noreturn]] void
markingTerminateHandler() {
  std::fputs("`std::terminate` called\n", stderr);
  std::abort();
}

[[noreturn]] void
throwRuntimeError() {
  throw std::runtime_error("oops");
}

// Lets an exception escape although declared `noexcept`. The `throw` is in a separate function so that the
// compiler does not reject or warn about it (`-Wterminate`)
void
throwThroughNoexcept() noexcept { // NOLINT(bugprone-exception-escape): this is the point of the test
  throwRuntimeError();
}

} // namespace

// `MODULE` -------------------------------------------------------------------------------------------------

struct MODULE : Test {
  MODULE() { // NOLINT
    reset();
  }

  ~MODULE() override = default;

protected:

  struct Member {
    static bool dtorCalled;

    ~Member() {  dtorCalled = true; }
  };

  struct A {
    virtual ~A() = default;
  };

  struct B : A {
    Member m;
  };

  u64 fooLvalue;
  u64 fooRvalue;

  void
  SetUp() override {
    reset();
  }

  void
  foo(const i32&) { // NOLINT
    ++fooLvalue;
  }

  void
  foo(i32&&) { // NOLINT
    ++fooRvalue;
  }

  template<typename T>
  void
  bar(T&& val) {
    foo(std::forward<T>(val));
  }

  void
  baz(auto&& val) {
    foo(std::forward<decltype(val)>(val));
  }

  void
  reset() {
    fooLvalue = 0;
    fooRvalue = 0;

    TracingString::reset();
  }
};

bool MODULE::Member::dtorCalled = false;

// Tests ====================================================================================================

TEST_F(MODULE, assignLvalueToOptional) {
  // Assigning an lvalue to an empty `std::optional` copy-constructs the value in place. There is no default
  // construction followed by a copy assignment, and the source stays untouched
  string trace;
  {
    optional<TracingString> dest;
    const TracingString lvalue(trace, "a");
    dest = lvalue;
  }
  EXPECT_EQ(TracingString::NUM_INSTANCES, 0);
  EXPECT_EQ(trace,
      "1.ctorP: a\n"
      "2.ctorCopy: a\n"
      "1.dtor: a\n"
      "2.dtor: a\n");
}

TEST_F(MODULE, assignLvalueToOptionalWithMove) {
  // Assigning an rvalue (`std::move`) to an empty `std::optional` move-constructs the value in place. The
  // moved-from source is left in a valid but unspecified state
  string trace;
  {
    optional<TracingString> dest;
    TracingString lvalue(trace, "a");
    dest = std::move(lvalue);
  }
  EXPECT_EQ(TracingString::NUM_INSTANCES, 0);
  EXPECT_EQ(trace,
      "1.ctorP: a\n"
      "2.ctorMove: a\n"
      "1.dtor: invalid\n"
      "2.dtor: a\n");
}

TEST_F(MODULE, divideByZeroF64) {
  // The standard leaves floating-point division by zero undefined, but on IEEE 754 platforms (`is_iec559`)
  // it yields positive or negative infinity, depending on the sign of the dividend
  using type = f64;
  const type zero = 0;

  type n = 4.2;
  EXPECT_EQ(n / zero, numeric_limits<type>::infinity());

  n = -4.2;
  EXPECT_EQ(n / zero, -numeric_limits<type>::infinity());
}

TEST_F(MODULE, optionalEmplace) {
  // `std::optional::emplace` constructs the value directly from the constructor arguments, without a
  // temporary, a copy, or a move
  string trace;
  {
    optional<TracingString> dest;
    dest.emplace(trace, "a");
  }
  EXPECT_EQ(TracingString::NUM_INSTANCES, 0);
  EXPECT_EQ(trace,
      "1.ctorP: a\n"
      "1.dtor: a\n");
}

TEST_F(MODULE, utf8Identifier) {
  // Identifiers may contain non-ASCII letters (standardized in C++23 via Unicode `XID_Start` and
  // `XID_Continue`, P1949). The test passes if it compiles

  // German
  [[maybe_unused]] i32 _blöße; // NOLINT

  // French
  [[maybe_unused]] i32 _ça; // NOLINT

  // Japanese
 [[maybe_unused]] i32 _こんにちわ; // NOLINT

  // Chinese (traditional)
  [[maybe_unused]] i32 _你好; // NOLINT
}

TEST_F(MODULE, utf8StringLiteral) {
  // An ordinary string literal is stored in the literal encoding, which is UTF-8 here: `ä` takes the two
  // bytes C3 A4, and `size` counts bytes, not characters
  const auto sv = "ä"sv;
  EXPECT_EQ(sv, "\xc3\xa4");
  ASSERT_EQ(sv.size(), 2);
  EXPECT_EQ(sv[0], '\xc3');
  EXPECT_EQ(sv[1], '\xa4');
}

TEST_F(MODULE, vectorPushBackLvalue) {
  // `std::vector::push_back(const T&)` copy-constructs the element; the source stays untouched
  string trace;
  {
    vector<TracingString> vec;
    const TracingString lvalue(trace, "a");
    vec.push_back(lvalue);
  }
  EXPECT_EQ(TracingString::NUM_INSTANCES, 0);
  EXPECT_EQ(trace,
      "1.ctorP: a\n"
      "2.ctorCopy: a\n"
      "1.dtor: a\n"
      "2.dtor: a\n");
}

TEST_F(MODULE, vectorPushBackLvalueWithMove) {
  // `std::vector::push_back(T&&)` move-constructs the element, leaving the moved-from source in a valid but
  // unspecified state
  string trace;
  {
    vector<TracingString> vec;
    TracingString lvalue(trace, "a");
    vec.push_back(std::move(lvalue));
  }
  EXPECT_EQ(TracingString::NUM_INSTANCES, 0);
  EXPECT_EQ(trace,
      "1.ctorP: a\n"
      "2.ctorMove: a\n"
      "1.dtor: invalid\n"
      "2.dtor: a\n");
}

TEST_F(MODULE, vectorEmplaceBack) {
  // `std::vector::emplace_back` constructs the element in place from the constructor arguments, without a
  // temporary, a copy, or a move
  string trace;
  {
    vector<TracingString> vec;
    vec.emplace_back(trace, "a");
  }
  EXPECT_EQ(TracingString::NUM_INSTANCES, 0);
  EXPECT_EQ(trace,
      "1.ctorP: a\n"
      "1.dtor: a\n");
}

TEST_F(MODULE, forwardWithBar) {
  // In a function template, `T&&` is a forwarding reference, and `std::forward<T>` preserves the argument's
  // value category: lvalues reach `foo(const i32&)`, rvalues reach `foo(i32&&)`
  const i32 lvalue = 0;
  bar(lvalue);
  EXPECT_EQ(fooLvalue, 1);

  bar(3);
  EXPECT_EQ(fooRvalue, 1);
}

TEST_F(MODULE, forwardWithBaz) {
  // Like `forwardWithBar`, but with a C++20 abbreviated function template (`auto&&`), which forwards with
  // `std::forward<decltype(val)>`
  const i32 lvalue = 0;
  baz(lvalue);
  EXPECT_EQ(fooLvalue, 1);

  baz(3);
  EXPECT_EQ(fooRvalue, 1);
}

TEST_F(MODULE, implicitVirtualDtor) {
  // A destructor is virtual if the base class destructor is, even if the derived class does not declare one.
  // Deleting through a base pointer runs the implicit destructor of `B`, which destroys its member `m`
  A* p = new B; // NOLINT
  EXPECT_FALSE(Member::dtorCalled);
  delete p; // NOLINT
  // Class `B` needs not declare an overriding destructor---it is there implicitly
  EXPECT_TRUE(Member::dtorCalled);
}

TEST(MODULE_DEATH_TEST, exceptionThroughNoexceptCallsTerminate) { // NOLINT(*-underscore-*)
  // An exception escaping a `noexcept` function calls `std::terminate`. A `catch` further up the call stack
  // is never reached
  EXPECT_EXIT(
    {
      std::set_terminate(markingTerminateHandler);
      try {
        throwThroughNoexcept();
      } catch (...) {
        // Never reached: the exception must not leave the `noexcept` function
        std::fputs("exception caught\n", stderr);
        std::_Exit(0);
      }
    },
    ABORTED(), "`std::terminate` called");
}

// EOF
