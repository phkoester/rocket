/**
 * @file chrono.h
 *
 * Utilities around #std::chrono.
 *
 * ## Time Zones
 *
 * This header declares #rocket::chrono::TimeZone and #rocket::chrono::ZonedTime, together with their
 * companions #rocket::chrono::SysInfo, #rocket::chrono::LocalInfo, #rocket::chrono::Choose,
 * #rocket::chrono::ZonedTraits, #rocket::chrono::NonexistentLocalTime, #rocket::chrono::AmbiguousLocalTime,
 * #rocket::chrono::locateZone, and #rocket::chrono::currentZone.
 *
 * If the standard library provides the C++20 time-zone supportm, these are #std::chrono::time_zone,
 * #std::chrono::zoned_time, and so on. Otherwise, they are portable implementations that follow the
 * specification of these classes in the C++20 standard, reading the time-zone data from
 * compiled TZif files (RFC 8536) in the directory given by the environment variable `TZDIR`, which defaults
 * to `/usr/share/zoneinfo`. This is the case on Darwin (macOS), whose libc++ is built without time-zone
 * support.
 *
 * Differences of the portable implementation:
 *
 * - There is no #std::chrono::tzdb; time zones are loaded on demand.
 * - Links (alternative names like `UTC` for `Etc/UTC`) are resolved via the file `tzdata.zi` in the
 *   time-zone directory. Without that file, #rocket::chrono::TimeZone::name returns the name the zone was
 *   located by.
 * - #rocket::chrono::SysInfo::save is derived from the UTC offsets, since TZif files do not record it.
 * - Time-zone files with leap seconds (`right/...`) are not supported.
 * - The formatter of #rocket::chrono::ZonedTime does not support dynamic width or precision.
 *
 * If the time-zone directory does not exist, `UTC` and its aliases are still available.
 */

#pragma once

#if defined(ROCKET_OS_MACOS) && defined(ROCKET_CXX_COMPILER_LLVM)
  #define ROCKET_HAS_CHRONO_TIME_ZONE_IMPL
#endif

#include "rocket/macro.h"

#include <chrono>
#include <mutex>

#ifdef ROCKET_HAS_CHRONO_TIME_ZONE_IMPL
  #include <compare>
  #include <concepts>
  #include <format>
  #include <memory>
  #include <ostream>
  #include <sstream>
  #include <stdexcept>
  #include <string>
  #include <type_traits>
  #include <utility>
#endif

namespace rocket::chrono {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

ROCKET_PUBLIC extern std::recursive_mutex clockMutex;

ROCKET_PUBLIC extern std::chrono::milliseconds clockOffset;

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
  ROCKET_MUTEX_LOCK(clockMutex);
  clockOffset = offset;
}

#endif // ROCKET_TEST

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

#ifndef ROCKET_HAS_CHRONO_TIME_ZONE_IMPL

// Time zones (standard library) ----------------------------------------------------------------------------

/// A time zone.
using TimeZone = std::chrono::time_zone;

/// Information about a time zone at a certain point in time.
using SysInfo = std::chrono::sys_info;

/// Information about a local time in a time zone.
using LocalInfo = std::chrono::local_info;

/// How to resolve an ambiguous local time.
using Choose = std::chrono::choose;

/// The exception thrown when a local time does not exist.
using NonexistentLocalTime = std::chrono::nonexistent_local_time;

/// The exception thrown when a local time is ambiguous.
using AmbiguousLocalTime = std::chrono::ambiguous_local_time;

/// The traits of a time-zone pointer used by #rocket::chrono::ZonedTime.
template<typename TimeZonePtr>
using ZonedTraits = std::chrono::zoned_traits<TimeZonePtr>;

/// A time point together with a time zone.
template<typename Duration, typename TimeZonePtr = const TimeZone*>
using ZonedTime = std::chrono::zoned_time<Duration, TimeZonePtr>;

/**
 * Locates a time zone by name.
 *
 * @ThreadSafe
 *
 * @param name the name of the time zone, for example `Europe/Berlin`
 * @return the time zone
 * @throw std::runtime_error if the time zone cannot be located
 */
inline const TimeZone* locateZone(std::string_view name) { return std::chrono::locate_zone(name); }

/**
 * Returns the current time zone of the computer.
 *
 * @ThreadSafe
 *
 * @return the current time zone
 */
inline const TimeZone* currentZone() { return std::chrono::current_zone(); }

#else

// Time zones (portable) ------------------------------------------------------------------------------------

/**
 * Information about a time zone at a certain point in time, equivalent to C++20's #std::chrono::sys_info.
 */
struct SysInfo {
  std::chrono::sys_seconds begin; ///< The begin of the period these values apply to.
  std::chrono::sys_seconds end; ///< The end (exclusive) of the period these values apply to.
  std::chrono::seconds offset; ///< The UTC offset, local time minus UTC.
  std::chrono::minutes save; ///< The daylight-saving offset. Non-zero if and only if daylight saving is in effect.
  std::string abbrev; ///< The abbreviation of the time zone, for example `CEST`.
};

/**
 * Writes a #rocket::chrono::SysInfo to an output stream.
 *
 * @param os the output stream
 * @param val the value to write
 * @return @p os
 */
template<typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const SysInfo& val) {
  os << '[' << val.begin << ',' << val.end << ',' << std::chrono::hh_mm_ss(val.offset) << ',' << val.save
     << ',';
  for (const char c : val.abbrev) {
    os << os.widen(c);
  }
  return os << ']';
}

/**
 * Information about a local time in a time zone, equivalent to C++20's #std::chrono::local_info.
 */
struct LocalInfo {
  static constexpr int unique = 0; ///< The local time maps to exactly one UTC time.
  static constexpr int nonexistent = 1; ///< The local time is in a gap, for example when DST starts.
  static constexpr int ambiguous = 2; ///< The local time maps to two UTC times, for example when DST ends.

  int result; ///< One of #unique, #nonexistent, and #ambiguous.
  SysInfo first; ///< The period the local time is in, or the period before the gap.
  SysInfo second; ///< The period after the gap, or the second period for an ambiguous time.
};

/**
 * Writes a #rocket::chrono::LocalInfo to an output stream.
 *
 * @param os the output stream
 * @param val the value to write
 * @return @p os
 */
template<typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const LocalInfo& val) {
  if (val.result == LocalInfo::nonexistent) {
    return os << "[nonexistent," << val.first << ',' << val.second << ']';
  }
  if (val.result == LocalInfo::ambiguous) {
    return os << "[ambiguous," << val.first << ',' << val.second << ']';
  }
  return os << val.first;
}

/// How to resolve an ambiguous local time, equivalent to C++20's #std::chrono::choose.
enum class Choose {
  earliest, ///< Choose the earlier time point.
  latest ///< Choose the later time point.
};

namespace internal {

template<typename Duration>
std::string
nonexistentMessage(const std::chrono::local_time<Duration>& tp, const LocalInfo& info) {
  using namespace std::chrono;
  std::ostringstream os;
  os << tp << " is in a gap between\n"
     << local_seconds(info.first.end.time_since_epoch()) + info.first.offset << ' ' << info.first.abbrev
     << " and\n"
     << local_seconds(info.second.begin.time_since_epoch()) + info.second.offset << ' ' << info.second.abbrev
     << " which are both equivalent to\n"
     << info.first.end << " UTC";
  return std::move(os).str();
}

template<typename Duration>
std::string
ambiguousMessage(const std::chrono::local_time<Duration>& tp, const LocalInfo& info) {
  using namespace std::chrono;
  std::ostringstream os;
  os << tp << " is ambiguous. It could be\n"
     << tp << ' ' << info.first.abbrev << " == " << tp - info.first.offset << " UTC or\n"
     << tp << ' ' << info.second.abbrev << " == " << tp - info.second.offset << " UTC";
  return std::move(os).str();
}

} // namespace internal

/**
 * The exception thrown when a local time does not exist, equivalent to C++20's
 * #std::chrono::nonexistent_local_time.
 */
struct NonexistentLocalTime : std::runtime_error {
  /**
   * @ctor
   *
   * @param tp the local time
   * @param info the information about @p tp; its `result` must be #rocket::chrono::LocalInfo::nonexistent
   */
  template<typename Duration>
  NonexistentLocalTime(const std::chrono::local_time<Duration>& tp, const LocalInfo& info) :
    std::runtime_error(internal::nonexistentMessage(tp, info)) {}
};

/**
 * The exception thrown when a local time is ambiguous, equivalent to C++20's
 * #std::chrono::ambiguous_local_time.
 */
struct AmbiguousLocalTime : std::runtime_error {
  /**
   * @ctor
   *
   * @param tp the local time
   * @param info the information about @p tp; its `result` must be #rocket::chrono::LocalInfo::ambiguous
   */
  template<typename Duration>
  AmbiguousLocalTime(const std::chrono::local_time<Duration>& tp, const LocalInfo& info) :
    std::runtime_error(internal::ambiguousMessage(tp, info)) {}
};

namespace internal {

struct ZoneData; // Defined in `chrono.cc`
struct TimeZoneAccess; // Defined in `chrono.cc`

} // namespace internal

/**
 * A time zone, equivalent to C++20's #std::chrono::time_zone.
 *
 * Instances cannot be constructed. They are obtained from #rocket::chrono::locateZone and
 * #rocket::chrono::currentZone, which keep them alive until the program ends.
 *
 * @ThreadSafe
 */
class TimeZone {
public:

  TimeZone(TimeZone&&) = default;

  ~TimeZone() = default;

  TimeZone& operator=(TimeZone&&) = default;

  /**
   * Returns the name of this time zone.
   *
   * @return the name, for example `Europe/Berlin`
   */
  [[nodiscard]] std::string_view name() const noexcept { return name_; }

  /**
   * Returns the information about this time zone at a UTC time point.
   *
   * @param tp the UTC time point
   * @return the information
   */
  template<typename Duration>
  [[nodiscard]] SysInfo
  get_info(const std::chrono::sys_time<Duration>& tp) const {
    return getInfo(std::chrono::floor<std::chrono::seconds>(tp));
  }

  /**
   * Returns the information about a local time in this time zone.
   *
   * @param tp the local time
   * @return the information
   */
  template<typename Duration>
  [[nodiscard]] LocalInfo
  get_info(const std::chrono::local_time<Duration>& tp) const {
    return getLocalInfo(std::chrono::floor<std::chrono::seconds>(tp));
  }

  /**
   * Converts a local time in this time zone to UTC.
   *
   * @param tp the local time
   * @return the UTC time point
   * @throw NonexistentLocalTime if @p tp does not exist
   * @throw AmbiguousLocalTime if @p tp is ambiguous
   */
  template<typename Duration>
  [[nodiscard]] std::chrono::sys_time<std::common_type_t<Duration, std::chrono::seconds>>
  to_sys(const std::chrono::local_time<Duration>& tp) const {
    using Ret = std::chrono::sys_time<std::common_type_t<Duration, std::chrono::seconds>>;
    const LocalInfo info = get_info(tp);
    if (info.result == LocalInfo::nonexistent) {
      throw NonexistentLocalTime(tp, info);
    }
    if (info.result == LocalInfo::ambiguous) {
      throw AmbiguousLocalTime(tp, info);
    }
    return Ret(tp.time_since_epoch()) - info.first.offset;
  }

  /**
   * Converts a local time in this time zone to UTC, resolving nonexistent and ambiguous local times.
   *
   * @param tp the local time
   * @param z how to resolve an ambiguous local time
   * @return the UTC time point. If @p tp does not exist, this is the UTC time point of the gap
   */
  template<typename Duration>
  [[nodiscard]] std::chrono::sys_time<std::common_type_t<Duration, std::chrono::seconds>>
  to_sys(const std::chrono::local_time<Duration>& tp, Choose z) const {
    using Ret = std::chrono::sys_time<std::common_type_t<Duration, std::chrono::seconds>>;
    const LocalInfo info = get_info(tp);
    if (info.result == LocalInfo::nonexistent) {
      return Ret(info.first.end);
    }
    if (info.result == LocalInfo::ambiguous && z == Choose::latest) {
      return Ret(tp.time_since_epoch()) - info.second.offset;
    }
    return Ret(tp.time_since_epoch()) - info.first.offset;
  }

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

  /// Compares the names of two time zones.
  friend bool operator==(const TimeZone& lhs, const TimeZone& rhs) noexcept { return lhs.name_ == rhs.name_; }

  /// Compares the names of two time zones.
  friend std::strong_ordering
  operator<=>(const TimeZone& lhs, const TimeZone& rhs) noexcept {
    return lhs.name_ <=> rhs.name_;
  }

private:

  friend struct internal::TimeZoneAccess;

  std::string name_;
  std::shared_ptr<const internal::ZoneData> data_;

  TimeZone(std::string name, std::shared_ptr<const internal::ZoneData> data) :
    name_(std::move(name)),
    data_(std::move(data)) {}

  [[nodiscard]] SysInfo getInfo(std::chrono::sys_seconds tp) const;

  [[nodiscard]] LocalInfo getLocalInfo(std::chrono::local_seconds tp) const;
};

/**
 * Locates a time zone by name, equivalent to C++20's #std::chrono::locate_zone.
 *
 * @ThreadSafe
 *
 * @param name the name of the time zone, for example `Europe/Berlin`
 * @return the time zone, valid until the program ends
 * @throw std::runtime_error if the time zone cannot be located
 */
const TimeZone* locateZone(std::string_view name);

/**
 * Returns the current time zone of the computer, equivalent to C++20's #std::chrono::current_zone.
 *
 * The time zone is taken from the environment variable `TZ`, the symbolic link `/etc/localtime`, or the
 * file `/etc/timezone`, in this order. If none of these yields a time zone, the result is `UTC`.
 *
 * @ThreadSafe
 *
 * @return the current time zone, valid until the program ends
 */
const TimeZone* currentZone();

/**
 * The traits of a time-zone pointer used by #rocket::chrono::ZonedTime, equivalent to C++20's
 * #std::chrono::zoned_traits.
 *
 * Specializations provide the static member functions `default_zone()` and `locate_zone(std::string_view)`.
 *
 * @tparam TimeZonePtr the time-zone pointer
 */
template<typename TimeZonePtr>
struct ZonedTraits {};

// XXX
/// @spec{#rocket::chrono::ZonedTraits, `const TimeZone*`}
template<>
struct ZonedTraits<const TimeZone*> {
  /// Returns `UTC`.
  static const TimeZone* default_zone() { return locateZone("UTC"); }

  /// Locates a time zone by name.
  static const TimeZone* locate_zone(std::string_view name) { return locateZone(name); }
};

/**
 * A time point together with a time zone, equivalent to C++20's #std::chrono::zoned_time.
 *
 * @tparam Duration the duration of the time point
 * @tparam TimeZonePtr the time-zone pointer
 *
 * @NotThreadSafe
 */
template<typename Duration, typename TimeZonePtr = const TimeZone*>
class ZonedTime {
  using Traits = ZonedTraits<TimeZonePtr>;

  template<typename D>
  using SysTime = std::chrono::sys_time<D>;

  template<typename D>
  using LocalTime = std::chrono::local_time<D>;

public:

  /// The duration of the time point, at least as fine as seconds.
  using duration = std::common_type_t<Duration, std::chrono::seconds>;

private:

  static constexpr bool HasDefaultZone = requires { Traits::default_zone(); };

  static constexpr bool HasLocateZone = requires(std::string_view name) {
    { Traits::locate_zone(name) } -> std::convertible_to<TimeZonePtr>;
  };

  static constexpr bool HasToSys = requires(TimeZonePtr& z) {
    { z->to_sys(LocalTime<Duration>()) } -> std::convertible_to<SysTime<duration>>;
  };

  static constexpr bool HasToSysChoose = requires(TimeZonePtr& z) {
    { z->to_sys(LocalTime<Duration>(), Choose::earliest) } -> std::convertible_to<SysTime<duration>>;
  };

public:

  /// @ctor_default
  ZonedTime()
    requires HasDefaultZone
    : zone_(Traits::default_zone()) {}

  ZonedTime(const ZonedTime&) = default;

  /**
   * @ctor
   *
   * @param st the UTC time point; the time zone is the default zone
   */
  ZonedTime(const SysTime<Duration>& st) // NOLINT(*-explicit-*)
    requires HasDefaultZone
    : zone_(Traits::default_zone()),
      tp_(st) {}

  /**
   * @ctor
   *
   * @param z the time zone
   */
  explicit ZonedTime(TimeZonePtr z) : zone_(std::move(z)) {}

  /**
   * @ctor
   *
   * @param name the name of the time zone
   */
  explicit ZonedTime(std::string_view name)
    requires HasLocateZone
    : zone_(Traits::locate_zone(name)) {}

  /**
   * @ctor
   *
   * @param y the zoned time to convert
   */
  template<typename Duration2>
    requires std::is_convertible_v<SysTime<Duration2>, SysTime<Duration>>
  ZonedTime(const ZonedTime<Duration2, TimeZonePtr>& y) : // NOLINT(*-explicit-*)
    zone_(y.get_time_zone()),
    tp_(y.get_sys_time()) {}

  /**
   * @ctor
   *
   * @param z the time zone
   * @param st the UTC time point
   */
  ZonedTime(TimeZonePtr z, const SysTime<Duration>& st) : zone_(std::move(z)), tp_(st) {}

  /**
   * @ctor
   *
   * @param name the name of the time zone
   * @param st the UTC time point
   */
  ZonedTime(std::string_view name, const SysTime<Duration>& st)
    requires HasLocateZone
    : ZonedTime(Traits::locate_zone(name), st) {}

  /**
   * @ctor
   *
   * @param z the time zone
   * @param tp the local time
   * @throw NonexistentLocalTime if @p tp does not exist
   * @throw AmbiguousLocalTime if @p tp is ambiguous
   */
  ZonedTime(TimeZonePtr z, const LocalTime<Duration>& tp)
    requires HasToSys
    : zone_(std::move(z)),
      tp_(zone_->to_sys(tp)) {}

  /**
   * @ctor
   *
   * @param name the name of the time zone
   * @param tp the local time
   * @throw NonexistentLocalTime if @p tp does not exist
   * @throw AmbiguousLocalTime if @p tp is ambiguous
   */
  ZonedTime(std::string_view name, const LocalTime<Duration>& tp)
    requires HasLocateZone && HasToSys
    : ZonedTime(Traits::locate_zone(name), tp) {}

  /**
   * @ctor
   *
   * @param z the time zone
   * @param tp the local time
   * @param c how to resolve an ambiguous local time
   */
  ZonedTime(TimeZonePtr z, const LocalTime<Duration>& tp, Choose c)
    requires HasToSysChoose
    : zone_(std::move(z)),
      tp_(zone_->to_sys(tp, c)) {}

  /**
   * @ctor
   *
   * @param name the name of the time zone
   * @param tp the local time
   * @param c how to resolve an ambiguous local time
   */
  ZonedTime(std::string_view name, const LocalTime<Duration>& tp, Choose c)
    requires HasLocateZone && HasToSysChoose
    : ZonedTime(Traits::locate_zone(name), tp, c) {}

  /**
   * @ctor
   *
   * @param z the time zone
   * @param y the zoned time whose UTC time point to use
   */
  template<typename Duration2, typename TimeZonePtr2>
    requires std::is_convertible_v<SysTime<Duration2>, SysTime<Duration>>
  ZonedTime(TimeZonePtr z, const ZonedTime<Duration2, TimeZonePtr2>& y) :
    zone_(std::move(z)),
    tp_(y.get_sys_time()) {}

  /**
   * @ctor
   *
   * The argument @p c has no effect; it exists for compatibility with #std::chrono::zoned_time.
   *
   * @param z the time zone
   * @param y the zoned time whose UTC time point to use
   * @param c ignored
   */
  template<typename Duration2, typename TimeZonePtr2>
    requires std::is_convertible_v<SysTime<Duration2>, SysTime<Duration>>
  ZonedTime(TimeZonePtr z, const ZonedTime<Duration2, TimeZonePtr2>& y, [[maybe_unused]] Choose c) :
    ZonedTime(std::move(z), y) {}

  /**
   * @ctor
   *
   * @param name the name of the time zone
   * @param y the zoned time whose UTC time point to use
   */
  template<typename Duration2, typename TimeZonePtr2>
    requires HasLocateZone && std::is_convertible_v<SysTime<Duration2>, SysTime<Duration>>
  ZonedTime(std::string_view name, const ZonedTime<Duration2, TimeZonePtr2>& y) :
    ZonedTime(Traits::locate_zone(name), y) {}

  /**
   * @ctor
   *
   * The argument @p c has no effect; it exists for compatibility with #std::chrono::zoned_time.
   *
   * @param name the name of the time zone
   * @param y the zoned time whose UTC time point to use
   * @param c ignored
   */
  template<typename Duration2, typename TimeZonePtr2>
    requires HasLocateZone && std::is_convertible_v<SysTime<Duration2>, SysTime<Duration>>
  ZonedTime(std::string_view name, const ZonedTime<Duration2, TimeZonePtr2>& y, [[maybe_unused]] Choose c) :
    ZonedTime(Traits::locate_zone(name), y) {}

  ~ZonedTime() = default;

  ZonedTime& operator=(const ZonedTime&) = default;

  /**
   * Sets the UTC time point, keeping the time zone.
   *
   * @param st the UTC time point
   * @return this instance
   */
  ZonedTime&
  operator=(const SysTime<Duration>& st) {
    tp_ = st;
    return *this;
  }

  /**
   * Sets the local time, keeping the time zone.
   *
   * @param lt the local time
   * @return this instance
   * @throw NonexistentLocalTime if @p lt does not exist
   * @throw AmbiguousLocalTime if @p lt is ambiguous
   */
  ZonedTime&
  operator=(const LocalTime<Duration>& lt) {
    tp_ = zone_->to_sys(lt);
    return *this;
  }

  /// Returns the UTC time point.
  operator SysTime<duration>() const { return get_sys_time(); } // NOLINT(*-explicit-*)

  /// Returns the local time.
  explicit operator LocalTime<duration>() const { return get_local_time(); }

  /**
   * Returns the time zone.
   *
   * @return the time zone
   */
  [[nodiscard]] TimeZonePtr get_time_zone() const { return zone_; }

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
   * Returns the information about the time zone at the time point.
   *
   * @return the information
   */
  [[nodiscard]] SysInfo get_info() const { return zone_->get_info(tp_); }

private:

  TimeZonePtr zone_;
  SysTime<duration> tp_;
};

/// @cond undocumented

namespace internal {

template<typename T>
using TimeZoneRepresentation =
  std::conditional_t<std::is_convertible_v<T, std::string_view>, const TimeZone*, std::remove_cvref_t<T>>;

} // namespace internal

ZonedTime() -> ZonedTime<std::chrono::seconds>;

template<typename Duration>
ZonedTime(std::chrono::sys_time<Duration>) -> ZonedTime<std::common_type_t<Duration, std::chrono::seconds>>;

template<typename TimeZonePtrOrName>
ZonedTime(TimeZonePtrOrName&&)
  -> ZonedTime<std::chrono::seconds, internal::TimeZoneRepresentation<TimeZonePtrOrName>>;

template<typename TimeZonePtrOrName, typename Duration>
ZonedTime(TimeZonePtrOrName&&, std::chrono::sys_time<Duration>)
  -> ZonedTime<
    std::common_type_t<Duration, std::chrono::seconds>,
    internal::TimeZoneRepresentation<TimeZonePtrOrName>>;

template<typename TimeZonePtrOrName, typename Duration>
ZonedTime(TimeZonePtrOrName&&, std::chrono::local_time<Duration>, Choose = Choose::earliest)
  -> ZonedTime<
    std::common_type_t<Duration, std::chrono::seconds>,
    internal::TimeZoneRepresentation<TimeZonePtrOrName>>;

template<typename Duration, typename TimeZonePtrOrName, typename TimeZonePtr2>
ZonedTime(TimeZonePtrOrName&&, ZonedTime<Duration, TimeZonePtr2>, Choose = Choose::earliest)
  -> ZonedTime<Duration, internal::TimeZoneRepresentation<TimeZonePtrOrName>>;

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
operator==(const ZonedTime<Duration1, TimeZonePtr>& lhs, const ZonedTime<Duration2, TimeZonePtr>& rhs) {
  return lhs.get_time_zone() == rhs.get_time_zone() && lhs.get_sys_time() == rhs.get_sys_time();
}

/**
 * Writes a zoned time to an output stream, using the format `%F %T %Z`.
 *
 * @param os the output stream
 * @param val the value to write
 * @return @p os
 */
template<typename CharT, typename Traits, typename Duration, typename TimeZonePtr>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const ZonedTime<Duration, TimeZonePtr>& val) {
  if constexpr (std::same_as<CharT, char>) {
    return os << std::format(os.getloc(), "{:L%F %T %Z}", val);
  } else {
    return os << std::format(os.getloc(), L"{:L%F %T %Z}", val);
  }
}

#endif // ROCKET_HAS_CHRONO_TIME_ZONE_IMPL

/// A zoned time with a precision of seconds.
using ZonedSeconds = ZonedTime<std::chrono::seconds>;

} // namespace rocket::chrono

// EOF
