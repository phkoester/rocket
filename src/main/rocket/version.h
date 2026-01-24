/*
 * @file version.h
 */

#pragma once

/// @cond undocumented

#define ROCKET_VERSION_NAME "0.3.0" ///< SemVer string.

#define ROCKET_VERSION_MAJOR 0 ///< Major.
#define ROCKET_VERSION_MINOR 3 ///< Minor.
#define ROCKET_VERSION_PATCH 0 ///< Patch.

/// Comparable version value.
#define ROCKET_VERSION ( \
    ROCKET_VERSION_MAJOR * 1'000'000 + \
    ROCKET_VERSION_MINOR * 1'000 + \
    ROCKET_VERSION_PATCH)

/// @endcond

// EOF
