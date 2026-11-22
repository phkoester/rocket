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
  using type = f64;
  const type zero = 0;

  type n = 4.2;
  EXPECT_EQ(n / zero, numeric_limits<type>::infinity());

  n = -4.2;
  EXPECT_EQ(n / zero, -numeric_limits<type>::infinity());
}

TEST_F(MODULE, optionalEmplace) {
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
  const auto sv = "ä"sv;
  EXPECT_EQ(sv, "\xc3\xa4");
  ASSERT_EQ(sv.size(), 2);
  EXPECT_EQ(sv[0], '\xc3');
  EXPECT_EQ(sv[1], '\xa4');
}

TEST_F(MODULE, vectorPushBackLvalue) {
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
  const i32 lvalue = 0;
  bar(lvalue);
  EXPECT_EQ(fooLvalue, 1);

  bar(3);
  EXPECT_EQ(fooRvalue, 1);
}

TEST_F(MODULE, forwardWithBaz) {
  const i32 lvalue = 0;
  baz(lvalue);
  EXPECT_EQ(fooLvalue, 1);

  baz(3);
  EXPECT_EQ(fooRvalue, 1);
}

TEST_F(MODULE, implicitVirtualDtor) {
  A* p = new B; // NOLINT
  EXPECT_FALSE(Member::dtorCalled);
  delete p; // NOLINT
  // Class `B` needs not declare an overriding destructor---it is there implicitly
  EXPECT_TRUE(Member::dtorCalled);
}

TEST(MODULE_DEATH_TEST, exceptionThroughNoexceptCallsTerminate) { // NOLINT(*-underscore-*)
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
