/**
 * @file Cow.h
 *
 * Copy-on-write values.
 */

#pragma once

#include <optional>

#include "rocket/assert.h"

namespace rocket {

// `Cow` ----------------------------------------------------------------------------------------------------

/**
 * A copy-on-write value.
 *
 * @tparam T the type of value
 * @tparam U the type of the owned value. If this is different from @p T, then @p T is assumed to be an
 *     efficiently copyable view type, such as `std::span` or `std::string_view`.
 */
template<typename T, typename U = T>
struct Cow {
  /**
   * @ctor
   *
   * @param ref the value to reference. If the types @p T and @p U are the same, The reference must remain
   *   valid for the lifetime of the `Cow`.
   */
  Cow(const T& ref) :
      modified_(false) {
    if constexpr (HAS_VIEW) {
      choice_.view = new T(ref);
    } else {
      choice_.ptr = &ref;
    }
  }

  /// @ ctor_copy
  Cow(const Cow& rhs) = delete;

  /// @ ctor_move
  Cow(Cow&& rhs) :
      modified_(rhs.modified_),
      choice_(rhs.choice_) {
    memset(&rhs, 0, sizeof(rhs));
  }

  /// @ member_op_assign_copy
  Cow& operator=(const Cow& rhs) = delete;

  /// @ member_op_assign_move
  Cow& operator=(Cow&& rhs) {
    modified_ = rhs.modified_;
    choice_ = rhs.choice_;
    memset(&rhs, 0, sizeof(rhs));
    return *this;
  }

  /// @dtor
  ~Cow() {
    if (modified_) {
      delete choice_.owned;
    } else if constexpr (HAS_VIEW) {
      delete choice_.view;
    }
  }

  /**
   * Assigns an owned value to the `Cow`, rendering the instance as "modified".
   *
   * The value is copied into the `Cow` as an owned value.
   *
   * @param value the value to assign
   * @return_this
   */
  Cow&
  operator=(const U& value) {
    if (modified_) {
      *choice_.owned = value;
    } else {
      if constexpr (HAS_VIEW) {
        delete choice_.view;
      }
      modified_ = true;
      choice_.owned = new U(value);
    }
    return *this;
  }

  /**
   * Assigns an owned value to the `Cow`, rendering the instance as "modified".
   *
   * The value is moved into the `Cow` as an owned value.
   *
   * @param value the value to assign
   * @return_this
   */
  Cow&
  operator=(U&& value) {
    if (modified_) {
      *choice_.owned = std::forward<U>(value);
    } else {
      if constexpr (HAS_VIEW) {
        delete choice_.view;
      }
      modified_ = true;
      choice_.owned = new U(std::forward<U>(value));
    }
    return *this;
  }

  /**
   * Returns a const reference to either the referenced or the owned value.
   *
   * This overload only exists when the types @p T and @p U are the same.
   *
   * @return a const reference to the referenced or the owned object
   */
  template<typename V = T> requires std::is_same_v<V, T> && std::is_same_v<T, U>
  const V&
  get() const {
    static_assert(not HAS_VIEW);
    return not modified_ ? *choice_.ptr : *choice_.owned;
  }

  /**
   * Returns a view to either the referenced or the owned value.
   *
   * This overload only exists when the types @p T and @p U are different.
   *
   * @return a view to either the referenced or the owned object
   */
  template<typename V = T> requires std::is_same_v<V, T> && (not std::is_same_v<T, U>)
  V
  get() const {
    static_assert(HAS_VIEW);
    return not modified_ ? *choice_.view : V(*choice_.owned);
  }

  /**
   * Returns `true` if the `Cow` has been assigned an owned value.
   *
   * @return `true` if the `Cow` has been assigned an owned value
   */
  bool modified() const { return modified_; }

  /**
   * Returns a nonconst reference to the owned value.
   *
   * @note This requires that the `Cow` is "modified", i.e. it has been assigned an owned value.
   *
   * @return a nonconst reference to the owned value
   * @throw #rocket::InvalidState if the `Cow` has not been assigned an owned value
   */
  U& owned() { ROCKET_EXPECT(modified_); return *choice_.owned; }

private:

  static constexpr bool HAS_VIEW = not std::is_same_v<T, U>;

  bool modified_;
  union {
    const T* ptr; ///< A reference if #HAS_VIEW is `false`
    const T* view; ///< A copyable view if #HAS_VIEW is `true`
    U* owned; ///< An owned value if #modified_ is `true`
  } choice_;
};

} // namespace rocket

// EOF
