/*
 * test-chrono.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/chrono/chrono.h"

#include <cstdlib>
#include <format>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace std::chrono;

using rocket::chrono::AmbiguousLocalTime;
using rocket::chrono::Choose;
using rocket::chrono::LocalInfo;
using rocket::chrono::NonexistentLocalTime;
using rocket::chrono::TimeZone;
using rocket::chrono::ZonedSeconds;
using rocket::chrono::ZonedTime;
using rocket::chrono::currentZone;
using rocket::chrono::locateZone;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(chrono, locateZone) {
  const TimeZone* berlin = locateZone("Europe/Berlin");
  ASSERT_NE(berlin, nullptr);
  EXPECT_EQ(berlin->name(), "Europe/Berlin");
  EXPECT_EQ(locateZone("Europe/Berlin"), berlin);

  // Links resolve to their targets

  EXPECT_EQ(locateZone("UTC")->name(), "Etc/UTC");
  EXPECT_EQ(locateZone("UTC"), locateZone("Etc/UTC"));
  EXPECT_EQ(locateZone("US/Eastern")->name(), "America/New_York");

  // Unknown or invalid names

  EXPECT_THROW(static_cast<void>(locateZone("x")), std::runtime_error);
  EXPECT_THROW(static_cast<void>(locateZone("")), std::runtime_error);
  EXPECT_THROW(static_cast<void>(locateZone("Europe")), std::runtime_error);
  EXPECT_THROW(static_cast<void>(locateZone("../../../etc/passwd")), std::runtime_error);
  EXPECT_THROW(static_cast<void>(locateZone("/etc/passwd")), std::runtime_error);
  EXPECT_THROW(static_cast<void>(locateZone("zone.tab")), std::runtime_error);
}

TEST(chrono, currentZone) {
  ASSERT_NE(currentZone(), nullptr);
  EXPECT_EQ(locateZone(currentZone()->name()), currentZone());
}

TEST(chrono, TimeZoneOrdering) {
  EXPECT_TRUE(*locateZone("Europe/Berlin") == *locateZone("Europe/Berlin"));
  EXPECT_TRUE(*locateZone("Europe/Berlin") < *locateZone("Europe/Paris"));
}

TEST(chrono, SysInfo) {
  const TimeZone* berlin = locateZone("Europe/Berlin");

  const auto summer = berlin->get_info(sys_days(2024y / July / 1) + 12h);
  EXPECT_EQ(summer.begin, sys_days(2024y / March / 31) + 1h);
  EXPECT_EQ(summer.end, sys_days(2024y / October / 27) + 1h);
  EXPECT_EQ(summer.offset, 2h);
  EXPECT_EQ(summer.save, 60min);
  EXPECT_EQ(summer.abbrev, "CEST");

  const auto winter = berlin->get_info(sys_days(2024y / January / 1));
  EXPECT_EQ(winter.end, sys_days(2024y / March / 31) + 1h);
  EXPECT_EQ(winter.offset, 1h);
  EXPECT_EQ(winter.save, 0min);
  EXPECT_EQ(winter.abbrev, "CET");

  // Periods begin and end exactly at the transitions

  EXPECT_EQ(berlin->get_info(summer.begin - 1s).abbrev, "CET");
  EXPECT_EQ(berlin->get_info(summer.begin).abbrev, "CEST");
  EXPECT_EQ(berlin->get_info(summer.end - 1s).abbrev, "CEST");
  EXPECT_EQ(berlin->get_info(summer.end).abbrev, "CET");

  // Sub-second precision

  EXPECT_EQ(berlin->get_info(sys_time<nanoseconds>(summer.begin) - 1ns).abbrev, "CET");
}

TEST(chrono, SysInfoFuture) {
  // Far beyond the transitions listed in the files, the rules apply

  const auto berlin = locateZone("Europe/Berlin")->get_info(sys_days(2200y / July / 1));
  EXPECT_EQ(berlin.offset, 2h);
  EXPECT_EQ(berlin.abbrev, "CEST");
  EXPECT_EQ(berlin.begin, sys_days(2200y / March / 30) + 1h); // Last Sunday in March
  EXPECT_EQ(berlin.end, sys_days(2200y / October / 26) + 1h); // Last Sunday in October

  const auto newYork = locateZone("America/New_York")->get_info(sys_days(2200y / January / 1));
  EXPECT_EQ(newYork.offset, -5h);
  EXPECT_EQ(newYork.save, 0min);
  EXPECT_EQ(newYork.abbrev, "EST");
}

TEST(chrono, SysInfoSouthernHemisphere) {
  const TimeZone* sydney = locateZone("Australia/Sydney");
  EXPECT_EQ(sydney->get_info(sys_days(2030y / January / 1)).offset, 11h);
  EXPECT_EQ(sydney->get_info(sys_days(2030y / July / 1)).offset, 10h);

  // Half-hour DST

  const auto lordHowe = locateZone("Australia/Lord_Howe")->get_info(sys_days(2030y / January / 1));
  EXPECT_EQ(lordHowe.offset, 11h);
  EXPECT_EQ(lordHowe.save, 30min);
}

TEST(chrono, SysInfoNoDst) {
  const auto utc = locateZone("UTC")->get_info(sys_days(2024y / July / 1));
  EXPECT_EQ(utc.offset, 0s);
  EXPECT_EQ(utc.save, 0min);
  EXPECT_EQ(utc.abbrev, "UTC");

  const auto kolkata = locateZone("Asia/Kolkata")->get_info(sys_days(2100y / July / 1));
  EXPECT_EQ(kolkata.offset, 5h + 30min);
  EXPECT_EQ(kolkata.abbrev, "IST");
}

TEST(chrono, LocalInfo) {
  const TimeZone* berlin = locateZone("Europe/Berlin");

  const auto unique = berlin->get_info(local_days(2024y / July / 1) + 12h);
  EXPECT_EQ(unique.result, LocalInfo::unique);
  EXPECT_EQ(unique.first.abbrev, "CEST");

  const auto gap = berlin->get_info(local_days(2024y / March / 31) + 2h + 30min);
  EXPECT_EQ(gap.result, LocalInfo::nonexistent);
  EXPECT_EQ(gap.first.abbrev, "CET");
  EXPECT_EQ(gap.second.abbrev, "CEST");

  const auto overlap = berlin->get_info(local_days(2024y / October / 27) + 2h + 30min);
  EXPECT_EQ(overlap.result, LocalInfo::ambiguous);
  EXPECT_EQ(overlap.first.abbrev, "CEST");
  EXPECT_EQ(overlap.second.abbrev, "CET");

  // The edges of the gap

  EXPECT_EQ(berlin->get_info(local_days(2024y / March / 31) + 2h - 1s).result, LocalInfo::unique);
  EXPECT_EQ(berlin->get_info(local_days(2024y / March / 31) + 2h).result, LocalInfo::nonexistent);
  EXPECT_EQ(berlin->get_info(local_days(2024y / March / 31) + 3h).result, LocalInfo::unique);
}

TEST(chrono, ToSys) {
  const TimeZone* berlin = locateZone("Europe/Berlin");

  EXPECT_EQ(berlin->to_sys(local_days(2024y / July / 1) + 12h), sys_days(2024y / July / 1) + 10h);

  const auto gap = local_days(2024y / March / 31) + 2h + 30min;
  EXPECT_THROW(static_cast<void>(berlin->to_sys(gap)), NonexistentLocalTime);
  EXPECT_EQ(berlin->to_sys(gap, Choose::earliest), sys_days(2024y / March / 31) + 1h);
  EXPECT_EQ(berlin->to_sys(gap, Choose::latest), sys_days(2024y / March / 31) + 1h);

  const auto overlap = local_days(2024y / October / 27) + 2h + 30min;
  EXPECT_THROW(static_cast<void>(berlin->to_sys(overlap)), AmbiguousLocalTime);
  EXPECT_EQ(berlin->to_sys(overlap, Choose::earliest), sys_days(2024y / October / 27) + 30min);
  EXPECT_EQ(berlin->to_sys(overlap, Choose::latest), sys_days(2024y / October / 27) + 1h + 30min);

  // Precision is kept

  const auto ms = berlin->to_sys(local_time<milliseconds>(local_days(2024y / July / 1) + 12h + 5ms));
  EXPECT_EQ(ms, sys_time<milliseconds>(sys_days(2024y / July / 1) + 10h + 5ms));
}

TEST(chrono, ToSysMessages) {
  const TimeZone* berlin = locateZone("Europe/Berlin");

  try {
    static_cast<void>(berlin->to_sys(local_days(2024y / March / 31) + 2h + 30min));
    FAIL();
  } catch (const NonexistentLocalTime& ex) {
    EXPECT_EQ(std::string(ex.what()),
      "2024-03-31 02:30:00 is in a gap between\n"
      "2024-03-31 02:00:00 CET and\n"
      "2024-03-31 03:00:00 CEST which are both equivalent to\n"
      "2024-03-31 01:00:00 UTC");
  }

  try {
    static_cast<void>(berlin->to_sys(local_days(2024y / October / 27) + 2h + 30min));
    FAIL();
  } catch (const AmbiguousLocalTime& ex) {
    EXPECT_EQ(std::string(ex.what()),
      "2024-10-27 02:30:00 is ambiguous.  It could be\n"
      "2024-10-27 02:30:00 CEST == 2024-10-27 00:30:00 UTC or\n"
      "2024-10-27 02:30:00 CET == 2024-10-27 01:30:00 UTC");
  }
}

TEST(chrono, ToLocal) {
  const TimeZone* newYork = locateZone("America/New_York");
  const auto utc = sys_days(2024y / July / 1) + 12h;
  EXPECT_EQ(newYork->to_local(utc), local_days(2024y / July / 1) + 8h);
  EXPECT_EQ(newYork->to_sys(newYork->to_local(utc)), utc);
}

TEST(chrono, ZonedTime) {
  const auto utc = sys_days(2024y / July / 1) + 12h;

  // Default zone is UTC

  const ZonedSeconds def;
  EXPECT_EQ(def.get_time_zone()->name(), "Etc/UTC");
  EXPECT_EQ(def.get_sys_time(), sys_seconds());
  EXPECT_EQ(ZonedTime(utc).get_time_zone()->name(), "Etc/UTC");

  // By pointer and by name

  const auto zt = ZonedTime(locateZone("Europe/Berlin"), utc);
  EXPECT_EQ(zt.get_time_zone()->name(), "Europe/Berlin");
  EXPECT_EQ(zt.get_sys_time(), utc);
  EXPECT_EQ(zt.get_local_time(), local_days(2024y / July / 1) + 14h);
  EXPECT_EQ(zt.get_info().abbrev, "CEST");
  EXPECT_EQ(static_cast<sys_seconds>(zt), utc);
  EXPECT_EQ(static_cast<local_seconds>(zt), local_days(2024y / July / 1) + 14h);

  const auto byName = ZonedTime("Europe/Berlin", utc);
  EXPECT_TRUE(byName == zt);

  // From local time

  const auto fromLocal = ZonedTime("Europe/Berlin", local_days(2024y / July / 1) + 14h);
  EXPECT_EQ(fromLocal.get_sys_time(), utc);
  EXPECT_THROW(ZonedTime("Europe/Berlin", local_days(2024y / March / 31) + 2h + 30min), NonexistentLocalTime);
  const auto latest = ZonedTime("Europe/Berlin", local_days(2024y / October / 27) + 2h + 30min, Choose::latest);
  EXPECT_EQ(latest.get_info().abbrev, "CET");

  // Another zone, same time point

  const auto newYork = ZonedTime("America/New_York", zt);
  EXPECT_EQ(newYork.get_sys_time(), utc);
  EXPECT_EQ(newYork.get_local_time(), local_days(2024y / July / 1) + 8h);
  EXPECT_FALSE(newYork == zt);

  // Precision

  const ZonedTime<milliseconds> ms = zt;
  EXPECT_EQ(ms.get_sys_time(), utc);
  const auto ns = ZonedTime(locateZone("Europe/Berlin"), sys_time<nanoseconds>(utc) + 1ns);
  static_assert(std::is_same_v<decltype(ns)::duration, nanoseconds>);
  EXPECT_FALSE(ns == zt);

  // Assignment

  auto assigned = zt;
  assigned = local_days(2024y / January / 1) + 12h;
  EXPECT_EQ(assigned.get_sys_time(), sys_days(2024y / January / 1) + 11h);
  assigned = utc;
  EXPECT_TRUE(assigned == zt);
}

TEST(chrono, ZonedTimeFormat) {
  const auto utc = sys_days(2024y / July / 1) + 12h;
  const auto berlin = ZonedTime(locateZone("Europe/Berlin"), utc);
  const auto newYork = ZonedTime(locateZone("America/New_York"), utc);

  EXPECT_EQ(std::format("{:%F %T %Z}", berlin), "2024-07-01 14:00:00 CEST");
  EXPECT_EQ(std::format("{:%FT%T%Ez}", berlin), "2024-07-01T14:00:00+02:00");
  EXPECT_EQ(std::format("{:%FT%T%z}", newYork), "2024-07-01T08:00:00-0400");
  EXPECT_EQ(std::format("{:%FT%T%Oz}", newYork), "2024-07-01T08:00:00-04:00");
  EXPECT_EQ(std::format("{:%H%%%Z}", berlin), "14%CEST");
  EXPECT_EQ(std::format("{}", berlin), "2024-07-01 14:00:00 CEST");
  EXPECT_EQ(std::format("{:>30}", berlin), "      2024-07-01 14:00:00 CEST");
  EXPECT_EQ(std::format("{:%FT%TZ}", ZonedTime(locateZone("UTC"), sys_time<milliseconds>(utc) + 5ms)),
    "2024-07-01T12:00:00.005Z");

  std::ostringstream os;
  os << berlin;
  EXPECT_EQ(os.str(), "2024-07-01 14:00:00 CEST");
}

#if !ROCKET_CHRONO_STD_TIME_ZONE

TEST(chrono, currentZoneTz) {
  const char* old = std::getenv("TZ"); // NOLINT(*-mt-unsafe)
  const std::string saved = old != nullptr ? old : "";

  ::setenv("TZ", "America/New_York", 1); // NOLINT(*-mt-unsafe)
  EXPECT_EQ(currentZone()->name(), "America/New_York");
  ::setenv("TZ", ":Asia/Tokyo", 1); // NOLINT(*-mt-unsafe)
  EXPECT_EQ(currentZone()->name(), "Asia/Tokyo");

  if (old != nullptr) {
    ::setenv("TZ", saved.c_str(), 1); // NOLINT(*-mt-unsafe)
  } else {
    ::unsetenv("TZ"); // NOLINT(*-mt-unsafe)
  }
}

#endif

// EOF
