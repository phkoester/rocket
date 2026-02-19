/**
 * @file Declared.h
 *
 * C++ reflection support: declared types.
 */

#pragma once

#include <type_traits>

namespace rocket::reflect {

// #Declared ------------------------------------------------------------------------------------------------

/**
 * This template provides access to default member references of a declared type.
 */
template<typename T>
struct Declared : std::false_type {};

} // namespace rocket::reflect

// EOF
