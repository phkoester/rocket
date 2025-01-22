/**
 * @file Lazy.h
 *
 * Lazy values.
 */

#pragma once

#include <functional>
#include <optional>

namespace rocket {

// `Lazy` ---------------------------------------------------------------------------------------------------

/**
 * A lazy value that is evaluated on demand using a `std::function`.
 *
 * @tparam T the type of the lazy value
 *
 * @NotThreadSafe
 */
template<typename T>
struct Lazy {
  /**
   * @ctor
   *
   * @param f the evaluation function
   */
  inline explicit Lazy(std::function<T()>&& f) : f_(std::move(f)) {}

  /**
   * If needed, evaluates, then returns the value.
   *
   * @return a value of type @p T
   */
  inline const T&
  get() const {
    if (not v_)
      v_ = f_();
    return *v_;
  }

  /**
   * Removes the evaluated value, if any, so that the next call of #get results in a fresh evaluation.
   */
  inline void reset() { v_ = std::nullopt; }

private:

  const std::function<T()> f_;
  mutable std::optional<T> v_;
};

} // namespace rocket

// EOF
