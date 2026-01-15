/*
 * test-C++.cc
 *
 * Tests related to the C++ language itself.
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket-gtest/TracingString.h"

// `Cxx` ----------------------------------------------------------------------------------------------------

struct Cxx : public Test {
  Cxx() {
    reset();
  }

  ~Cxx() override {}

protected:

  struct Member {
    static bool dtorCalled;

    ~Member() {  dtorCalled = true; }
  };

  struct A {
    virtual ~A() {}
  };

  struct B : A {
    Member m;
  };

  size_t fooLvalue;
  size_t fooRvalue;

  void
  SetUp() override {
    reset();
  }

  void
  foo(const int&) {
    ++fooLvalue;
  }

  void
  foo(int&&) {
    ++fooRvalue;
  }

  template<typename T>
  void
  bar(T&& v) {
    foo(std::forward<T>(v));
  }

  void
  baz(auto&& v) {
    foo(std::forward<decltype(v)>(v));
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
    TracingString lvalue(trace, "a");
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

TEST_F(Cxx, divideByZero_double) {
  using type = double;
  type zero = 0;

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
  int _blöße; nop(_blöße);

  // French
  int _ça; nop(_ça);

  // Japanese
  int _こんにちわ; nop(_こんにちわ);

  // Chinese (traditional)
  int _你好; nop(_你好);
}

TEST_F(Cxx, utf8StringLiteral) {
  EXPECT_EQ("ä", "\xc3\xa4");
  EXPECT_EQ("ä"sv.size(), 2);
}

TEST_F(Cxx, vectorPushBackLvalue) {
  string trace;
  {
    vector<TracingString> v;
    TracingString lvalue(trace, "a");
    v.push_back(lvalue);
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
    vector<TracingString> v;
    TracingString lvalue(trace, "a");
    v.push_back(std::move(lvalue));
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
    vector<TracingString> v;
    v.emplace_back(trace, "a");
  }
  EXPECT_EQ(TracingString::NUM_INSTANCES, 0);
  EXPECT_EQ(trace,
      "1.ctorP: a\n"
      "1.dtor: a\n");
}

TEST_F(Cxx, forwardWithBar) {
  int lvalue;
  bar(lvalue);
  EXPECT_EQ(fooLvalue, 1);

  bar(3);
  EXPECT_EQ(fooRvalue, 1);
}

TEST_F(Cxx, forwardWithBaz) {
  int lvalue;
  baz(lvalue);
  EXPECT_EQ(fooLvalue, 1);

  baz(3);
  EXPECT_EQ(fooRvalue, 1);
}

TEST_F(Cxx, implicitVirtualDtor) {
  A* p = new B;
  EXPECT_FALSE(Member::dtorCalled);
  delete p;
  // Class `B` needs not declare an overriding destructor---it is there implicitly
  EXPECT_TRUE(Member::dtorCalled);
}

// EOF
