/*
 * chrono.cc
 */

#include "chrono.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <span>
#include <tuple>
#include <vector>

#ifdef _MSVC_STL_VERSION
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <Windows.h>
#endif

using namespace rocket;
using namespace rocket::chrono::internal;
using namespace std;
using namespace std::chrono;

using rocket::chrono::sys_info;
using rocket::chrono::time_zone;

// Part I: utilities related to `std::chrono` ===============================================================

namespace {

// Local functions ------------------------------------------------------------------------------------------

#ifdef _MSVC_STL_VERSION

// Converts a UTF-8 string to UTF-16, as needed by ICU
u16string
toUtf16(string_view str) {
  if (str.empty()) {
    return {};
  }
  const auto len = static_cast<i32>(str.size());
  const i32 size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), len, nullptr, 0);
  if (size <= 0) {
    return {};
  }
  u16string ret(static_cast<u16string::size_type>(size), u'\0');
  MultiByteToWideChar(
    CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), len, reinterpret_cast<wchar_t*>(ret.data()), size); // NOLINT
  return ret;
}

#endif // _MSVC_STL_VERSION

} // namespace

namespace rocket::chrono {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

std::recursive_mutex&
clockMutex() {
  static std::recursive_mutex mutex;
  return mutex;
}

milliseconds&
clockOffset() {
  static milliseconds offset = 0ms;
  return offset;
}

} // namespace internal

// Functions ------------------------------------------------------------------------------------------------

string
abbreviation([[maybe_unused]] string_view name, [[maybe_unused]] bool dst, string_view fallback) {
#ifdef _MSVC_STL_VERSION
  // We do not include any ICU header, as these are the headers of the ICU linked with Rocket
  using UCalendar = void;
  using UErrorCode = i32;
  constexpr i32 UCAL_GREGORIAN = 1;
  constexpr i32 UCAL_SHORT_STANDARD = 1;
  constexpr i32 UCAL_SHORT_DST = 3;
  using UcalOpen = UCalendar* (*)(const char16_t*, i32, const char*, i32, UErrorCode*);
  using UcalClose = void (*)(UCalendar*);
  using UcalGetTimeZoneDisplayName = i32 (*)(const UCalendar*, i32, const char*, char16_t*, i32, UErrorCode*);

  const HMODULE icu = LoadLibraryExW(L"icu.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (icu == nullptr) {
    return string(fallback);
  }
  const auto ucalOpen = reinterpret_cast<UcalOpen>(GetProcAddress(icu, "ucal_open")); // NOLINT
  const auto ucalClose = reinterpret_cast<UcalClose>(GetProcAddress(icu, "ucal_close")); // NOLINT
  const auto ucalGetTimeZoneDisplayName = reinterpret_cast<UcalGetTimeZoneDisplayName>( // NOLINT
    GetProcAddress(icu, "ucal_getTimeZoneDisplayName"));
  if (ucalOpen == nullptr || ucalClose == nullptr || ucalGetTimeZoneDisplayName == nullptr) {
    return string(fallback);
  }

  const u16string name16 = toUtf16(name);
  if (name16.empty()) {
    return string(fallback);
  }

  // A null locale is the default locale
  UErrorCode status = 0;
  UCalendar* const cal = ucalOpen(name16.c_str(), -1, nullptr, UCAL_GREGORIAN, &status);
  if (status > 0) {
    return string(fallback);
  }
  array<char16_t, 64> buf {};
  const i32 len = ucalGetTimeZoneDisplayName(
    cal, dst ? UCAL_SHORT_DST : UCAL_SHORT_STANDARD, nullptr, buf.data(), i32(buf.size()), &status);
  ucalClose(cal);
  if (status > 0 || len <= 0) {
    return string(fallback);
  }

  // Localized names may not be ASCII, for example `GMT−5` with U+2212 (MINUS SIGN)
  const auto* const wide = reinterpret_cast<const wchar_t*>(buf.data()); // NOLINT
  const i32 size = WideCharToMultiByte(CP_UTF8, 0, wide, len, nullptr, 0, nullptr, nullptr);
  if (size <= 0) {
    return string(fallback);
  }
  string ret(static_cast<string::size_type>(size), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide, len, ret.data(), size, nullptr, nullptr);
  return ret;
#else
  return string(fallback);
#endif // _MSVC_STL_VERSION
}

} // namespace rocket::chrono

// Part II: time zones ======================================================================================

#ifndef ROCKET_HAS_STD_CHRONO_TIME_ZONE

namespace {

// Local constants ------------------------------------------------------------------------------------------

// The range of time points covered by time zones. This is the range of `year`, so calendar computations
// cannot overflow
constexpr sys_seconds MIN_TIME = sys_days(year::min() / January / 1);
constexpr sys_seconds MAX_TIME = sys_days(year::max() / December / 31);

constexpr string_view DEFAULT_ZONE_INFO_DIR = "/usr/share/zoneinfo";

// The largest time-zone file we read. The largest real ones have a few KiB
constexpr size_t MAX_FILE_SIZE = 1 << 20;

// `LocalType` ----------------------------------------------------------------------------------------------

// A local-time type: the UTC offset, whether it is DST, and the abbreviation
struct LocalType {
  seconds offset {};
  bool dst = false;
  string abbrev;
};

// `PosixRule` ----------------------------------------------------------------------------------------------

// A POSIX TZ string as found in the footer of TZif files, for example `CET-1CEST,M3.5.0,M10.5.0/3`. Offsets
// are UTC offsets, that is, local time minus UTC, unlike in the TZ string
struct PosixRule {
  struct Date {
    enum class Kind {
      Julian1, // `Jn`: 1 <= n <= 365, February 29 is never counted
      Julian0, // `n`: 0 <= n <= 365, February 29 is counted in leap years
      MonthWeekDay // `Mm.w.d`: day `d` (0 = Sunday) of week `w` (5 = last) of month `m`
    };

    Kind kind = Kind::MonthWeekDay;
    i32 day = 0;
    i32 month = 0;
    i32 week = 0;
    i32 weekday = 0;
    seconds time = 2h; // Local time of the transition, may be negative or exceed 24 hours
  };

  LocalType std;
  optional<LocalType> dst;
  Date start; // Start of DST
  Date end; // End of DST
};

// `PosixParser` --------------------------------------------------------------------------------------------

// POSIX TZ strings
struct PosixParser {
  explicit PosixParser(string_view str) : str_(str) {}

  optional<PosixRule>
  parse() {
    PosixRule rule;
    seconds offset;
    if (not parseAbbrev(rule.std.abbrev) || not parseTime(offset)) {
      return nullopt;
    }
    rule.std.offset = -offset;

    if (atEnd()) {
      return rule;
    }

    LocalType dst;
    dst.dst = true;
    if (not parseAbbrev(dst.abbrev)) {
      return nullopt;
    }
    if (not atEnd() && peek() != ',') {
      if (not parseTime(offset)) {
        return nullopt;
      }
      dst.offset = -offset;
    } else {
      dst.offset = rule.std.offset + 1h;
    }
    rule.dst = std::move(dst);

    if (atEnd()) {
      // POSIX leaves the rule implementation-defined. Like glibc, use the US rule
      rule.start = { .kind = PosixRule::Date::Kind::MonthWeekDay, .month = 3, .week = 2 };
      rule.end = { .kind = PosixRule::Date::Kind::MonthWeekDay, .month = 11, .week = 1 };
      return rule;
    }
    if (not consume(',') || not parseDate(rule.start) || not consume(',') || not parseDate(rule.end) ||
        not atEnd()) {
      return nullopt;
    }
    return rule;
  }

private:

  string_view str_;
  size_t pos_ = 0;

  [[nodiscard]] bool atEnd() const { return pos_ == str_.size(); }

  [[nodiscard]] char peek() const { return atEnd() ? '\0' : str_[pos_]; }

  bool
  consume(char c) {
    if (peek() != c) {
      return false;
    }
    ++pos_;
    return true;
  }

  static bool isDigit(char c) { return c >= '0' && c <= '9'; }

  static bool isAlpha(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }

  // `std` or `dst`: three or more letters, or `<...>` with letters, digits, `+`, and `-`
  bool
  parseAbbrev(string& out) {
    out.clear();
    if (consume('<')) {
      while (not atEnd() && peek() != '>') {
        const char c = peek();
        if (not isAlpha(c) && not isDigit(c) && c != '+' && c != '-') {
          return false;
        }
        out.push_back(c);
        ++pos_;
      }
      return consume('>') && out.size() >= 3;
    }
    while (isAlpha(peek())) {
      out.push_back(peek());
      ++pos_;
    }
    return out.size() >= 3;
  }

  bool
  parseNumber(i32& out, i32 max) {
    if (not isDigit(peek())) {
      return false;
    }
    out = 0;
    while (isDigit(peek())) {
      out = out * 10 + (peek() - '0');
      if (out > max) {
        return false;
      }
      ++pos_;
    }
    return true;
  }

  // `[+-]hh[:mm[:ss]]`. RFC 8536 allows hours up to 167, and negative times in rules
  bool
  parseTime(seconds& out) {
    bool negative = false;
    if (consume('-')) {
      negative = true;
    } else {
      consume('+');
    }
    i32 h = 0;
    i32 m = 0;
    i32 s = 0;
    if (not parseNumber(h, 167)) {
      return false;
    }
    if (consume(':')) {
      if (not parseNumber(m, 59)) {
        return false;
      }
      if (consume(':') && not parseNumber(s, 59)) {
        return false;
      }
    }
    out = hours(h) + minutes(m) + seconds(s);
    if (negative) {
      out = -out;
    }
    return true;
  }

  bool
  parseDate(PosixRule::Date& out) {
    using Kind = PosixRule::Date::Kind;
    if (consume('J')) {
      out.kind = Kind::Julian1;
      if (not parseNumber(out.day, 365) || out.day < 1) {
        return false;
      }
    } else if (consume('M')) {
      out.kind = Kind::MonthWeekDay;
      if (not parseNumber(out.month, 12) || out.month < 1 || not consume('.') ||
          not parseNumber(out.week, 5) || out.week < 1 || not consume('.') ||
          not parseNumber(out.weekday, 6)) {
        return false;
      }
    } else {
      out.kind = Kind::Julian0;
      if (not parseNumber(out.day, 365)) {
        return false;
      }
    }
    out.time = 2h;
    if (consume('/')) {
      return parseTime(out.time);
    }
    return true;
  }
};

// Local functions ------------------------------------------------------------------------------------------

// The local time of a transition in year `y`
local_seconds
localTransition(const PosixRule::Date& date, year y) {
  using Kind = PosixRule::Date::Kind;
  local_days day;
  switch (date.kind) {
    case Kind::Julian1:
      day = local_days(y / January / 1) + days(date.day - 1);
      if (y.is_leap() && date.day >= 60) {
        day += days(1);
      }
      break;
    case Kind::Julian0:
      day = local_days(y / January / 1) + days(date.day);
      break;
    case Kind::MonthWeekDay: {
      const auto m = month(static_cast<unsigned>(date.month));
      const auto wd = weekday(static_cast<unsigned>(date.weekday));
      day = date.week == 5 ?
        local_days(y / m / wd[last]) :
        local_days(y / m / wd[static_cast<unsigned>(date.week)]);
      break;
    }
  }
  return day + date.time;
}

sys_seconds
toSys(local_seconds tp, seconds offset) {
  return sys_seconds(tp.time_since_epoch()) - offset;
}

// The period of a rule containing `tp`. Since the rule repeats every year, looking at the transitions of the
// surrounding years suffices
sys_info
ruleInfo(const PosixRule& rule, sys_seconds tp) {
  if (not rule.dst) {
    return { MIN_TIME, MAX_TIME, rule.std.offset, 0min, rule.std.abbrev };
  }
  const LocalType& dst = *rule.dst;
  const i32 y = static_cast<i32>(year_month_day(floor<days>(tp + rule.std.offset)).year());

  // If DST ends exactly when it starts in the next year, as in `AAA-2<+01>-1,0/0,J365/23`, it is in effect
  // all year. If it starts and ends at the same time, it is never in effect

  const auto startOf = [&](i32 i) { return toSys(localTransition(rule.start, year(i)), rule.std.offset); };
  const auto endOf = [&](i32 i) { return toSys(localTransition(rule.end, year(i)), dst.offset); };
  const i32 refYear = clamp(y, static_cast<i32>(year::min()), static_cast<i32>(year::max()) - 1);
  if (endOf(refYear) == startOf(refYear + 1)) {
    return { MIN_TIME, MAX_TIME, dst.offset, duration_cast<minutes>(dst.offset - rule.std.offset), dst.abbrev };
  }
  if (startOf(refYear) == endOf(refYear)) {
    return { MIN_TIME, MAX_TIME, rule.std.offset, 0min, rule.std.abbrev };
  }

  struct Change {
    sys_seconds time;
    bool toDst;
  };

  vector<Change> changes;
  const i32 minYear = max(y - 2, static_cast<i32>(year::min()));
  const i32 maxYear = min(y + 2, static_cast<i32>(year::max()));
  for (i32 i = minYear; i <= maxYear; ++i) {
    changes.push_back({ startOf(i), true });
    changes.push_back({ endOf(i), false });
  }

  // At equal times, the start of DST wins

  ranges::sort(changes, [](const Change& lhs, const Change& rhs) {
    return lhs.time != rhs.time ? lhs.time < rhs.time : !lhs.toDst && rhs.toDst;
  });

  // Find the effective changes around `tp`. Changes at the same time are combined

  bool state = not changes.front().toDst;
  sys_seconds begin = MIN_TIME;
  sys_seconds end = MAX_TIME;
  for (size_t i = 0; i < changes.size();) {
    const sys_seconds time = changes[i].time;
    bool newState = state;
    for (; i < changes.size() && changes[i].time == time; ++i) {
      newState = changes[i].toDst;
    }
    if (newState == state) {
      continue;
    }
    if (time > tp) {
      end = time;
      break;
    }
    state = newState;
    begin = time;
  }

  begin = max(begin, MIN_TIME);
  end = min(end, MAX_TIME);
  if (state) {
    return { begin, end, dst.offset, duration_cast<minutes>(dst.offset - rule.std.offset), dst.abbrev };
  }
  return { begin, end, rule.std.offset, 0min, rule.std.abbrev };
}

} // namespace

namespace rocket::chrono {

namespace internal {

// `TimeZoneAccess` -----------------------------------------------------------------------------------------

// Declared in `chrono.h`
struct TimeZoneAccess {
  static const ZoneData&
  data(const time_zone& tz) {
    return *tz.data_;
  }

  static unique_ptr<time_zone>
  make(string name, shared_ptr<const ZoneData> data) {
    return unique_ptr<time_zone>(new time_zone(std::move(name), std::move(data)));
  }
};

// `ZoneData` -----------------------------------------------------------------------------------------------

// Declared in `chrono.h`
struct ZoneData {
  // The type before the first transition
  LocalType initial;
  minutes initialSave {};

  // The transitions, sorted, without redundant ones: `types[i]` and `saves[i]` apply from `times[i]` on
  vector<sys_seconds> times;
  vector<LocalType> types;
  vector<minutes> saves;

  // The rule for time points after `ruleStart`, which is the last transition in the file, including
  // redundant ones, or `MIN_TIME` if the file has no transitions
  optional<PosixRule> rule;
  sys_seconds ruleStart = MIN_TIME;
};

} // namespace internal

} // namespace rocket::chrono

namespace {

// TZif files -----------------------------------------------------------------------------------------------

struct ParseError {};

class Reader {
public:

  explicit Reader(span<const uint8_t> bytes) : bytes_(bytes) {}

  [[nodiscard]] size_t pos() const { return pos_; }

  [[nodiscard]] size_t remaining() const { return bytes_.size() - pos_; }

  void
  skip(size_t n) {
    need(n);
    pos_ += n;
  }

  uint8_t
  u8() {
    need(1);
    return bytes_[pos_++];
  }

  uint32_t
  u32() {
    need(4);
    uint32_t ret = 0;
    for (i32 i = 0; i < 4; ++i) {
      ret = (ret << 8) | bytes_[pos_++];
    }
    return ret;
  }

  int64_t
  time(size_t size) {
    if (size == 4) {
      return static_cast<int32_t>(u32());
    }
    need(8);
    uint64_t ret = 0;
    for (i32 i = 0; i < 8; ++i) {
      ret = (ret << 8) | bytes_[pos_++];
    }
    return static_cast<int64_t>(ret);
  }

  span<const uint8_t>
  bytes(size_t n) {
    need(n);
    auto ret = bytes_.subspan(pos_, n);
    pos_ += n;
    return ret;
  }

private:

  span<const uint8_t> bytes_;
  size_t pos_ = 0;

  void
  need(size_t n) const {
    if (remaining() < n) {
      throw ParseError();
    }
  }
};

struct Header {
  char version = 0;
  uint32_t isutcnt = 0;
  uint32_t isstdcnt = 0;
  uint32_t leapcnt = 0;
  uint32_t timecnt = 0;
  uint32_t typecnt = 0;
  uint32_t charcnt = 0;

  [[nodiscard]] size_t
  dataSize(size_t timeSize) const {
    return timecnt * timeSize + timecnt + typecnt * 6 + charcnt + leapcnt * (timeSize + 4) + isstdcnt + isutcnt;
  }
};

Header
readHeader(Reader& in) {
  const auto magic = in.bytes(4);
  if (magic[0] != 'T' || magic[1] != 'Z' || magic[2] != 'i' || magic[3] != 'f') {
    throw ParseError();
  }
  Header ret;
  ret.version = static_cast<char>(in.u8());
  in.skip(15);
  ret.isutcnt = in.u32();
  ret.isstdcnt = in.u32();
  ret.leapcnt = in.u32();
  ret.timecnt = in.u32();
  ret.typecnt = in.u32();
  ret.charcnt = in.u32();

  // Counts are limited so `dataSize` cannot overflow; real files are far below these limits

  if (ret.typecnt == 0 || ret.typecnt > 256 || ret.charcnt == 0 || ret.charcnt > 65536 ||
      ret.timecnt > 1'000'000 || ret.leapcnt > 100'000 || (ret.isutcnt != 0 && ret.isutcnt != ret.typecnt) ||
      (ret.isstdcnt != 0 && ret.isstdcnt != ret.typecnt)) {
    throw ParseError();
  }
  return ret;
}

// Computes the DST offset of DST periods, which TZif files do not record, from the standard periods before
// and after. If a zone changes its standard time around a DST period, one of them belongs to a different
// standard time, so the more plausible difference wins. After the last transition, the standard period after
// is that of the rule. `types[0]` is the initial type
vector<minutes>
computeSaves(const vector<const LocalType*>& types, const optional<PosixRule>& rule) {
  vector<minutes> ret(types.size(), 0min);
  for (size_t i = 0; i < types.size(); ++i) {
    if (not types[i]->dst) {
      continue;
    }

    optional<seconds> before;
    for (size_t j = i; j-- > 0;) {
      if (not types[j]->dst) {
        before = types[j]->offset;
        break;
      }
    }
    optional<seconds> after;
    for (size_t j = i + 1; j < types.size(); ++j) {
      if (not types[j]->dst) {
        after = types[j]->offset;
        break;
      }
    }
    if (not after && rule) {
      after = rule->std.offset;
    }

    // Prefer positive saves, then multiples of 30 minutes, then smaller ones. Negative saves only occur
    // where both candidates are negative, as in the winter time of Europe/Dublin

    const auto key = [](minutes m) { return tuple(m < 0min, m % 30min != 0min, abs(m)); };
    minutes save = 0min;
    for (const auto& stdOffset : { after, before }) {
      if (not stdOffset) {
        continue;
      }
      const auto candidate = duration_cast<minutes>(types[i]->offset - *stdOffset);
      if (candidate != 0min && (save == 0min || key(candidate) < key(save))) {
        save = candidate;
      }
    }
    ret[i] = save != 0min ? save : 60min; // DST must have a nonzero save
  }
  return ret;
}

shared_ptr<const ZoneData>
parseTzif(span<const uint8_t> bytes) {
  Reader in(bytes);
  Header header = readHeader(in);
  size_t timeSize = 4;

  // Version 2 and later repeat the data with 64-bit times, followed by a footer. Readers must use these

  if (header.version >= '2') {
    in.skip(header.dataSize(4));
    header = readHeader(in);
    timeSize = 8;
  }
  if (header.leapcnt != 0) {
    throw ParseError(); // Leap seconds are not supported
  }

  // Data block

  vector<int64_t> rawTimes(header.timecnt);
  for (auto& t : rawTimes) {
    t = in.time(timeSize);
  }
  vector<uint8_t> rawIndices(header.timecnt);
  for (auto& i : rawIndices) {
    i = in.u8();
    if (i >= header.typecnt) {
      throw ParseError();
    }
  }
  struct TtInfo {
    int32_t utoff;
    bool isDst;
    uint8_t desigidx;
  };
  vector<TtInfo> ttInfos(header.typecnt);
  for (auto& tt : ttInfos) {
    tt.utoff = static_cast<int32_t>(in.u32());
    tt.isDst = in.u8() != 0;
    tt.desigidx = in.u8();
    if (tt.utoff == numeric_limits<int32_t>::min() || tt.desigidx >= header.charcnt) {
      throw ParseError();
    }
  }
  const auto chars = in.bytes(header.charcnt);
  in.skip(header.isstdcnt + header.isutcnt);

  vector<LocalType> localTypes;
  for (const auto& tt : ttInfos) {
    const auto* begin = reinterpret_cast<const char*>(chars.data()) + tt.desigidx;
    const auto* end = reinterpret_cast<const char*>(chars.data()) + chars.size();
    const auto* nul = find(begin, end, '\0');
    if (nul == end) {
      throw ParseError();
    }
    localTypes.push_back({ seconds(tt.utoff), tt.isDst, string(begin, nul) });
  }

  // Footer

  optional<PosixRule> rule;
  if (header.version >= '2') {
    if (in.u8() != '\n') {
      throw ParseError();
    }
    string footer;
    for (char c = static_cast<char>(in.u8()); c != '\n'; c = static_cast<char>(in.u8())) {
      footer.push_back(c);
    }
    if (not footer.empty()) {
      rule = PosixParser(footer).parse(); // An invalid footer is ignored
    }
  }

  // Build the zone data. Local time before the first transition is given by the first type (RFC 8536). Times
  // outside the supported range are moved to its bounds

  auto data = make_shared<ZoneData>();
  data->initial = localTypes[0];
  data->rule = std::move(rule);

  int64_t prev = numeric_limits<int64_t>::min();
  for (size_t i = 0; i < rawTimes.size(); ++i) {
    if (rawTimes[i] <= prev) {
      throw ParseError(); // Must be ascending
    }
    prev = rawTimes[i];

    const LocalType& type = localTypes[rawIndices[i]];
    if (rawTimes[i] <= MIN_TIME.time_since_epoch().count()) {
      data->initial = type;
      continue;
    }
    if (rawTimes[i] >= MAX_TIME.time_since_epoch().count()) {
      break;
    }
    const sys_seconds time { seconds(rawTimes[i]) };
    data->ruleStart = time;

    // Skip redundant transitions. A transition that only changes the DST flag is redundant, too; some files
    // contain these where the zone was converted from negative DST. The period then counts as standard time

    LocalType& current = data->types.empty() ? data->initial : data->types.back();
    if (type.offset == current.offset && type.abbrev == current.abbrev) {
      current.dst = current.dst && type.dst;
      continue;
    }
    data->times.push_back(time);
    data->types.push_back(type);
  }

  vector<const LocalType*> allTypes { &data->initial };
  for (const auto& type : data->types) {
    allTypes.push_back(&type);
  }
  auto saves = computeSaves(allTypes, data->rule);
  data->initialSave = saves[0];
  data->saves.assign(saves.begin() + 1, saves.end());

  return data;
}

shared_ptr<const ZoneData>
makeUtcData() {
  auto data = make_shared<ZoneData>();
  data->initial = { 0s, false, "UTC" };
  return data;
}

// Time-zone database ---------------------------------------------------------------------------------------

constexpr string_view UtcName = "Etc/UTC";
constexpr string_view UtcAliases[] = { "UTC", "Etc/UCT", "UCT", "Etc/Universal", "Universal", "Etc/Zulu", "Zulu" };

bool
isValidName(string_view name) {
  if (name.empty() || name.size() > 255 || name.front() == '/' || name.back() == '/') {
    return false;
  }
  if (name.find_first_of(string_view("\\\0", 2)) != string_view::npos) {
    return false;
  }
  size_t pos = 0;
  while (pos <= name.size()) {
    const size_t next = min(name.find('/', pos), name.size());
    const string_view component = name.substr(pos, next - pos);
    if (component.empty() || component == "." || component == "..") {
      return false;
    }
    pos = next + 1;
  }
  return true;
}

optional<vector<uint8_t>>
readFile(const filesystem::path& path) {
  error_code ec;
  if (not filesystem::is_regular_file(path, ec)) {
    return nullopt;
  }
  const auto size = filesystem::file_size(path, ec);
  if (ec || size > MAX_FILE_SIZE) {
    return nullopt;
  }
  ifstream in(path, ios::binary);
  if (not in) {
    return nullopt;
  }
  vector<uint8_t> ret(size);
  in.read(reinterpret_cast<char*>(ret.data()), static_cast<streamsize>(size));
  if (in.gcount() != static_cast<streamsize>(size)) {
    return nullopt;
  }
  return ret;
}

class Database {
public:

  Database() {
    const char* dir = getenv("TZDIR"); // NOLINT(*-mt-unsafe)
    dir_ = dir != nullptr && *dir != '\0' ? filesystem::path(dir) : filesystem::path(DEFAULT_ZONE_INFO_DIR);
    loadLinks();
  }

  const time_zone*
  locate(string_view name) {
    ROCKET_MUTEX_LOCK(mutex_);

    if (auto it = byName_.find(name); it != byName_.end()) {
      return it->second;
    }
    if (not isValidName(name)) {
      return nullptr;
    }

    // Resolve links. Links point to zones, but be robust against chains and cycles

    string canonical(name);
    for (i32 i = 0; i < 8; ++i) {
      auto it = links_.find(canonical);
      if (it == links_.end()) {
        break;
      }
      canonical = it->second;
    }

    const time_zone* ret = nullptr;
    if (auto it = byName_.find(canonical); it != byName_.end()) {
      ret = it->second;
    } else {
      auto data = load(canonical);
      if (data == nullptr) {
        return nullptr;
      }
      zones_.push_back(TimeZoneAccess::make(canonical, std::move(data)));
      ret = zones_.back().get();
      byName_.emplace(canonical, ret);
    }
    byName_.emplace(string(name), ret);
    return ret;
  }

  [[nodiscard]] const filesystem::path& dir() const { return dir_; }

private:

  mutex mutex_;
  filesystem::path dir_;
  map<string, string, less<>> links_; // Link name to target
  vector<unique_ptr<time_zone>> zones_;
  map<string, const time_zone*, less<>> byName_; // Zone and link names to zones

  // `tzdata.zi` lists links as `L TARGET LINK`
  void
  loadLinks() {
    ifstream in(dir_ / "tzdata.zi");
    string line;
    while (in && getline(in, line)) {
      if (not line.starts_with("L ")) {
        continue;
      }
      const auto sep = line.find(' ', 2);
      if (sep == string::npos) {
        continue;
      }
      string target = line.substr(2, sep - 2);
      string link = line.substr(sep + 1);
      while (not link.empty() && (link.back() == '\r' || link.back() == ' ')) {
        link.pop_back();
      }
      if (not target.empty() && not link.empty()) {
        links_.emplace(std::move(link), std::move(target));
      }
    }

    // Make sure the UTC aliases resolve even without `tzdata.zi`

    for (const auto alias : UtcAliases) {
      links_.emplace(string(alias), string(UtcName));
    }
  }

  shared_ptr<const ZoneData>
  load(const string& name) {
    if (auto bytes = readFile(dir_ / name)) {
      try {
        return parseTzif(*bytes);
      } catch (const ParseError&) {
        return nullptr;
      }
    }

    // Without time-zone files, UTC must still be available

    if (name == UtcName) {
      return makeUtcData();
    }
    return nullptr;
  }
};

Database&
database() {
  static auto* ret = new Database(); // Never destroyed, so time zones outlive static objects
  return *ret;
}

// Extracts the zone name from a path into a time-zone directory, for example
// `/var/db/timezone/zoneinfo/Europe/Berlin` (macOS) or `../usr/share/zoneinfo/Europe/Berlin` (Linux)
optional<string>
zoneNameFromPath(string_view path) {
  const auto& dir = database().dir().generic_string();
  if (path.starts_with(dir) && path.size() > dir.size() + 1 && path[dir.size()] == '/') {
    return string(path.substr(dir.size() + 1));
  }
  constexpr string_view Marker = "zoneinfo/";
  const auto pos = path.rfind(Marker);
  if (pos == string_view::npos) {
    return nullopt;
  }
  string_view name = path.substr(pos + Marker.size());
  if (name.starts_with("posix/")) {
    name.remove_prefix(6);
  }
  return string(name);
}

} // namespace

namespace rocket::chrono {

// `time_zone` ----------------------------------------------------------------------------------------------

sys_info
time_zone::getSysInfo(sys_seconds tp) const {
  const ZoneData& data = *data_;
  tp = clamp(tp, MIN_TIME, MAX_TIME - 1s);

  // After the last transition in the file, the rule applies. Its first period continues the last
  // transition, which is its begin

  if (data.rule && tp >= data.ruleStart) {
    sys_info info = ruleInfo(*data.rule, tp);
    if (info.begin <= data.ruleStart) {
      info.begin = data.times.empty() ? MIN_TIME : data.times.back();
    }
    return info;
  }

  const auto it = ranges::upper_bound(data.times, tp);
  const auto nextEnd = [&](size_t next) {
    if (next < data.times.size()) {
      return data.times[next];
    }
    return data.rule ? ruleInfo(*data.rule, data.ruleStart).end : MAX_TIME;
  };

  if (it == data.times.begin()) {
    return { MIN_TIME, nextEnd(0), data.initial.offset, data.initialSave, data.initial.abbrev };
  }
  const auto i = static_cast<size_t>(it - data.times.begin() - 1);
  const LocalType& type = data.types[i];
  return { data.times[i], nextEnd(i + 1), type.offset, data.saves[i], type.abbrev };
}

local_info
time_zone::getLocalInfo(local_seconds tp) const {
  // UTC offsets are below 26 hours (RFC 8536), so the periods that may contain `tp` start within 2 days

  constexpr auto Window = days(2);
  const sys_seconds t0(tp.time_since_epoch());
  const sys_seconds probeBegin = t0 < MIN_TIME + Window ? MIN_TIME : t0 - Window;
  const sys_seconds probeEnd = t0 > MAX_TIME - Window ? MAX_TIME : t0 + Window;

  vector<sys_info> periods;
  for (sys_info info = getSysInfo(probeBegin);; info = getSysInfo(info.end)) {
    const bool last = info.end >= probeEnd || info.end >= MAX_TIME;
    periods.push_back(std::move(info));
    if (last) {
      break;
    }
  }

  const auto contains = [&](const sys_info& info) {
    const sys_seconds utc = t0 - info.offset;
    return info.begin <= utc && utc < info.end;
  };

  local_info ret {};
  bool found = false;
  for (const auto& info : periods) {
    if (not contains(info)) {
      continue;
    }
    if (not found) {
      found = true;
      ret.result = local_info::unique;
      ret.first = info;
    } else {
      ret.result = local_info::ambiguous;
      ret.second = info;
      return ret;
    }
  }
  if (found) {
    return ret;
  }

  // No period contains `tp`: it is in a gap

  for (size_t i = 0; i + 1 < periods.size(); ++i) {
    if (t0 - periods[i].offset >= periods[i].end && t0 - periods[i + 1].offset < periods[i + 1].begin) {
      ret.result = local_info::nonexistent;
      ret.first = periods[i];
      ret.second = periods[i + 1];
      return ret;
    }
  }

  // Unreachable for consistent data; return the period by UTC

  ret.result = local_info::unique;
  ret.first = getSysInfo(t0);
  return ret;
}

// Functions ------------------------------------------------------------------------------------------------

const time_zone*
current_zone() {
  auto& db = database();

  // `/etc/localtime` is usually a symbolic link into the time-zone directory

  error_code ec;
  const auto target = filesystem::read_symlink("/etc/localtime", ec);
  if (not ec) {
    if (auto zoneName = zoneNameFromPath(target.generic_string())) {
      if (const auto* ret = db.locate(*zoneName)) {
        return ret;
      }
    }
  }

  // Debian and derivatives write the name to `/etc/timezone`

  ifstream in("/etc/timezone");
  string line;
  if (in && getline(in, line)) {
    while (not line.empty() && isspace(static_cast<u8>(line.back()))) {
      line.pop_back();
    }
    if (const auto* ret = db.locate(line)) {
      return ret;
    }
  }

  return locate_zone("UTC");
}

const time_zone*
locate_zone(string_view name) {
  if (const auto* ret = database().locate(name)) {
    return ret;
  }
  throw runtime_error(fmt::format("`rocket::chrono::locate_zone`: cannot locate zone: {}", name));
}

} // namespace rocket::chrono

#endif // ROCKET_HAS_STD_CHRONO_TIME_ZONE

// EOF
