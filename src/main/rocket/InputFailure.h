/**
 * @file InputFailure.
 *
 * An `InputFailure` exception.
 */

#pragma once

#define ROCKET_EXCEPT_H

#include "rocket/Exception.h"
#include "rocket/str/Range.h"

namespace rocket {

// `InputFailure` -------------------------------------------------------------------------------------------

/**
 * Instances of this class are thrown when reading a string input fails.
 */
struct InputFailure : InvalidState {
  /// @type_base
  using Base = InvalidState;

  /**
   * @ctor
   *
   * @param position the position to store
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  InputFailure(
      size_t position,
      std::string_view msg,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST) :
      InputFailure(position, {}, msg, std::move(sl), std::move(st)) {}

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
      size_t position,
      str::Range range,
      std::string_view msg,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST) :
      InputFailure(position, { range }, msg, std::move(sl), std::move(st)) {}

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
      size_t position,
      std::initializer_list<str::Range> ranges,
      std::string_view msg,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST) :
      Base(msg, std::move(sl), std::move(st)),
      position_(position),
      ranges_(ranges) {}

  /// @dtor
  virtual ~InputFailure() = default;

  /**
   * Returns the stored position.
   *
   * @return the stored position
   */
  size_t position() const { return position_; }

  /**
   * Returns the stored position ranges.
   *
   * @return the stored position ranges
   */
  const str::Ranges& ranges() const { return ranges_; }

private:

  const size_t position_;
  const str::Ranges ranges_;
};

} // namespace rocket

// EOF
