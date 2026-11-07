/*
 * test-chrono.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/scope.h"
#include "rocket/chrono/chrono.h"
#include "rocket/system/system.h"

#include <format>
#include <sstream>
#include <string>
#include <string_view>

using namespace std::chrono; // XXX

using rocket::chrono::ambiguous_local_time;
using rocket::chrono::choose;
using rocket::chrono::local_info;
using rocket::chrono::nonexistent_local_time;
using rocket::chrono::time_zone;
using rocket::chrono::zoned_seconds;
using rocket::chrono::zoned_time;

using rocket::chrono::current_zone;
using rocket::chrono::locate_zone;

namespace {

// Constants ------------------------------------------------------------------------------------------------

/// Central European Time, localized.
const string CET = rocket::chrono::timeZoneAbbrev("Europe/Berlin", false, "CET");
/// Central European Summer Time, localized.
const string CEST = rocket::chrono::timeZoneAbbrev("Europe/Berlin", true, "CEST");
/// Eastern Standard Time, localized.
const string EST = rocket::chrono::timeZoneAbbrev("America/New_York", false, "EST");
/// India Standard Time, localized.
const string IST = rocket::chrono::timeZoneAbbrev("Asia/Kolkata", false, "IST");
/// Coordinated Universal Time, localized.
const string UTC = rocket::chrono::timeZoneAbbrev("UTC", false, "UTC");

} // namespace

// `TEST`---------------------------------------------------------------------------------------------------

// `sys_info` ...............................................................................................

TEST(chrono, sys_info) {
  const time_zone* berlin = locate_zone("Europe/Berlin");

  const auto summer = berlin->get_info(sys_days(2024y / July / 1) + 12h);
  EXPECT_EQ(summer.begin, sys_days(2024y / March / 31) + 1h);
  EXPECT_EQ(summer.end, sys_days(2024y / October / 27) + 1h);
  EXPECT_EQ(summer.offset, 2h);
  EXPECT_EQ(summer.save, 60min);
  EXPECT_EQ(summer.abbrev, CEST);

  const auto winter = berlin->get_info(sys_days(2024y / January / 1));
  EXPECT_EQ(winter.end, sys_days(2024y / March / 31) + 1h);
  EXPECT_EQ(winter.offset, 1h);
  EXPECT_EQ(winter.save, 0min);
  EXPECT_EQ(winter.abbrev, CET);

  // Periods begin and end exactly at the transitions

  EXPECT_EQ(berlin->get_info(summer.begin - 1s).abbrev, CET);
  EXPECT_EQ(berlin->get_info(summer.begin).abbrev, CEST);
  EXPECT_EQ(berlin->get_info(summer.end - 1s).abbrev, CEST);
  EXPECT_EQ(berlin->get_info(summer.end).abbrev, CET);

  // Sub-second precision

#ifdef ROCKET_OS_WINDOWS
  EXPECT_EQ(berlin->get_info(sys_time<microseconds>(summer.begin) - 1us).abbrev, CET);
#else
  EXPECT_EQ(berlin->get_info(sys_time<nanoseconds>(summer.begin) - 1ns).abbrev, CET);
#endif
}

TEST(chrono, sys_info_future) {
  // Far beyond the transitions listed in the files, the rules apply

  const auto berlin = locate_zone("Europe/Berlin")->get_info(sys_days(2200y / July / 1));
  EXPECT_EQ(berlin.offset, 2h);
  EXPECT_EQ(berlin.abbrev, CEST);
  EXPECT_EQ(berlin.begin, sys_days(2200y / March / 30) + 1h); // Last Sunday in March
  EXPECT_EQ(berlin.end, sys_days(2200y / October / 26) + 1h); // Last Sunday in October

  const auto newYork = locate_zone("America/New_York")->get_info(sys_days(2200y / January / 1));
  EXPECT_EQ(newYork.offset, -5h);
  EXPECT_EQ(newYork.save, 0min);
  EXPECT_EQ(newYork.abbrev, EST);
}

TEST(chrono, sys_info_southern_hemisphere) {
  const time_zone* sydney = locate_zone("Australia/Sydney");
  EXPECT_EQ(sydney->get_info(sys_days(2030y / January / 1)).offset, 11h);
  EXPECT_EQ(sydney->get_info(sys_days(2030y / July / 1)).offset, 10h);

  // Half-hour DST

  const auto lordHowe = locate_zone("Australia/Lord_Howe")->get_info(sys_days(2030y / January / 1));
  EXPECT_EQ(lordHowe.offset, 11h);
  EXPECT_EQ(lordHowe.save, 30min);
}

TEST(chrono, sys_info_no_dst) {
  const auto utc = locate_zone("UTC")->get_info(sys_days(2024y / July / 1));
  EXPECT_EQ(utc.offset, 0s);
  EXPECT_EQ(utc.save, 0min);
  EXPECT_EQ(utc.abbrev, UTC);

  const auto kolkata = locate_zone("Asia/Kolkata")->get_info(sys_days(2100y / July / 1));
  EXPECT_EQ(kolkata.offset, 5h + 30min);
  EXPECT_EQ(kolkata.abbrev, IST);
}

// `local_info` .............................................................................................

TEST(chrono, local_info) {
  const time_zone* berlin = locate_zone("Europe/Berlin");

  const auto unique = berlin->get_info(local_days(2024y / July / 1) + 12h);
  EXPECT_EQ(unique.result, local_info::unique);
  EXPECT_EQ(unique.first.abbrev, CEST);

  const auto gap = berlin->get_info(local_days(2024y / March / 31) + 2h + 30min);
  EXPECT_EQ(gap.result, local_info::nonexistent);
  EXPECT_EQ(gap.first.abbrev, CET);
  EXPECT_EQ(gap.second.abbrev, CEST);

  const auto overlap = berlin->get_info(local_days(2024y / October / 27) + 2h + 30min);
  EXPECT_EQ(overlap.result, local_info::ambiguous);
  EXPECT_EQ(overlap.first.abbrev, CEST);
  EXPECT_EQ(overlap.second.abbrev, CET);

  // The edges of the gap

  EXPECT_EQ(berlin->get_info(local_days(2024y / March / 31) + 2h - 1s).result, local_info::unique);
  EXPECT_EQ(berlin->get_info(local_days(2024y / March / 31) + 2h).result, local_info::nonexistent);
  EXPECT_EQ(berlin->get_info(local_days(2024y / March / 31) + 3h).result, local_info::unique);
}

// `time_zone` ..............................................................................................

TEST(chrono, time_zone_ordering) {
  EXPECT_TRUE(*locate_zone("Europe/Berlin") == *locate_zone("Europe/Berlin"));
  EXPECT_TRUE(*locate_zone("Europe/Berlin") < *locate_zone("Europe/Paris"));
}

TEST(chrono, time_zone_to_local) {
  const time_zone* newYork = locate_zone("America/New_York");
  const auto utc = sys_days(2024y / July / 1) + 12h;
  EXPECT_EQ(newYork->to_local(utc), local_days(2024y / July / 1) + 8h);
  EXPECT_EQ(newYork->to_sys(newYork->to_local(utc)), utc);
}

TEST(chrono, time_zone_to_sys) {
  const time_zone* berlin = locate_zone("Europe/Berlin");

  EXPECT_EQ(berlin->to_sys(local_days(2024y / July / 1) + 12h), sys_days(2024y / July / 1) + 10h);

  const auto gap = local_days(2024y / March / 31) + 2h + 30min;
  EXPECT_THROW(static_cast<void>(berlin->to_sys(gap)), nonexistent_local_time);
  EXPECT_EQ(berlin->to_sys(gap, choose::earliest), sys_days(2024y / March / 31) + 1h);
  EXPECT_EQ(berlin->to_sys(gap, choose::latest), sys_days(2024y / March / 31) + 1h);

  const auto overlap = local_days(2024y / October / 27) + 2h + 30min;
  EXPECT_THROW(static_cast<void>(berlin->to_sys(overlap)), ambiguous_local_time);
  EXPECT_EQ(berlin->to_sys(overlap, choose::earliest), sys_days(2024y / October / 27) + 30min);
  EXPECT_EQ(berlin->to_sys(overlap, choose::latest), sys_days(2024y / October / 27) + 1h + 30min);

  // Precision is kept

  const auto ms = berlin->to_sys(local_time<milliseconds>(local_days(2024y / July / 1) + 12h + 5ms));
  EXPECT_EQ(ms, sys_time<milliseconds>(sys_days(2024y / July / 1) + 10h + 5ms));
}

TEST(chrono, time_zone_to_sys_messages) {
  const time_zone* berlin = locate_zone("Europe/Berlin");

  try {
    static_cast<void>(berlin->to_sys(local_days(2024y / October / 27) + 2h + 30min));
    FAIL();
  } catch (const ambiguous_local_time& ex) {
    EXPECT_EQ(std::string(ex.what()),
      "2024-10-27 02:30:00 is ambiguous.  It could be\n"
      "2024-10-27 02:30:00 " + CEST + " == 2024-10-27 00:30:00 UTC or\n"
      "2024-10-27 02:30:00 " + CET + " == 2024-10-27 01:30:00 UTC");
  }

  try {
    static_cast<void>(berlin->to_sys(local_days(2024y / March / 31) + 2h + 30min));
    FAIL();
  } catch (const nonexistent_local_time& ex) {
    EXPECT_EQ(std::string(ex.what()),
      "2024-03-31 02:30:00 is in a gap between\n"
      "2024-03-31 02:00:00 " + CET + " and\n"
      "2024-03-31 03:00:00 " + CEST + " which are both equivalent to\n"
      "2024-03-31 01:00:00 UTC");
  }
}

TEST(chrono, current_zone) {
  ASSERT_NE(current_zone(), nullptr);
  EXPECT_EQ(locate_zone(current_zone()->name()), current_zone());
}

TEST(chrono, locate_zone) {
  const time_zone* berlin = locate_zone("Europe/Berlin");
  ASSERT_NE(berlin, nullptr);
  EXPECT_EQ(berlin->name(), "Europe/Berlin");
  EXPECT_EQ(locate_zone("Europe/Berlin"), berlin);

  // Links resolve to their targets

  EXPECT_EQ(locate_zone("UTC")->name(), "Etc/UTC");
  EXPECT_EQ(locate_zone("UTC"), locate_zone("Etc/UTC"));
  EXPECT_EQ(locate_zone("US/Eastern")->name(), "America/New_York");

  // Unknown or invalid names

  EXPECT_THROW(static_cast<void>(locate_zone("x")), std::runtime_error);
  EXPECT_THROW(static_cast<void>(locate_zone("")), std::runtime_error);
  EXPECT_THROW(static_cast<void>(locate_zone("Europe")), std::runtime_error);
  EXPECT_THROW(static_cast<void>(locate_zone("../../../etc/passwd")), std::runtime_error);
  EXPECT_THROW(static_cast<void>(locate_zone("/etc/passwd")), std::runtime_error);
  EXPECT_THROW(static_cast<void>(locate_zone("zone.tab")), std::runtime_error);
}

// `zoned_time` .............................................................................................

TEST(chrono, zoned_time) {
  const auto utc = sys_days(2024y / July / 1) + 12h;

  // Default zone is UTC

  const zoned_seconds def;
  EXPECT_EQ(def.get_time_zone()->name(), "Etc/UTC");
  EXPECT_EQ(def.get_sys_time(), sys_seconds());
  EXPECT_EQ(zoned_time(utc).get_time_zone()->name(), "Etc/UTC");

  // By pointer and by name

  const auto zt = zoned_time(locate_zone("Europe/Berlin"), utc);
  EXPECT_EQ(zt.get_time_zone()->name(), "Europe/Berlin");
  EXPECT_EQ(zt.get_sys_time(), utc);
  EXPECT_EQ(zt.get_local_time(), local_days(2024y / July / 1) + 14h);
  EXPECT_EQ(zt.get_info().abbrev, CEST);
  EXPECT_EQ(static_cast<sys_seconds>(zt), utc);
  EXPECT_EQ(static_cast<local_seconds>(zt), local_days(2024y / July / 1) + 14h);

  const auto byName = zoned_time("Europe/Berlin", utc);
  EXPECT_TRUE(byName == zt);

  // From local time

  const auto fromLocal = zoned_time("Europe/Berlin", local_days(2024y / July / 1) + 14h);
  EXPECT_EQ(fromLocal.get_sys_time(), utc);
  EXPECT_THROW(zoned_time("Europe/Berlin", local_days(2024y / March / 31) + 2h + 30min), nonexistent_local_time);
  const auto latest = zoned_time("Europe/Berlin", local_days(2024y / October / 27) + 2h + 30min, choose::latest);
  EXPECT_EQ(latest.get_info().abbrev, CET);

  // Another zone, same time point

  const auto newYork = zoned_time("America/New_York", zt);
  EXPECT_EQ(newYork.get_sys_time(), utc);
  EXPECT_EQ(newYork.get_local_time(), local_days(2024y / July / 1) + 8h);
  EXPECT_FALSE(newYork == zt);

  // Precision

  const zoned_time<milliseconds> ms = zt;
  EXPECT_EQ(ms.get_sys_time(), utc);
  const auto ns = zoned_time(locate_zone("Europe/Berlin"), sys_time<nanoseconds>(utc) + 1ns);
  static_assert(std::is_same_v<decltype(ns)::duration, nanoseconds>);
  EXPECT_FALSE(ns == zt);

  // Assignment

  auto assigned = zt;
  assigned = local_days(2024y / January / 1) + 12h;
  EXPECT_EQ(assigned.get_sys_time(), sys_days(2024y / January / 1) + 11h);
  assigned = utc;
  EXPECT_TRUE(assigned == zt);
}

TEST(chrono, zoned_time_format) {
  const auto utc = sys_days(2024y / July / 1) + 12h;
  const auto berlin = zoned_time(locate_zone("Europe/Berlin"), utc);
  const auto newYork = zoned_time(locate_zone("America/New_York"), utc);

  EXPECT_EQ(std::format("{:%F %T %Z}", berlin), "2024-07-01 14:00:00 " + CEST);
  EXPECT_EQ(std::format("{:%FT%T%Ez}", berlin), "2024-07-01T14:00:00+02:00");
  EXPECT_EQ(std::format("{:%FT%T%z}", newYork), "2024-07-01T08:00:00-0400");
  EXPECT_EQ(std::format("{:%FT%T%Oz}", newYork), "2024-07-01T08:00:00-04:00");
  EXPECT_EQ(std::format("{:%H%%%Z}", berlin), "14%" + CEST);
  EXPECT_EQ(std::format("{}", berlin), "2024-07-01 14:00:00 " + CEST);
  EXPECT_EQ(std::format("{:>30}", berlin), std::format("{:>30}", "2024-07-01 14:00:00 " + CEST));
  EXPECT_EQ(std::format("{:%FT%TZ}", zoned_time(locate_zone("UTC"), sys_time<milliseconds>(utc) + 5ms)),
    "2024-07-01T12:00:00.005Z");

  std::ostringstream os;
  os << berlin;
  EXPECT_EQ(os.str(), "2024-07-01 14:00:00 " + CEST);
}

// EOF
