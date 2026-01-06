/**
 * @file Lazy.h
 *
 * Lazy values.
 */

#pragma once

#include "macro.h"

#include <functional>
#include <mutex>
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
   * @param fn the evaluation function
   */
  inline explicit Lazy(const std::function<T()>& fn) : fn_(fn) {}

  /**
   * If needed, evaluates, then returns the value.
   *
   * @return a value of type @p T
   */
  inline const T&
  get() const {
    if (not v_)
      v_ = fn_();
    return *v_;
  }

  /**
   * Removes the evaluated value, if any, so that the next call of #get results in a fresh evaluation.
   */
  inline void reset() { v_ = std::nullopt; }

private:

  const std::function<T()> fn_;
  mutable std::optional<T> v_;
};

// `ThreadSafeLazy` -----------------------------------------------------------------------------------------

/**
 * A lazy value that is evaluated on demand using a `std::function`.
 *
 * @tparam T the type of the lazy value
 *
 * @ThreadSafe
 */
template<typename T>
struct ThreadSafeLazy {
  /**
   * @ctor
   *
   * @param fn the evaluation function
   */
  inline explicit ThreadSafeLazy(const std::function<T()>& fn) : fn_(fn) {}

  /**
   * If needed, evaluates, then returns the value.
   *
   * @return a value of type @p T
   */
  const T&
  get() const {
    ROCKET_MUTEX_LOCK(mutex_);
    if (not v_)
      v_ = fn_();
    return *v_;
  }

  /**
   * Removes the evaluated value, if any, so that the next call of #get results in a fresh evaluation.
   */
  void
  reset() {
    ROCKET_MUTEX_LOCK(mutex_);
    v_ = std::nullopt;
  }

private:

  const std::function<T()> fn_;
  mutable std::optional<T> v_;
  mutable std::mutex mutex_; // Guards `v_`
};

} // namespace rocket

// EOF
