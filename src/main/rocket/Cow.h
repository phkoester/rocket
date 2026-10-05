/**
 * @file Cow.h
 *
 * Copy-on-write values.
 */

#pragma once

#include "rocket/assert.h"

#include <variant>

namespace rocket {

// `Cow` ----------------------------------------------------------------------------------------------------

/**
 * A copy-on-write value.
 *
 * @tparam T the type of the value
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
  explicit Cow(const T& ref) : choice_(Ref(ref)) {}

  /// @ctor_copy
  Cow(const Cow& rhs) = delete;

  /// @ctor_move
  Cow(Cow&& rhs) noexcept = default;

  /// @member_op_asgmt_copy
  Cow& operator=(const Cow& rhs) = delete;

  /// @member_op_asgmt_move
  Cow& operator=(Cow&& rhs) noexcept = default;

  /**
   * Assigns an owned value to the #Cow, rendering the instance as "modified".
   *
   * The value is copied into the #Cow as an owned value.
   *
   * @param rhs the value to assign
   * @return_this
   */
  Cow&
  operator=(const U& rhs) {
    choice_.template emplace<U>(rhs); // XXX
    return *this;
  }

  /**
   * Assigns an owned value to the #Cow, rendering the instance as "modified".
   *
   * The value is moved into the #Cow as an owned value.
   *
   * @param rhs the value to assign
   * @return_this
   */
  Cow&
  operator=(U&& rhs) noexcept {
    choice_.template emplace<U>(std::move(rhs)); // XXX
    return *this;
  }

#if 0
  /**
   * Returns a const reference to either the referenced or the owned value.
   *
   * This overload only exists if the types @p T and @p U are the same.
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
   * This overload only exists if the types @p T and @p U are different.
   *
   * @return a view to either the referenced or the owned value
   */
  template<typename V = T> requires std::is_same_v<V, T> && (not std::is_same_v<T, U>)
  [[nodiscard]] V
  get() const {
    static_assert(HasView);
    return not modified_ ? *viewPtr() : V(*ownedPtr());
  }
#endif

  /**
   * Provides access to the value.
   *
   * If @p T and @p U are the same type, then this returns a `const T&`, either referencing a referenced or
   * an owned value.
   *
   * Otherwise, this returns a `const T`, which is a copy of the view type @p T, providing a view to either a
   * referenced or an owned value.
   *
   * @return a reference or a view to the value, either referenced or owned
   */
  [[nodiscard]] decltype(auto)
  get() const {
    if constexpr (HasView) {
      return modified() ? T(std::get<U>(choice_)) : std::get<T>(choice_);
    } else {
      return modified() ? std::get<U>(choice_) : std::get<Ref>(choice_).get();
    }
  }

  /**
   * Checks if the #Cow has been assigned an owned value.
   *
   * @return whether the #Cow has been assigned an owned value
   */
  [[nodiscard]] bool modified() const { return choice_.index() == 1; }

  /**
   * Returns a nonconst reference to the owned value.
   *
   * @note This requires that the #Cow is "modified", i.e. it has been assigned an owned value.
   *
   * @return a nonconst reference to the owned value
   */
  [[nodiscard]] U&
  owned() {
    ROCKET_EXPECT(modified());
    return std::get<U>(choice_);
  }

  /**
   * Returns a const reference to the owned value.
   *
   * @note This requires that the #Cow is "modified", i.e. it has been assigned an owned value.
   *
   * @return a const reference to the owned value
   */
  [[nodiscard]] const U&
  owned() const {
    ROCKET_EXPECT(modified());
    return std::get<U>(choice_);
  }

private:

  static constexpr bool HasView = not std::is_same_v<T, U>;

  using Ref = std::conditional_t<HasView, T, std::reference_wrapper<const T>>;

  std::variant<Ref, U> choice_;
};

} // namespace rocket

// EOF
