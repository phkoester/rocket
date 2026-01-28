/**
 * @file terminal.h
 *
 * Terminal utilities.
 */

#pragma once

#include "rocket/rocket.h"
#include "rocket/nio/nio-fwd.h"

#include <optional>
#include <string>

namespace rocket::system::terminal {

// #Ansi ----------------------------------------------------------------------------------------------------

/**
 * ANSI escape sequences.
 */
struct Ansi {
  /**
   * @ctor
   *
   * @param active if `true`, then ANSI escape sequences are generated, otherwise empty strings.
   */
  explicit Ansi(bool active) : active_(active) {}

  /**
   * Clears the screen, including the back log.
   *
   * @return an ANSI escape sequence if this instance is active, otherwise an empty string
   */
  std::string clear() const;

  /**
   * Moves the cursor down by @p n lines.
   *
   * @param n the number of lines to move
   * @return an ANSI escape sequence if this instance is active, otherwise an empty string
   */
  std::string down(i32 n) const;

  /**
   * Moves the cursor left by @p n columns.
   *
   * @param n the number of columns to move
   * @return an ANSI escape sequence if this instance is active, otherwise an empty string
   */
  std::string left(i32 n) const;

  /**
   * Moves the cursor to line @p line and column @p column.
   *
   * @param column the column to move to, starting with 1
   * @param line the line to move to, starting with 1
   * @return an ANSI escape sequence if this instance is active, otherwise an empty string
   */
  std::string move(i32 column, i32 line) const;

 /**
   * Moves the cursor right by @p n columns.
   *
   * @param n the number of columns to move
   * @return an ANSI escape sequence if this instance is active, otherwise an empty string
   */
  std::string right(i32 n) const;

  /**
   * Moves the cursor down by @p n lines.
   *
   * @param n the number of lines to move
   * @return an ANSI escape sequence if this instance is active, otherwise an empty string
   */
  std::string up(i32 n) const;

private:

  bool active_;
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Returns the terminal's current cursor position, if available. The pair' s `first` is the column starting
 * with 1, `second` is the line starting with 1.
 *
 * @param out the sink to write to
 * @return the cursor position, or null if not available
 */
std::optional<std::pair<u64, u64>> position(nio::Sink& out);

/**
 * Returns the terminal's current size, if available. The pair' s `first` is the width in columns, `second`
 * is the height in lines.
 *
 * @param io #rocket::nio::Io object
 * @return the terminal size, or null if not available
 */
std::optional<std::pair<u64, u64>> size(nio::Io& io);

} // namespace rocket::system::terminal

// EOF
