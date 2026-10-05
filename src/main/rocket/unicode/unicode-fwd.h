/**
 * @file unicode-fwd.h
 *
 * `unicode` forward declarations.
 */

#pragma once

#include <string>

namespace rocket::unicode {

// `InvalidUnicodePolicy` -----------------------------------------------------------------------------------

/**
 * A policy for handling invalid Unicode, such as invalid UTF-8 byte sequences or invalid UTF-32 code points.
 */
enum InvalidUnicodePolicy {
  /**
   * Throw an exception of type #rocket::InputFailure.
   *
   * This is usually the default behavior.
   */
  Throw,
  /**
   * Continue running.
   *
   * This might involve replacing invalid UTF-8 byte sequences or invalid UTF-32 code points by the
   * replacement character `�` (U+FFFD) or returning a value indicating an error without throwing an
   * exception.
   */
  Continue
};

// `CodePoint` ----------------------------------------------------------------------------------------------

struct CodePoint;

// Functions ------------------------------------------------------------------------------------------------

/**
 * Converts the UTF-8 string @p str to a UTF-32 string.
 *
 * @param str a UTF-8 string
 * @param policy whether to throw or replace if @p str contains invalid UTF-8 byte sequences
 * @return a UTF-32 string
 */
std::u32string convertUtf8To32(std::string_view str, InvalidUnicodePolicy policy = Throw);

/**
 * Converts the UTF-32 string @p str to a UTF-8 string.
 *
 * @param str a UTF-32 string
 * @param policy whether to throw or replace if @p str contains invalid UTF-32 code points
 * @return a UTF-8 string
 */
std::string convertUtf32To8(std::u32string_view str, InvalidUnicodePolicy policy = Throw);

} // namespace rocket::unicode

// EOF
