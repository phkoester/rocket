/// @file chrono.h
///
/// Utilities related to #std::chrono.

#pragma once

#include "rocket/chrono/time_zone.h"
#include "rocket/macro.h" // `ROCKET_MUTEX_LOCK`

#include <chrono>
#include <mutex>

namespace rocket::chrono {

// `SystemClock`, `SystemTimePoint` -------------------------------------------------------------------------

/// The system clock.
using SystemClock = std::chrono::system_clock;
/// A system-clock time point.
using SystemTimePoint = std::chrono::time_point<SystemClock>;

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

std::recursive_mutex& clockMutex();

std::chrono::milliseconds& clockOffset();

} // namespace internal

#ifdef ROCKET_TEST

namespace testing {

// Testing --------------------------------------------------------------------------------------------------

/// Sets the clock offset so that the current time becomes a given time point.
///
/// The offset is rounded to the precision of the clock offset.
///
/// Useful for testing.
///
/// @ThreadSafe
///
/// @tparam Clock the clock to use
/// @tparam Duration the duration type of the time point
/// @param tp the time point that should be the current time
template<typename Clock = SystemClock, typename Duration = std::chrono::milliseconds>
void
setClock(const std::chrono::time_point<Clock, Duration>& tp) {
  ROCKET_MUTEX_LOCK(internal::clockMutex());
  internal::clockOffset() = round<std::chrono::milliseconds>(tp - Clock::now());
}

/// Sets the clock offset.
///
/// Useful for testing.
///
/// @ThreadSafe
///
/// @param offset the clock offset
template<typename Duration>
void
setClockOffset(Duration offset) {
  ROCKET_MUTEX_LOCK(internal::clockMutex());
  internal::clockOffset() = offset;
}

} // namespace testing

#endif // ROCKET_TEST

// Functions ------------------------------------------------------------------------------------------------

/// Returns the abbreviation the standard library uses for a time zone.
///
/// Microsoft’s STL takes the abbreviations from the system ICU (`icu.dll`), localized for the user’s default
/// locale, so on a German system, `CET` and `CEST` are `MEZ` and `MESZ`. We ask the same ICU for the
/// abbreviation. Other implementations use the abbreviations from the time-zone database, which are not
/// localized.
///
/// @param name the name of the time zone, in UTF-8, e.g., `Europe/Berlin`
/// @param dst whether to return the abbreviation for daylight saving time
/// @param fallback the abbreviation from the time-zone database
/// @return the abbreviation
std::string abbreviation(std::string_view name, bool dst, std::string_view fallback);

/// Checks if @p name is an unambiguous time-zone name, which input must use.
///
/// Such a name contains a `/`, as in `Europe/Berlin` or `Etc/GMT+5`, or is `UTC` or `GMT`. This rejects the
/// legacy names of the time-zone database that look like abbreviations but stand for a single zone, such as
/// `CET` (`Europe/Brussels`), `EST` (`America/Panama`), or `EST5EDT`, as well as names such as `Japan`. Real
/// abbreviations, such as `CEST` or `CST`, are not names at all; they are ambiguous.
///
/// @param name the name
/// @return whether @p name is an unambiguous time-zone name
constexpr bool
isUnambiguousZoneName(std::string_view name) {
  return name.find('/') != std::string_view::npos || name == "UTC" || name == "GMT";
}

/// Locates a time zone by an unambiguous name, see #rocket::chrono::isUnambiguousZoneName.
///
/// @param name the name of the time zone, for example `Europe/Berlin`
/// @return the time zone, valid until the program ends
/// @throw #std::runtime_error if the time zone cannot be located or @p name is not unambiguous
inline const time_zone*
locateUnambiguousZone(std::string_view name) {
  // Locate first, so that an unknown name is reported as such
  const auto* ret = locate_zone(name);
  if (not isUnambiguousZoneName(name)) {
    throw std::runtime_error("ambiguous time-zone name `" + std::string(name) + '`');
  }
  return ret;
}

/// Returns the current time of the clock, considering the clock offset.
///
/// Testable code should use this function.
///
/// @ThreadSafe
///
/// @tparam Clock the clock to use
/// @return the current time
template<typename Clock = SystemClock>
std::chrono::time_point<Clock>
now() {
  ROCKET_MUTEX_LOCK(internal::clockMutex());
  return Clock::now() + internal::clockOffset();
}

} // namespace rocket::chrono

// EOF
