/**
 * @file TracingString.h
 *
 * A tracing string.
 */

#pragma once

#include "rocket/rocket.h"

#include <string>

namespace rocket::gtest {

/**
 * A string that exactly traces which member functions were called.
 *
 * Useful for testing/debugging copy and move semantics.
 *
 * @NotThreadSafe
 */
struct TracingString {
  /// The number of currently existing instances.
  static u64 NUM_INSTANCES;

  /**
   * Resets the ID counter. This may be called on test setup.
   */
  static void
  reset() {
    ID_COUNTER = 0;
  }

  /// Counts how often the default constructor was called for this instance.
  u64 ctor = 0;
  /// Counts how often the copy constructor was called for this instance.
  u64 ctorCopy = 0;
  /// Counts how often the move constructor was called for this instance.
  u64 ctorMove = 0;
  /// Counts how often the constructor taking a pointer was called for this instance.
  u64 ctorP = 0;
  /// Counts how often the destructor was called for this instance.
  u64 dtor = 0;

  /// Counts how often the copy-assignment operator was called for this instance.
  u64 opAsgmtCopy = 0;
  /// Counts how often the move-assignment operator was called for this instance.
  u64 opAsgmtMove = 0;

  /**
   * @ctor
   *
   * @param trace the string to append tracing messages to
   */
  explicit TracingString(std::string& trace) noexcept :
      trace_(&trace),
      id_(++ID_COUNTER) {
    ++NUM_INSTANCES;
    this->trace("ctor");
    ++ctor;
  }

  /// @ctor_copy
  TracingString(const TracingString& rhs) noexcept :
      trace_(rhs.trace_),
      id_(++ID_COUNTER),
      v_(rhs.v_) {
    ++NUM_INSTANCES;
    trace("ctorCopy");
    ++ctorCopy;
  }

  /// @ctor_move
  TracingString(TracingString&& rhs) noexcept :
      trace_(rhs.trace_),
      id_(++ID_COUNTER),
      v_(rhs.v_) {
    ++NUM_INSTANCES;
    rhs.invalidate();
    trace("ctorMove");
    ++ctorMove;
  }

  /**
   * @ctor
   *
   * @param trace the string to append tracing messages to
   * @param p a pointer to a C string
   */
  explicit TracingString(std::string& trace, const char* p) noexcept :
      trace_(&trace),
      id_(++ID_COUNTER),
      v_(p) {
    ++NUM_INSTANCES;
    this->trace("ctorP");
    ++ctorP;
  }

  /// @dtor
  ~TracingString() noexcept {
    --NUM_INSTANCES;
    trace("dtor");
    ++dtor;
  }

  /// @member_op_asgmt_copy

  TracingString&
  operator=(const TracingString& rhs) noexcept { // cppcheck-suppress operatorEqVarError
    v_ = rhs.v_;
    trace("opAsgmtCopy");
    ++opAsgmtCopy;
    return *this;
  }

  /// @member_op_asgmt_move
  TracingString&
  operator=(TracingString&& rhs) noexcept { // cppcheck-suppress operatorEqVarError
    v_ = rhs.v_;
    rhs.invalidate();
    trace("opAsgmtMove");
    ++opAsgmtMove;
    return *this;
  }

  /// @member_op_cast{`std::string`}
  operator std::string() const noexcept {
    return v_;
  }

  /// @member_op_cast{`std::string_view`}
  operator std::string_view() const noexcept {
    return v_;
  }

private:

  static u64 ID_COUNTER;

  std::string* trace_;
  u64 id_;
  std::string v_;

  void
  invalidate() {
    v_ = "invalid";
  }

  void trace(std::string_view what) const;
};

} // namespace rocket::gtest

// EOF
