/**
 * @file ChattyString.h
 *
 * A test class with detailed output and statistics.
 *
 * Useful for testing/debugging copy and move semantics.
 */

#pragma once

#include <iostream>

namespace rocket::gtest {

/**
 * A test class with detailed output and statistics.
 *
 * Useful for testing/debugging copy and move semantics.
 *
 * @NotThreadSafe
 */
struct ChattyString {
  /// The number of currently existing instances.
  static size_t NUM_INSTANCES;

  /**
   * Resets the ID counter. This may be called on test setup.
   */
  static void
  reset() {
    ID_COUNTER = 0;
  }

  /// Counts how often the default constructor was called for this instance.
  size_t defaultCtor = 0;
  /// Counts how often the copy constructor was called for this instance.
  size_t copyCtor = 0;
  /// Counts how often the move constructor was called for this instance.
  size_t moveCtor = 0;
  /// Counts how often the constructor taking a pointer was called for this instance.
  size_t ctorP = 0;
  /// Counts how often the destructor was called for this instance.
  size_t dtor = 0;

  /// Counts how often the copy-assignment operator was called for this instance.
  size_t copyAsgmtOp = 0;
  /// Counts how often the move-assignment operator was called for this instance.
  size_t moveAsgmtOp = 0;

  /**
   * @ctor
   *
   * @param os the output stream to chat to
   */
  explicit ChattyString(std::ostream& os = std::cout) noexcept :
      os_(&os),
      id_(++ID_COUNTER) {
    ++NUM_INSTANCES;
    info("Default ctor");
    ++defaultCtor;
  }

  /// @ctor_copy
  ChattyString(const ChattyString& rhs) noexcept :
      v_(rhs.v_),
      os_(rhs.os_),
      id_(++ID_COUNTER) {
    ++NUM_INSTANCES;
    info("Copy ctor");
    ++copyCtor;
  }

  /// @ctor_move
  ChattyString(ChattyString&& rhs) noexcept :
      v_(rhs.v_),
      os_(rhs.os_),
      id_(++ID_COUNTER) {
    ++NUM_INSTANCES;
    rhs.invalidate();
    info("Move ctor");
    ++moveCtor;
  }

  /**
   * @ctor
   *
   * @param p a pointer to a C string
   * @param os the output stream to chat to
   */
  explicit ChattyString(const char* p, std::ostream& os = std::cout) noexcept :
      v_(p),
      os_(&os),
      id_(++ID_COUNTER) {
    ++NUM_INSTANCES;
    info("Ctor p");
    ++ctorP;
  }

  /// @dtor
  ~ChattyString() noexcept {
    --NUM_INSTANCES;
    info("Dtor");
    ++dtor;
  }

  /// @member_op_asgmt_copy
  ChattyString&
  // cppcheck-suppress operatorEqVarError
  operator=(const ChattyString& rhs) noexcept {
    v_ = rhs.v_;

    info("Copy-asgmt op");
    ++copyAsgmtOp;
    return *this;
  }

  /// @member_op_asgmt_move
  ChattyString&
  // cppcheck-suppress operatorEqVarError
  operator=(ChattyString&& rhs) noexcept {
    v_ = rhs.v_;
    rhs.invalidate();

    info("Move-asgmt op");
    ++moveAsgmtOp;
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

  friend std::ostream& operator<<(std::ostream&, const ChattyString&);

  static size_t ID_COUNTER;

  std::string v_;
  std::ostream* os_;
  size_t id_;
  mutable size_t printed_ = 0;

  void
  // cppcheck-suppress passedByValue
  info(std::string_view msg) const {
    *os_ << id_ << '.' << v_ << ": " << msg << '\n';
  }

  void
  invalidate() {
    v_ = "invalid";
  }

  std::ostream&
  print(std::ostream& os) const {
    os << id_ << '.' << v_ << ": ";
    print0(os, "defaultCtor", defaultCtor);
    print0(os, "copyCtor", copyCtor);
    print0(os, "moveCtor", moveCtor);
    print0(os, "ctorP", ctorP);
    print0(os, "dtor", dtor);
    print0(os, "copyAsgmtOp", copyAsgmtOp);
    print0(os, "moveAsgmtOp", moveAsgmtOp);
    printed_ = 0;
    return os << '\n';
  }

  void
  // cppcheck-suppress passedByValue
  print0(std::ostream& os, std::string_view name, size_t value) const {
    if (value > 0) {
      if (printed_++ > 0)
        os << ", ";
      os << name << '=' << value;
    }
  }
};

/// @op_output{#rocket::gtest::ChattyString}
inline std::ostream&
operator<<(std::ostream& lhs, const ChattyString& rhs) {
  return rhs.print(lhs);
}

} // namespace rocket::gtest

// EOF
