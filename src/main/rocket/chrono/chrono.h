/**
 * @file chrono.h
 *
 * Utilities related #std::chrono.
 */

#pragma once

#include "rocket/chrono/time_zone.h"
#include "rocket/macro.h" // `ROCKET_MUTEX_LOCK`

#include <chrono>
#include <mutex>

namespace rocket::chrono {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

std::recursive_mutex& clockMutex();

std::chrono::milliseconds& clockOffset();

} // namespace internal

#ifdef ROCKET_TEST

namespace testing {

// Testing --------------------------------------------------------------------------------------------------

/**
 * Sets the clock offset so that the current time becomes a given time point.
 *
 * The offset is rounded to the precision of the clock offset.
 *
 * Useful for testing.
 *
 * @ThreadSafe
 *
 * @param tp the time point that should be the current time
 */
template<typename Duration>
void
setClock(const std::chrono::sys_time<Duration>& tp) {
  using namespace std::chrono;
  ROCKET_MUTEX_LOCK(internal::clockMutex());
  internal::clockOffset() = round<milliseconds>(tp - system_clock::now());
}

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
  ROCKET_MUTEX_LOCK(internal::clockMutex());
  internal::clockOffset() = offset;
}

} // namespace testing

#endif // ROCKET_TEST

// Functions ------------------------------------------------------------------------------------------------

/**
 * Returns the current time of the clock, considering the clock offset.
 *
 * Testable code should use this function.
 *
 * @ThreadSafe
 *
 * @tparam Clock the clock to use
 * @return the current time
 */
template<typename Clock = std::chrono::system_clock>
std::chrono::time_point<Clock>
now() {
  ROCKET_MUTEX_LOCK(internal::clockMutex());
  return Clock::now() + internal::clockOffset();
}

} // namespace rocket::chrono

// EOF
