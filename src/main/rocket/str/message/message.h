/**
 * @file message.h
 *
 * Common, predefined messages.
 */

#pragma once

#include "rocket/rocket.h"

#include <optional>
#include <source_location>
#include <string>

namespace rocket::str::message {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Makes a message saying the input @p input cannot be scanned as a value of type @p type.
 *
 * @param input the input
 * @param type the type
 * @return a message
 */
std::string cannotScanAs(std::string_view input, const std::type_info& type);

/**
 * Makes a message saying there is an overflow of type @p type.
 *
 * @param type the type
 * @param msg additional message
 * @return a message
 */
std::string overflow(const std::type_info& type, std::string_view msg = "");

/**
 * Makes a message saying there is an underflow of type @p type.
 *
 * @param type the type
 * @param msg additional message
 * @return a message
 */
std::string underflow(const std::type_info& type, std::string_view msg = "");

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
