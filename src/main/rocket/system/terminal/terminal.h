/**
 * @file terminal.h
 *
 * Terminal utilities.
 */

#pragma once

#include "rocket/nio/nio-fwd.h"

#include <optional>
#include <string>

namespace rocket::system::terminal {

// `Style` --------------------------------------------------------------------------------------------------

/**
 * An enum for terminal colors and styles.
 */
enum Style {
  black = 30, ///< Black.
  red, ///< Red.
  green, ///< Green.
  yellow, ///< Yellow.
  blue, ///< Blue.
  magenta, ///< Magenta.
  cyan, ///< Cyan.
  white, ///< White.

  bold      = 2 <<  9, ///< Bold modifier (1,024).
  high      = 2 << 10, ///< High-intensity modifier (2,048).
  underline = 2 << 11  ///< Underline modifier (4,096).
};

// `Ansi` ---------------------------------------------------------------------------------------------------

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
  std::string down(int n) const;

  /**
   * Moves the cursor left by @p n columns.
   *
   * @param n the number of columns to move
   * @return an ANSI escape sequence if this instance is active, otherwise an empty string
   */
  std::string left(int n) const;

  /**
   * Moves the cursor to line @p line and column @p column.
   *
   * @param column the column to move to, starting with 1
   * @param line the line to move to, starting with 1
   * @return an ANSI escape sequence if this instance is active, otherwise an empty string
   */
  std::string move(int column, int line) const;

  /**
   * Sends an ANSI escape sequence to the sink, and returns the response from `stdin`.
   *
   * @param out the sink
   * @param sequence the ANSI escape sequence to send
   * @return the response from `stdin` if this instance is active, otherwise an empty string
   * @throw #rocket::InputFailure if the response from `stdin` cannot be read
   */
  std::string request(nio::Sink& out, std::string_view sequence) const;

  /**
   * Moves the cursor right by @p n columns.
   *
   * @param n the number of columns to move
   * @return an ANSI escape sequence if this instance is active, otherwise an empty string
   */
  std::string right(int n) const;

  /**
   * Sets the foreground style.
   *
   * @param fg a bit mask for the foreground style
   * @return an ANSI escape sequence if this instance is active, otherwise an empty string
   */
  std::string style(int fg = 0) const;

  /**
   * Sets the foreground and background style.
   *
   * @param fg a bit mask for the foreground style
   * @param bg a bit mask for the background style
   * @return an ANSI escape sequence if this instance is active, otherwise an empty string
   */
  std::string style(int fg, int bg) const;

  /**
   * Moves the cursor down by @p n lines.
   *
   * @param n the number of lines to move
   * @return an ANSI escape sequence if this instance is active, otherwise an empty string
   */
  std::string up(int n) const;

private:

  bool active_;
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Returns the terminal's current cursor position, if available. The pair' s `first` is the column starting
 * with 1, `second` is the line starting with 1.
 *
 * @param out the sink. If this is `stdout` or `stderr` connected to a terminal, then a proper position is
 *     returned
 * @return the cursor position, or null if not available
 */
std::optional<std::pair<size_t, size_t>> position(nio::Sink& out);

/**
 * Returns the terminal's current size, if available. The pair' s `first` is the width in columns, `second`
 * is the height in lines.
 *
 * @param out the sink. If this is `stdout` or `stderr` connected to a terminal, then a proper size is
 *     returned
 * @return the terminal size, or null if not available
 */
std::optional<std::pair<size_t, size_t>> size(nio::Sink& out);

} // namespace rocket::system::terminal

// EOF
