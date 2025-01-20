/**
 * @file terminal.h
 *
 * Terminal utilities.
 */

#pragma once

#include <optional>
#include <string>

namespace rocket::terminal {

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
   * @return an ANSI escape sequence if the output stream this instance was constructed with is connected to
   *     a terminal, otherwise an empty string
   */
  std::string clear() const;

  /**
   * Moves the cursor down by @p n lines.
   *
   * @param n the number of lines to move
   * @return an ANSI escape sequence if the output stream this instance was constructed with is connected to
   *     a terminal, otherwise an empty string
   */
  std::string down(int n) const;

  /**
   * Moves the cursor left by @p n columns.
   *
   * @param n the number of columns to move
   * @return an ANSI escape sequence if the output stream this instance was constructed with is connected to
   *     a terminal, otherwise an empty string
   */
  std::string left(int n) const;

  /**
   * Moves the cursor to line @p line and column @p column.
   *
   * @param line the line to move to
   * @param column the column to move to
   * @return an ANSI escape sequence if the output stream this instance was constructed with is connected to
   *     a terminal, otherwise an empty string
   */
  std::string move(int line, int column) const;

  /**
   * Moves the cursor right by @p n columns.
   *
   * @param n the number of columns to move
   * @return an ANSI escape sequence if the output stream this instance was constructed with is connected to
   *     a terminal, otherwise an empty string
   */
  std::string right(int n) const;

  /**
   * Sets the foreground style.
   *
   * @param fg a bit mask for the foreground style
   * @return an ANSI escape sequence if the output stream this instance was constructed with is connected to
   *     a terminal, otherwise an empty string
   */
  std::string style(int fg = 0) const;

  /**
   * Sets the foreground and background style.
   *
   * @param fg a bit mask for the foreground style
   * @param bg a bit mask for the background style
   * @return an ANSI escape sequence if the output stream this instance was constructed with is connected to
   *     a terminal, otherwise an empty string
   */
  std::string style(int fg, int bg) const;

  /**
   * Moves the cursor down by @p n lines.
   *
   * @param n the number of lines to move
   * @return an ANSI escape sequence if the output stream this instance was constructed with is connected to
   *     a terminal, otherwise an empty string
   */
  std::string up(int n) const;

private:

  bool active_;
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Returns the terminal size, if available. The pair' s `first` is the width in columns, `second` the height
 * in lines.
 *
 * @param io the stream. If this is `std::cin`, `std::cout`, or `std::cerr` connected to a terminal, then a
 *     proper size is returned
 * @return the terminal size, or null if not available
 */
std::optional<std::pair<size_t, size_t>> size(const std::basic_ios<char>& io);

} // namespace rocket::terminal

// EOF
