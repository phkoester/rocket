/**
 * @file typeinfo.h
 *
 * Provides #std::type_info in a portable way.
 */

#pragma once

#include "rocket/rocket.h"

#include <typeinfo>

// `std::type_info` ----------------------------------------------------------------------------------------

#ifdef ROCKET_OS_WINDOWS

namespace std {

/**
 * On Windows, `type_info` is in the global namespace, so we need to using-declare it here to make it
 * available as #std::type_info.
 */
// XXX using ::type_info;

} // namespace std

#endif

// EOF
