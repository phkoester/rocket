/**
 * @file chrono.h
 *
 * Utilities around `std::chrono`.
 */

#pragma once

#include <chrono>

namespace rocket::chrono {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

#ifdef ROCKET_TESTING

/// @ThreadSafe
void setClockOffset(std::chrono::hours offset);

#endif

} // namespace internal

// `SystemClockTimePoint` -----------------------------------------------------------------------------------

using SystemClockTimePoint = std::chrono::time_point<std::chrono::system_clock>;

// Functions -----------------------------------------------------------------------------------------------

/**
 * Returns the current time, as returned by `std::chrono::system_clock`.
 *
 * Code to be unit-tested should use this function instead of `std::chrono::system_clock::now()`.
 *
 * @ThreadSafe
 */
SystemClockTimePoint systemClockNow();

} // namespace rocket::chrono

// EOF
