/**
 * @file chrono.h
 *
 * Utilities around `std::chrono`.
 */

#pragma once

#include "rocket/macro.h"

#include <chrono>

namespace rocket::chrono {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

extern std::recursive_mutex clockMutex;

extern std::chrono::milliseconds clockOffset;

#ifdef ROCKET_TESTING

/**
 * Sets the clock offset.
 *
 * Useful for testing.
 *
 * @ThreadSafe
 *
 * @param offset the clock offset
 */
template<typename Duration>
void
setClockOffset(Duration offset) {
  ROCKET_MUTEX_LOCK(clockMutex);
  clockOffset = offset;
}

#endif

} // namespace internal

// Functions -----------------------------------------------------------------------------------------------

/**
 * Returns the current time of the clock.
 *
 * Testable code should use this function.
 *
 * @ThreadSafe
 *
 * @tparam Clock the clock to use
 * @return the current time
 */
template<typename Clock>
std::chrono::time_point<Clock>
now() {
  ROCKET_MUTEX_LOCK(internal::clockMutex);
  return Clock::now() + internal::clockOffset;
}

} // namespace rocket::chrono

// EOF
