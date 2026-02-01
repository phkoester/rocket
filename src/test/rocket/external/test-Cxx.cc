/*
 * test-Cxx.cc
 *
 * Tests related to the C++ language itself.
 */

#include "rocket-test/rocket-test.h"

#include "rocket-test/TracingString.h"

// #Cxx -----------------------------------------------------------------------------------------------------

struct Cxx : public Test {
  Cxx() { // NOLINT
    reset();
  }

  ~Cxx() override = default;

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

bool Cxx::Member::dtorCalled = false;

TEST_F(Cxx, assignLvalueToOptional) {
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

TEST_F(Cxx, assignLvalueToOptionalWithMove) {
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

TEST_F(Cxx, divideByZeroF64) {
  using type = f64;
  const type zero = 0;

  type n = 4.2;
  EXPECT_EQ(n / zero, numeric_limits<type>::infinity());

  n = -4.2;
  EXPECT_EQ(n / zero, -numeric_limits<type>::infinity());
}

TEST_F(Cxx, optionalEmplace) {
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

TEST_F(Cxx, utf8Identifier) {
  // German
  [[maybe_unused]] i32 _blöße; // NOLINT

  // French
  [[maybe_unused]] i32 _ça; // NOLINT

  // Japanese
 [[maybe_unused]] i32 _こんにちわ; // NOLINT

  // Chinese (traditional)
  [[maybe_unused]] i32 _你好; // NOLINT
}

TEST_F(Cxx, utf8StringLiteral) {
  const auto sv = "ä"sv;
  EXPECT_EQ(sv, "\xc3\xa4");
  ASSERT_EQ(sv.size(), 2);
  EXPECT_EQ(sv[0], '\xc3');
  EXPECT_EQ(sv[1], '\xa4');
}

TEST_F(Cxx, vectorPushBackLvalue) {
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

TEST_F(Cxx, vectorPushBackLvalueWithMove) {
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

TEST_F(Cxx, vectorEmplaceBack) {
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

TEST_F(Cxx, forwardWithBar) {
  const i32 lvalue = 0;
  bar(lvalue);
  EXPECT_EQ(fooLvalue, 1);

  bar(3);
  EXPECT_EQ(fooRvalue, 1);
}

TEST_F(Cxx, forwardWithBaz) {
  const i32 lvalue = 0;
  baz(lvalue);
  EXPECT_EQ(fooLvalue, 1);

  baz(3);
  EXPECT_EQ(fooRvalue, 1);
}

TEST_F(Cxx, implicitVirtualDtor) {
  A* p = new B;
  EXPECT_FALSE(Member::dtorCalled);
  delete p; // NOLINT
  // Class #B needs not declare an overriding destructor---it is there implicitly
  EXPECT_TRUE(Member::dtorCalled);
}

// EOF
