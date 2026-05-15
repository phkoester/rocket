/**
 * @file Cow.h
 *
 * Copy-on-write values.
 */

#pragma once

#include "rocket/assert.h"

#include <array>

namespace rocket {

// #Cow -----------------------------------------------------------------------------------------------------

/**
 * A copy-on-write value.
 *
 * @tparam T the type of value
 * @tparam U the type of the owned value. If this is different from @p T, then @p T is assumed to be an
 *   efficiently copyable view type, such as #std::span or #std::string_view.
 */
template<typename T, typename U = T>
struct Cow {
  /**
   * @ctor
   *
   * @param ref the value to reference. If the types @p T and @p U are the same, The reference must remain
   *   valid for the lifetime of the #Cow.
   */
  explicit Cow(const T& ref) {
    if constexpr (HasView) {
      new(viewPtr()) T(ref);
    } else {
      choice_.ptr = &ref;
    }
  }

  /// @ctor_copy
  Cow(const Cow& rhs) = delete;

  /// @ctor_move
  Cow(Cow&& rhs) noexcept :
    modified_(rhs.modified_) {
    if (modified_) {
      new(ownedPtr()) U(std::move(*rhs.ownedPtr()));
    } else if constexpr (HasView) {
      new(viewPtr()) T(std::move(*rhs.viewPtr()));
    } else {
      choice_.ptr = rhs.choice_.ptr;
    }
    std::memset(static_cast<void*>(&rhs), 0, sizeof(rhs));
  }

  /// @member_op_asgmt_copy
  Cow& operator=(const Cow& rhs) = delete;

  /// @member_op_asgmt_move
  Cow& operator=(Cow&& rhs) noexcept {
    modified_ = rhs.modified_;
    if (modified_) {
      new(ownedPtr()) U(std::move(*rhs.ownedPtr()));
    } else if constexpr (HasView) {
      new(viewPtr()) T(std::move(*rhs.viewPtr()));
    } else {
      choice_.ptr = rhs.choice_.ptr;
    }
    std::memset(static_cast<void*>(&rhs), 0, sizeof(rhs));
    return *this;
  }

  /// @dtor
  ~Cow() {
    if (modified_) {
      destroyOwned();
    } else if constexpr (HasView) {
      destroyView();
    }
  }

  /**
   * Assigns an owned value to the #Cow, rendering the instance as "modified".
   *
   * The value is copied into the #Cow as an owned value.
   *
   * @param value the value to assign
   * @return_this
   */
  Cow&
  operator=(const U& value) {
    if (modified_) {
      destroyOwned();
      new(ownedPtr()) U(value);
    } else {
      if constexpr (HasView) {
        destroyView();
      }
      modified_ = true;
      new(ownedPtr()) U(value);
    }
    return *this;
  }

  /**
   * Assigns an owned value to the #Cow, rendering the instance as "modified".
   *
   * The value is moved into the #Cow as an owned value.
   *
   * @param value the value to assign
   * @return_this
   */
  Cow&
  operator=(U&& value) noexcept {
    if (modified_) {
      destroyOwned();
      new(ownedPtr()) U(std::move(value));
    } else {
      if constexpr (HasView) {
        destroyView();
      }
      modified_ = true;
      new(ownedPtr()) U(std::move(value));
    }
    return *this;
  }

  /**
   * Returns a const reference to either the referenced or the owned value.
   *
   * This overload only exists when the types @p T and @p U are the same.
   *
   * @return a const reference to either the referenced or the owned value
   */
  template<typename V = T> requires std::is_same_v<V, T> && std::is_same_v<T, U>
  [[nodiscard]] const V&
  get() const {
    static_assert(not HasView);
    return not modified_ ? *choice_.ptr : *ownedPtr();
  }

  /**
   * Returns a view to either the referenced or the owned value.
   *
   * This overload only exists when the types @p T and @p U are different.
   *
   * @return a view to either the referenced or the owned value
   */
  template<typename V = T> requires std::is_same_v<V, T> && (not std::is_same_v<T, U>)
  [[nodiscard]] V
  get() const {
    static_assert(HasView);
    return not modified_ ? *viewPtr() : V(*ownedPtr());
  }

  /**
   * Checks if the #Cow has been assigned an owned value.
   *
   * @return whether the #Cow has been assigned an owned value
   */
  [[nodiscard]] bool modified() const { return modified_; }

  /**
   * Returns a nonconst reference to the owned value.
   *
   * @note This requires that the #Cow is "modified", i.e. it has been assigned an owned value.
   *
   * @return a nonconst reference to the owned value
   */
  [[nodiscard]] U& owned() { ROCKET_EXPECT(modified_); return *ownedPtr(); }

private:

  static constexpr bool HasView = not std::is_same_v<T, U>;

  void destroyOwned() noexcept { ownedPtr()->~U(); }

  void destroyView() noexcept { viewPtr()->~T(); }

  [[nodiscard]] constexpr U* ownedPtr() { return reinterpret_cast<U*>(choice_.owned.data()); }

  [[nodiscard]] constexpr const U*
  ownedPtr() const {
    return reinterpret_cast<const U*>(choice_.owned.data());
  }

  [[nodiscard]] constexpr T* viewPtr() { return reinterpret_cast<T*>(choice_.view.data()); }

  [[nodiscard]] constexpr const T*
  viewPtr() const {
    return reinterpret_cast<const T*>(choice_.view.data());
  }

  bool modified_ = false; ///< Whether there is an owned value
  union {
    const T* ptr; ///< A reference if not #HasView.
    std::array<char, sizeof(T)> view; ///< A copyable view if #HasView.
    std::array<char, sizeof(U)> owned; ///< An owned value if #modified_.
  } choice_;
};

} // namespace rocket

// EOF
