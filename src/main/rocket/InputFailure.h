/**
 * @file InputFailure.h
 *
 * An input-failure exception carrying an optional position and, optionally, ranges.
 */

#pragma once

#include "rocket/Exception.h"
#include "rocket/str/Range.h"

#include <optional>

namespace rocket {

// `InputFailure` -------------------------------------------------------------------------------------------

/**
 * Instances of this class are thrown when reading a string input fails.
 *
 * The position is empty if it is unknown, e.g., because the input is read from a source that cannot report
 * its position. Ranges are only stored together with a position: an instance without a position never has
 * ranges.
 */
struct InputFailure : InvalidState {
  /**
   * Creates an instance with the range @f$[a,b)@f$, if @p position, @p a, and @p b are all known. Otherwise,
   * the instance has no ranges.
   *
   * @param position the position to store, or an empty optional if the position is unknown
   * @param a the lower bound of the range, or an empty optional if it is unknown
   * @param b the upper bound of the range, or an empty optional if it is unknown
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   * @return the new instance
   */
  [[nodiscard]] static InputFailure
  withRange(
    std::optional<u64> position,
    std::optional<u64> a,
    std::optional<u64> b,
    std::string_view msg,
    const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
    const std::optional<stacktrace>& st = ROCKET_EXCEPTION_ST) {
    if (position && a && b) {
      return { position, { { *a, *b } }, msg, sl, st };
    }
    return { position, msg, sl, st };
  }

  /**
   * @ctor
   *
   * @param position the position to store, or an empty optional if the position is unknown
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  InputFailure(
    std::optional<u64> position,
    std::string_view msg,
    const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
    const std::optional<stacktrace>& st = ROCKET_EXCEPTION_ST) :
    InputFailure(position, {}, msg, sl, st) {}

  /**
   * @ctor
   *
   * @param position the position to store, or an empty optional if the position is unknown
   * @param ranges the ranges to store. This must be empty if @p position is empty
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   * @throw #rocket::InvalidArgument if @p position is empty, but @p ranges is not
   */
  InputFailure(
    std::optional<u64> position,
    std::initializer_list<str::Range> ranges,
    std::string_view msg,
    const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
    const std::optional<stacktrace>& st = ROCKET_EXCEPTION_ST) :
    InvalidState(msg, sl, st),
    position_(position),
    ranges_(ranges) {
    if (not position_ && not ranges_.empty()) {
      throw InvalidArgument("ranges", "Must be empty if there is no position", sl, st);
    }
  }

  ~InputFailure() override = default;

  /**
   * Returns the stored position.
   *
   * @return the stored position, or an empty optional if the position is unknown
   */
  [[nodiscard]] std::optional<u64> position() const { return position_; }

  /**
   * Returns the stored position ranges.
   *
   * @return the stored position ranges, which are empty if the position is unknown
   */
  [[nodiscard]] const str::Ranges& ranges() const { return ranges_; }

  /**
   * Returns a copy of this instance with the position and all range bounds translated by a function.
   *
   * If @p fn cannot translate the position or a range bound, i.e., returns an empty optional, the copy has
   * neither a position nor ranges.
   *
   * @tparam F the function type
   * @param fn the function, which maps a position to the translated position. It may return a #u64 or a
   *   `std::optional<u64>`
   * @return the translated copy
   */
  template<typename F>
  [[nodiscard]] InputFailure
  mapped(const F& fn) const {
    InputFailure ret(*this);
    if (not position_) {
      return ret; // Nothing to translate; there are no ranges either
    }

    bool known = true;
    const auto map = [&fn, &known](u64 pos) -> u64 {
      const std::optional<u64> ret = fn(pos);
      if (not ret) {
        known = false;
        return 0;
      }
      return *ret;
    };

    ret.position_ = map(*position_);
    for (auto& range : ret.ranges_) {
      range.a = map(range.a);
      if (range.b) {
        *range.b = map(*range.b);
      }
    }
    if (not known) {
      ret.position_.reset();
      ret.ranges_.clear();
    }
    return ret;
  }

  /**
   * Returns a copy of this instance with the position and all ranges moved by an offset.
   *
   * This translates a failure in a substring to the enclosing string. If @p offset is empty, i.e., the
   * position of the substring is unknown, the copy has neither a position nor ranges.
   *
   * @param offset the offset to add, or an empty optional if it is unknown
   * @return the moved copy
   */
  [[nodiscard]] InputFailure
  shifted(std::optional<u64> offset) const {
    return mapped([offset](u64 position) -> std::optional<u64> {
      if (not offset) {
        return std::nullopt;
      }
      return position + *offset;
    });
  }

private:

  std::optional<u64> position_;
  str::Ranges ranges_;
};

} // namespace rocket

// EOF
