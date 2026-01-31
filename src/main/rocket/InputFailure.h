/**
 * @file InputFailure.h
 *
 * An input-failure exception carrying a position and, optionally, ranges.
 */

#pragma once

#include "rocket/Exception.h"
#include "rocket/str/Range.h"

namespace rocket {

// #InputFailure --------------------------------------------------------------------------------------------

/**
 * Instances of this class are thrown when reading a string input fails.
 */
struct InputFailure : InvalidState {
  /**
   * @ctor
   *
   * @param position the position to store
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  InputFailure(
      u64 position,
      std::string_view msg,
      const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
      const std::optional<std::stacktrace>& st = ROCKET_EXCEPTION_ST) :
      InputFailure(position, {}, msg, sl, st) {}

  /**
   * @ctor
   *
   * @param position the position to store
   * @param range the range to store
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  InputFailure(
      u64 position,
      str::Range range,
      std::string_view msg,
      const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
      const std::optional<std::stacktrace>& st = ROCKET_EXCEPTION_ST) :
      InputFailure(position, { range }, msg, sl, st) {}

  /**
   * @ctor
   *
   * @param position the position to store
   * @param ranges the ranges to store
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  InputFailure(
      u64 position,
      std::initializer_list<str::Range> ranges,
      std::string_view msg,
      const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
      const std::optional<std::stacktrace>& st = ROCKET_EXCEPTION_ST) :
      InvalidState(msg, sl, st),
      position_(position),
      ranges_(ranges) {}

  ~InputFailure() override {}

  /**
   * Returns the stored position.
   *
   * @return the stored position
   */
  u64 position() const { return position_; }

  /**
   * Returns the stored position ranges.
   *
   * @return the stored position ranges
   */
  const str::Ranges& ranges() const { return ranges_; }

private:

  const u64 position_;
  const str::Ranges ranges_;
};

} // namespace rocket

// EOF
