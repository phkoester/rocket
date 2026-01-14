/**
 * @file message.h
 *
 * Common, predefined messages.
 */

#pragma once

#include "rocket/Type.h"

#include <optional>
#include <source_location>
#include <string>

namespace rocket::str::message {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Makes a message saying the input @p input cannot be parsed as a value of type @p type.
 *
 * @param input the input
 * @param type a #rocket::Type value
 * @return a message
 */
std::string cannotParseAs(std::string_view input, const Type& type);

/**
 * Makes a message saying there is an overflow of type @p type.
 *
 * @param type the type
 * @param msg additional message
 * @return a message
 */
std::string overflow(const Type& type, std::string_view msg = "");

/**
 * If available, prepends a source location to a message.
 *
 * @param msg a message
 * @param sl the source location, or null if not available
 * @return a message
 */
std::string withSourceLocation(std::string_view msg, const std::optional<std::source_location>& sl);

} // namespace rocket::str::message

// EOF
