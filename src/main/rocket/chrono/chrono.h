/**
 * @file chrono.h
 *
 * Utilities related #std::chrono.
 *
 * ## Time Zones
 *
 * This header declares #rocket::chrono::time_zone and #rocket::chrono::zoned_time, together with their
 * companions #rocket::chrono::sys_info, #rocket::chrono::local_info, #rocket::chrono::choose,
 * #rocket::chrono::zoned_traits, #rocket::chrono::zoned_seconds, #rocket::chrono::nonexistent_local_time,
 * #rocket::chrono::ambiguous_local_time, #rocket::chrono::locate_zone, and #rocket::chrono::current_zone.
 *
 * If the standard library provides the C++20 time-zone support, these are #std::chrono::time_zone,
 * #std::chrono::zoned_time, and so on, brought into `rocket::chrono` by using-declarations. Otherwise, they
 * are portable implementations that follow the specification of these classes in the C++20 standard,
 * reading the time-zone data from compiled TZif files (RFC 8536) in the directory given by the environment
 * variable `TZDIR`, which defaults to `/usr/share/zoneinfo`. This is the case on macOS, whose libc++ is
 * built without time-zone support.
 *
 * Differences of the portable implementation:
 *
 * - There is no `rocket::chrono::tzdb`; time zones are loaded on demand.
 * - Links (alternative names like `UTC` for `Etc/UTC`) are resolved via the file `tzdata.zi` in the
 *   time-zone directory. Without that file, #rocket::chrono::time_zone::name returns the name the zone was
 *   located by.
 * - The member `save` of #rocket::chrono::sys_info is derived from the UTC offsets, since TZif files do not
 *   record it.
 * - Time-zone files with leap seconds (`right/...`) are not supported.
 * - The formatter of #rocket::chrono::zoned_time does not support dynamic width or precision.
 *
 * If the time-zone directory does not exist, `UTC` and its aliases are still available.
 */

#pragma once

#include <version>

#if defined(ROCKET_OS_MACOS) && defined(ROCKET_CXX_COMPILER_LLVM)
  #if __cpp_lib_chrono >= 201907L
    #error This C++ version supports `std::chrono` time zones
  #endif
#else
  #if __cpp_lib_chrono < 201907L
    #error This C++ version does not support `std::chrono` time zones
  #endif
  #define ROCKET_HAS_STD_CHRONO_TIME_ZONE
#endif

#include "rocket/macro.h"

#include <chrono>
#include <mutex>

#ifndef ROCKET_HAS_STD_CHRONO_TIME_ZONE
  #include <fmt/chrono.h>

  #include <compare>
  #include <concepts>
  #include <memory>
  #include <ostream>
  #include <sstream>
  #include <stdexcept>
  #include <string>
  #include <type_traits>
  #include <utility>
#endif

// Part I: utilities related to `std::chrono` ===============================================================

namespace rocket::chrono {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

std::recursive_mutex& clockMutex();

std::chrono::milliseconds& clockOffset();

#ifdef ROCKET_TEST

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
  ROCKET_MUTEX_LOCK(clockMutex());
  clockOffset() = offset;
}

#endif // ROCKET_TEST

} // namespace internal

// Functions -----------------------------------------------------------------------------------------------

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
template<typename Clock>
std::chrono::time_point<Clock>
now() {
  ROCKET_MUTEX_LOCK(internal::clockMutex());
  return Clock::now() + internal::clockOffset();
}

/**
 * Returns the abbreviation the standard library uses for a time zone.
 *
 * Microsoft’s STL takes the abbreviations from the system ICU (`icu.dll`), localized for the user’s default
 * locale, so on a German system, `CET` and `CEST` are `MEZ` and `MESZ`. We ask the same ICU for the
 * abbreviation. Other implementations use the abbreviations from the time-zone database, which are not
 * localized.
 *
 * @param name the name of the time zone, in UTF-8, e.g., `Europe/Berlin`
 * @param dst whether to return the abbreviation for daylight saving time
 * @param fallback the abbreviation from the time-zone database
 * @return the abbreviation
 */
std::string abbreviation(std::string_view name, bool dst, std::string_view fallback);

} // namespace rocket::chrono

// Part II: time zones ======================================================================================

namespace rocket::chrono {

#ifdef ROCKET_HAS_STD_CHRONO_TIME_ZONE

/// Information about a time zone at a certain point in time.
using sys_info = std::chrono::sys_info;

/// Information about a local time in a time zone.
using local_info = std::chrono::local_info;

/// How to resolve an ambiguous local time.
using choose = std::chrono::choose;

/// The exception thrown when a local time is ambiguous.
using ambiguous_local_time = std::chrono::ambiguous_local_time;

/// The exception thrown when a local time does not exist.
using nonexistent_local_time = std::chrono::nonexistent_local_time;

/// A time zone.
using time_zone = std::chrono::time_zone;

#ifdef ROCKET_DOXYGEN

/**
 * Returns the current time zone of the computer.
 *
 * @return the current time zone, valid until the program ends
 */
const time_zone* current_zone();

/**
 * Locates a time zone by name.
 *
 * @param name the name of the time zone, for example `Europe/Berlin`
 * @return the time zone, valid until the program ends
 * @throw std::runtime_error if the time zone cannot be located
 */
const time_zone* locate_zone(std::string_view name);

#else

using std::chrono::current_zone;
using std::chrono::locate_zone;

#endif // ROCKET_DOXYGEN

/// The traits of a time-zone pointer used by #rocket::chrono::zoned_time.

template<typename T>
using zoned_traits = std::chrono::zoned_traits<T>;

/// A time point together with a time zone.
template<typename Duration, typename TimeZonePtr = const time_zone*>
using zoned_time = std::chrono::zoned_time<Duration, TimeZonePtr>;

/// A zoned time with a precision of seconds.
using zoned_seconds = std::chrono::zoned_time<std::chrono::seconds>;

#else

// `sys_info` -----------------------------------------------------------------------------------------------

/**
 * Information about a time zone at a certain point in time, equivalent to C++20’s #std::chrono::sys_info.
 */
struct sys_info {
  /// The beginning of the period these values apply to.
  std::chrono::sys_seconds begin;
  /// The end (exclusive) of the period these values apply to.
  std::chrono::sys_seconds end;
  /// The UTC offset, local time minus UTC.
  std::chrono::seconds offset;
  /// The daylight-saving offset. Nonzero if and only if daylight saving is in effect.
  std::chrono::minutes save;
  /// The abbreviation of the time zone, for example `CEST`.
  std::string abbrev;
};

/**
 * Writes a #rocket::chrono::sys_info to an output stream.
 *
 * @param os the output stream
 * @param val the value to write
 * @return @p os
 */
template<typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const sys_info& val) {
  os << '[' << val.begin << ',' << val.end << ',' << std::chrono::hh_mm_ss(val.offset) << ',' <<
    val.save << ',';
  for (const char c : val.abbrev) {
    os << os.widen(c);
  }
  return os << ']';
}

// `local_info` --------------------------------------------------------------------------------------------

/**
 * Information about a local time in a time zone, equivalent to C++20’s #std::chrono::local_info.
 */
struct local_info {
  static constexpr int unique = 0; ///< The local time maps to exactly one UTC time.
  static constexpr int nonexistent = 1; ///< The local time is in a gap, for example when DST starts.
  static constexpr int ambiguous = 2; ///< The local time maps to two UTC times, for example when DST ends.

  int result; ///< One of #unique, #nonexistent, and #ambiguous.
  sys_info first; ///< The period the local time is in, or the period before the gap.
  sys_info second; ///< The period after the gap, or the second period for an ambiguous time.
};

/**
 * Writes a #rocket::chrono::local_info to an output stream.
 *
 * @param os the output stream
 * @param val the value to write
 * @return @p os
 */
template<typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const local_info& val) {
  if (val.result == local_info::nonexistent) {
    return os << "[nonexistent," << val.first << ',' << val.second << ']';
  }
  if (val.result == local_info::ambiguous) {
    return os << "[ambiguous," << val.first << ',' << val.second << ']';
  }
  return os << val.first;
}

namespace internal {

template<typename Duration>
std::string
ambiguousMessage(const std::chrono::local_time<Duration>& tp, const local_info& info) {
  using namespace std::chrono;
  std::ostringstream os;
  os << tp << " is ambiguous.  It could be\n" << tp << ' ' << info.first.abbrev << " == " <<
    tp - info.first.offset << " UTC or\n" << tp << ' ' << info.second.abbrev << " == " <<
    tp - info.second.offset << " UTC";
  return std::move(os).str();
}

template<typename Duration>
std::string
nonexistentMessage(const std::chrono::local_time<Duration>& tp, const local_info& info) {
  using namespace std::chrono;
  std::ostringstream os;
  os << tp << " is in a gap between\n" <<
    local_seconds(info.first.end.time_since_epoch()) + info.first.offset << ' ' << info.first.abbrev <<
    " and\n" << local_seconds(info.second.begin.time_since_epoch()) + info.second.offset << ' ' <<
    info.second.abbrev << " which are both equivalent to\n" << info.first.end << " UTC";
  return std::move(os).str();
}

} // namespace internal

// `choose` ------------------------------------------------------------------------------------------------

/// How to resolve an ambiguous local time, equivalent to C++20’s `std::chrono::choose`.
enum class choose {
  earliest, ///< choose the earlier time point.
  latest ///< choose the later time point.
};

// `ambiguous_local_time` -----------------------------------------------------------------------------------

/**
 * The exception thrown when a local time is ambiguous, equivalent to C++20's
 * #std::chrono::ambiguous_local_time.
 */
struct ambiguous_local_time : std::runtime_error {
  /**
   * @ctor
   *
   * @param tp the local time
   * @param info the information about @p tp; its `result` must be #rocket::chrono::local_info::ambiguous
   */
  template<typename Duration>
  ambiguous_local_time(const std::chrono::local_time<Duration>& tp, const local_info& info) :
    std::runtime_error(internal::ambiguousMessage(tp, info)) {}
};

// `nonexistent_local_time` --------------------------------------------------------------------------------

/**
 * The exception thrown when a local time does not exist, equivalent to C++20's
 * #std::chrono::nonexistent_local_time.
 */
struct nonexistent_local_time : std::runtime_error {
  /**
   * @ctor
   *
   * @param tp the local time
   * @param info the information about @p tp; its `result` must be #rocket::chrono::local_info::nonexistent
   */
  template<typename Duration>
  nonexistent_local_time(const std::chrono::local_time<Duration>& tp, const local_info& info) :
    std::runtime_error(internal::nonexistentMessage(tp, info)) {}
};

// `time_zone` ----------------------------------------------------------------------------------------------

namespace internal {

struct TimeZoneAccess; // Defined in `chrono.cc`

struct ZoneData; // Defined in `chrono.cc`

} // namespace internal

/**
 * A time zone, equivalent to C++20’s #std::chrono::time_zone.
 *
 * Instances cannot be constructed. They are obtained from #rocket::chrono::locate_zone and
 * #rocket::chrono::current_zone, which keep them alive until the program ends.
 *
 * @ThreadSafe
 */
struct time_zone {
  /// @ctor_move
  time_zone(time_zone&& rhs) = default;

  /// @dtor
  ~time_zone() = default;

  /// @member_op_asgmt_move
  time_zone& operator=(time_zone&& rhs) = default;

  /**
   * Returns the information about this time zone at a UTC time point.
   *
   * @param tp the UTC time point
   * @return the information
   */
  template<typename Duration>
  [[nodiscard]] sys_info
  get_info(const std::chrono::sys_time<Duration>& tp) const {
    return getSysInfo(std::chrono::floor<std::chrono::seconds>(tp));
  }

  /**
   * Returns the information about a local time in this time zone.
   *
   * @param tp the local time
   * @return the information
   */
  template<typename Duration>
  [[nodiscard]] local_info
  get_info(const std::chrono::local_time<Duration>& tp) const {
    return getLocalInfo(std::chrono::floor<std::chrono::seconds>(tp));
  }

  /**
   * Returns the name of this time zone.
   *
   * @return the name, for example `Europe/Berlin`
   */
  [[nodiscard]] std::string_view name() const noexcept { return name_; }

  /**
   * Converts a UTC time point to the local time in this time zone.
   *
   * @param tp the UTC time point
   * @return the local time
   */
  template<typename Duration>
  [[nodiscard]] std::chrono::local_time<std::common_type_t<Duration, std::chrono::seconds>>
  to_local(const std::chrono::sys_time<Duration>& tp) const {
    using Ret = std::chrono::local_time<std::common_type_t<Duration, std::chrono::seconds>>;
    return Ret(tp.time_since_epoch()) + get_info(tp).offset;
  }

  /**
   * Converts a local time in this time zone to UTC.
   *
   * @param tp the local time
   * @return the UTC time point
   * @throw nonexistent_local_time if @p tp does not exist
   * @throw ambiguous_local_time if @p tp is ambiguous
   */
  template<typename Duration>
  [[nodiscard]] std::chrono::sys_time<std::common_type_t<Duration, std::chrono::seconds>>
  to_sys(const std::chrono::local_time<Duration>& tp) const {
    using Ret = std::chrono::sys_time<std::common_type_t<Duration, std::chrono::seconds>>;
    const local_info info = get_info(tp);
    if (info.result == local_info::nonexistent) {
      throw nonexistent_local_time(tp, info);
    }
    if (info.result == local_info::ambiguous) {
      throw ambiguous_local_time(tp, info);
    }
    return Ret(tp.time_since_epoch()) - info.first.offset;
  }

  /**
   * Converts a local time in this time zone to UTC, resolving nonexistent and ambiguous local times.
   *
   * @param tp the local time
   * @param c how to resolve an ambiguous local time
   * @return the UTC time point. If @p tp does not exist, this is the UTC time point of the gap
   */
  template<typename Duration>
  [[nodiscard]] std::chrono::sys_time<std::common_type_t<Duration, std::chrono::seconds>>
  to_sys(const std::chrono::local_time<Duration>& tp, choose c) const {
    using Ret = std::chrono::sys_time<std::common_type_t<Duration, std::chrono::seconds>>;
    const local_info info = get_info(tp);
    if (info.result == local_info::nonexistent) {
      return Ret(info.first.end);
    }
    if (info.result == local_info::ambiguous && c == choose::latest) {
      return Ret(tp.time_since_epoch()) - info.second.offset;
    }
    return Ret(tp.time_since_epoch()) - info.first.offset;
  }

  /// Compares the names of two time zones.
  friend bool
  operator==(const time_zone& lhs, const time_zone& rhs) noexcept {
    return lhs.name_ == rhs.name_;
  }

  /// Compares the names of two time zones.
  friend std::strong_ordering
  operator<=>(const time_zone& lhs, const time_zone& rhs) noexcept {
    return lhs.name_ <=> rhs.name_;
  }

private:

  friend struct internal::TimeZoneAccess;

  std::string name_;
  std::shared_ptr<const internal::ZoneData> data_;

  time_zone(std::string name, std::shared_ptr<const internal::ZoneData> data) :
    name_(std::move(name)),
    data_(std::move(data)) {}

  [[nodiscard]] local_info getLocalInfo(std::chrono::local_seconds tp) const;

  [[nodiscard]] sys_info getSysInfo(std::chrono::sys_seconds tp) const;
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Returns the current time zone of the computer, equivalent to C++20’s #std::chrono::current_zone.
 *
 * The time zone is taken from the symbolic link `/etc/localtime`, or the file `/etc/timezone`, in this
 * order. If none of these yields a time zone, the result is `UTC`.
 *
 * @ThreadSafe
 *
 * @return the current time zone, valid until the program ends
 */
const time_zone* current_zone();

/**
 * Locates a time zone by name, equivalent to C++20’s #std::chrono::locate_zone.
 *
 * @ThreadSafe
 *
 * @param name the name of the time zone, for example `Europe/Berlin`
 * @return the time zone, valid until the program ends
 * @throw std::runtime_error if the time zone cannot be located
 */
const time_zone* locate_zone(std::string_view name);

// `zoned_traits` -------------------------------------------------------------------------------------------

/**
 * The traits of a time-zone pointer used by #rocket::chrono::zoned_time, equivalent to C++20's
 * #std::chrono::zoned_traits.
 *
 * Specializations provide the static member functions `default_zone()` and `locate_zone(std::string_view)`.
 *
 * @tparam TimeZonePtr the time-zone pointer
 */
template<typename TimeZonePtr>
struct zoned_traits {};

/// @spec{#rocket::chrono::zoned_traits, const #rocket::chrono::time_zone*}
template<>
struct zoned_traits<const time_zone*> {
  /**
   * Returns the default time zone.
   *
   * @return the time zone `UTC`
   */
  static const time_zone* default_zone() { return ::rocket::chrono::locate_zone("UTC"); }

  /**
   * Locates a time zone by name.
   *
   * @param name the name of the time zone
   * @return the time zone, valid until the program ends
   * @throw std::runtime_error if the time zone cannot be located
   */
  static const time_zone* locate_zone(std::string_view name) { return ::rocket::chrono::locate_zone(name); }
};

// `zoned_time` ---------------------------------------------------------------------------------------------

/**
 * A time point together with a time zone, equivalent to C++20’s #std::chrono::zoned_time.
 *
 * @tparam Duration the duration of the time point
 * @tparam TimeZonePtr the time-zone pointer
 *
 * @NotThreadSafe
 */
template<typename Duration, typename TimeZonePtr = const time_zone*>
struct zoned_time {
  /// The duration of the time point, at least as fine as seconds.
  using duration = std::common_type_t<Duration, std::chrono::seconds>;

private:

  using Traits = zoned_traits<TimeZonePtr>;

  template<typename D>
  using SysTime = std::chrono::sys_time<D>;

  template<typename D>
  using LocalTime = std::chrono::local_time<D>;

  static constexpr bool HasDefaultZone = requires { Traits::default_zone(); };

  static constexpr bool HasLocateZone = requires(std::string_view name) {
    { Traits::locate_zone(name) } -> std::convertible_to<TimeZonePtr>;
  };

  static constexpr bool HasToSys = requires(TimeZonePtr& z) {
    { z->to_sys(LocalTime<Duration>()) } -> std::convertible_to<SysTime<duration>>;
  };

  static constexpr bool HasToSysChoose = requires(TimeZonePtr& z) {
    { z->to_sys(LocalTime<Duration>(), choose::earliest) } -> std::convertible_to<SysTime<duration>>;
  };

public:

  /// @ctor_default
  zoned_time() requires HasDefaultZone : zone_(Traits::default_zone()) {}

  /// @ctor_copy
  zoned_time(const zoned_time& rhs) = default;

  /**
   * @ctor
   *
   * @param st the UTC time point; the time zone is the default zone
   */
  zoned_time(const SysTime<Duration>& st) requires HasDefaultZone : // NOLINT(*-explicit-*)
    zone_(Traits::default_zone()),
    tp_(st) {}

  /**
   * @ctor
   *
   * @param zone the time zone
   */
  explicit zoned_time(TimeZonePtr zone) : zone_(std::move(zone)) {}

  /**
   * @ctor
   *
   * @param name the name of the time zone
   */
  explicit zoned_time(std::string_view name) requires HasLocateZone :
    zone_(Traits::locate_zone(name)) {}

  /**
   * @ctor
   *
   * @param rhs the zoned time to convert
   */
  template<typename Duration2> requires std::is_convertible_v<SysTime<Duration2>, SysTime<Duration>>
  zoned_time(const zoned_time<Duration2, TimeZonePtr>& rhs) : // NOLINT(*-explicit-*)
    zone_(rhs.get_time_zone()),
    tp_(rhs.get_sys_time()) {}

  /**
   * @ctor
   *
   * @param zone the time zone
   * @param st the UTC time point
   */
  zoned_time(TimeZonePtr zone, const SysTime<Duration>& st) :
    zone_(std::move(zone)),
    tp_(st) {}

  /**
   * @ctor
   *
   * @param name the name of the time zone
   * @param st the UTC time point
   */
  zoned_time(std::string_view name, const SysTime<Duration>& st) requires HasLocateZone :
    zoned_time(Traits::locate_zone(name), st) {}

  /**
   * @ctor
   *
   * @param zone the time zone
   * @param tp the local time
   * @throw nonexistent_local_time if @p tp does not exist
   * @throw ambiguous_local_time if @p tp is ambiguous
   */
  zoned_time(TimeZonePtr zone, const LocalTime<Duration>& tp) requires HasToSys :
    zone_(std::move(zone)),
    tp_(zone_->to_sys(tp)) {}

  /**
   * @ctor
   *
   * @param name the name of the time zone
   * @param tp the local time
   * @throw nonexistent_local_time if @p tp does not exist
   * @throw ambiguous_local_time if @p tp is ambiguous
   */
  zoned_time(std::string_view name, const LocalTime<Duration>& tp) requires HasLocateZone && HasToSys :
    zoned_time(Traits::locate_zone(name), tp) {}

  /**
   * @ctor
   *
   * @param zone the time zone
   * @param tp the local time
   * @param c how to resolve an ambiguous local time
   */
  zoned_time(TimeZonePtr zone, const LocalTime<Duration>& tp, choose c) requires HasToSysChoose :
    zone_(std::move(zone)),
    tp_(zone_->to_sys(tp, c)) {}

  /**
   * @ctor
   *
   * @param name the name of the time zone
   * @param tp the local time
   * @param c how to resolve an ambiguous local time
   */
  zoned_time(std::string_view name, const LocalTime<Duration>& tp, choose c) requires
    HasLocateZone && HasToSysChoose :
    zoned_time(Traits::locate_zone(name), tp, c) {}

  /**
   * @ctor
   *
   * @param zone the time zone
   * @param rhs the zoned time whose UTC time point to use
   */
  template<typename Duration2, typename TimeZonePtr2> requires
    std::is_convertible_v<SysTime<Duration2>, SysTime<Duration>>
  zoned_time(TimeZonePtr zone, const zoned_time<Duration2, TimeZonePtr2>& rhs) :
    zone_(std::move(zone)),
    tp_(rhs.get_sys_time()) {}

  /**
   * @ctor
   *
   * The argument @p c has no effect; it exists for compatibility with #std::chrono::zoned_time.
   *
   * @param zone the time zone
   * @param rhs the zoned time whose UTC time point to use
   * @param c ignored
   */
  template<typename Duration2, typename TimeZonePtr2>
    requires std::is_convertible_v<SysTime<Duration2>, SysTime<Duration>>
  zoned_time(TimeZonePtr zone, const zoned_time<Duration2, TimeZonePtr2>& rhs, [[maybe_unused]] choose c) :
    zoned_time(std::move(zone), rhs) {}

  /**
   * @ctor
   *
   * @param name the name of the time zone
   * @param rhs the zoned time whose UTC time point to use
   */
  template<typename Duration2, typename TimeZonePtr2> requires
    HasLocateZone && std::is_convertible_v<SysTime<Duration2>, SysTime<Duration>>
  zoned_time(std::string_view name, const zoned_time<Duration2, TimeZonePtr2>& rhs) :
    zoned_time(Traits::locate_zone(name), rhs) {}

  /**
   * @ctor
   *
   * The argument @p c has no effect; it exists for compatibility with #std::chrono::zoned_time.
   *
   * @param name the name of the time zone
   * @param rhs the zoned time whose UTC time point to use
   * @param c ignored
   */
  template<typename Duration2, typename TimeZonePtr2>
    requires HasLocateZone && std::is_convertible_v<SysTime<Duration2>, SysTime<Duration>>
  zoned_time(
    std::string_view name,
    const zoned_time<Duration2, TimeZonePtr2>& rhs,
    [[maybe_unused]] choose c) :
    zoned_time(Traits::locate_zone(name), rhs) {}

  /// @dtor
  ~zoned_time() = default;

  /// @member_op_asgmt_copy
  zoned_time& operator=(const zoned_time& rhs) = default;

  /**
   * Sets the local time, keeping the time zone.
   *
   * @param lt the local time
   * @return this instance
   * @throw nonexistent_local_time if @p lt does not exist
   * @throw ambiguous_local_time if @p lt is ambiguous
   */
  zoned_time&
  operator=(const LocalTime<Duration>& lt) {
    tp_ = zone_->to_sys(lt);
    return *this;
  }

  /**
   * Sets the UTC time point, keeping the time zone.
   *
   * @param st the UTC time point
   * @return this instance
   */
  zoned_time&
  operator=(const SysTime<Duration>& st) {
    tp_ = st;
    return *this;
  }

  /// Returns the local time.
  explicit operator LocalTime<duration>() const { return get_local_time(); }

  /// Returns the UTC time point.
  operator SysTime<duration>() const { return get_sys_time(); } // NOLINT(*-explicit-*)

  /**
   * Returns the information about the time zone at the time point.
   *
   * @return the information
   */
  [[nodiscard]] sys_info get_info() const { return zone_->get_info(tp_); }

  /**
   * Returns the local time.
   *
   * @return the local time
   */
  [[nodiscard]] LocalTime<duration> get_local_time() const { return zone_->to_local(tp_); }

  /**
   * Returns the UTC time point.
   *
   * @return the UTC time point
   */
  [[nodiscard]] SysTime<duration> get_sys_time() const { return tp_; }

  /**
   * Returns the time zone.
   *
   * @return the time zone
   */
  [[nodiscard]] TimeZonePtr get_time_zone() const { return zone_; }

private:

  TimeZonePtr zone_;
  SysTime<duration> tp_;
};

// Functions ------------------------------------------------------------------------------------------------

namespace internal {

template<typename T>
using TimeZoneRepresentation = std::conditional_t<
  std::is_convertible_v<T, std::string_view>,
  const time_zone*,
  std::remove_cvref_t<T>>;

} // namespace internal

/// @cond undocumented

zoned_time() -> zoned_time<std::chrono::seconds>;

template<typename Duration>
zoned_time(std::chrono::sys_time<Duration>) ->
  zoned_time<std::common_type_t<Duration, std::chrono::seconds>>;

template<typename TimeZonePtrOrName>
zoned_time(TimeZonePtrOrName&&) ->
  zoned_time<std::chrono::seconds, internal::TimeZoneRepresentation<TimeZonePtrOrName>>;

template<typename TimeZonePtrOrName, typename Duration>
zoned_time(TimeZonePtrOrName&&, std::chrono::sys_time<Duration>)
  -> zoned_time<
    std::common_type_t<Duration, std::chrono::seconds>,
    internal::TimeZoneRepresentation<TimeZonePtrOrName>>;

template<typename TimeZonePtrOrName, typename Duration>
zoned_time(TimeZonePtrOrName&&, std::chrono::local_time<Duration>, choose = choose::earliest) ->
  zoned_time<
    std::common_type_t<Duration, std::chrono::seconds>,
    internal::TimeZoneRepresentation<TimeZonePtrOrName>>;

template<typename Duration, typename TimeZonePtrOrName, typename TimeZonePtr2>
zoned_time(TimeZonePtrOrName&&, zoned_time<Duration, TimeZonePtr2>, choose = choose::earliest) ->
  zoned_time<Duration, internal::TimeZoneRepresentation<TimeZonePtrOrName>>;

/// @endcond

/**
 * Compares two zoned times for equality.
 *
 * @param lhs the first zoned time
 * @param rhs the second zoned time
 * @return whether both the time zones and the UTC time points are equal
 */
template<typename Duration1, typename Duration2, typename TimeZonePtr>
bool
operator==(const zoned_time<Duration1, TimeZonePtr>& lhs, const zoned_time<Duration2, TimeZonePtr>& rhs) {
  return lhs.get_time_zone() == rhs.get_time_zone() && lhs.get_sys_time() == rhs.get_sys_time();
}

/**
 * Writes a zoned time to an output stream, using the format `{:L%F %T %Z}`.
 *
 * @param os the output stream
 * @param val the value to write
 * @return @p os
 */
template<typename CharT, typename Traits, typename Duration, typename TimeZonePtr>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const zoned_time<Duration, TimeZonePtr>& val) {
  if constexpr (std::same_as<CharT, char>) {
    return os << fmt::format(os.getloc(), "{:L%F %T %Z}", val);
  } else {
    return os << fmt::format(os.getloc(), L"{:L%F %T %Z}", val);
  }
}

/// A zoned time with a precision of seconds, equivalent to C++20’s `std::chrono::zoned_seconds`.
using zoned_seconds = zoned_time<std::chrono::seconds>;

#endif // ROCKET_HAS_STD_CHRONO_TIME_ZONE

} // namespace rocket::chrono

// EOF
