/**
 * @file message.h
 *
 * Common, predefined messages.
 */

#pragma once

#include "Type.h"

#include <optional>
#include <source_location>
#include <string>

namespace rocket::message {

// Constants ------------------------------------------------------------------------------------------------

constexpr std::string_view SIZE_T_OVERFLOW = "`size_t` overflow";

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
 * Makes a message to be passed to the #rocket::Exception base class.
 *
 * @param msg the exception message
 * @param sl the source location
 * @return a message
 */
std::string exceptionBase(std::string_view msg, const std::optional<std::source_location>& sl);

std::string
iteratorAt(const Type& type, size_t pos, std::string_view msg);

template<typename It>
std::string
iteratorAt(const It& it, size_t pos, std::string_view msg) {
  return iteratorAt(Type::of(it), pos, msg);
}

template<typename It>
std::string
iteratorOutOfBounds(const It& it, size_t pos) {
  return iteratorAt(it, pos, "is out of bounds");
}

/**
 * Makes a message saying there is an overflow for type @p type.
 *
 * @param type a #rocket::Type value
 * @return a message
 */
std::string overflow(const Type& type);

} // namespace rocket::message

// EOF
