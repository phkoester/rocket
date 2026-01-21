/*
 * @file version.h
 */

#pragma once

#define ROCKET_VERSION_NAME "0.2.0" ///< SemVer string.

#define ROCKET_VERSION_MAJOR 0 ///< Major.
#define ROCKET_VERSION_MINOR 2 ///< Minor.
#define ROCKET_VERSION_PATCH 0 ///< Patch.

/// Comparable version value.
#define ROCKET_VERSION ( \
    ROCKET_VERSION_MAJOR * 1'000'000 + \
    ROCKET_VERSION_MINOR * 1'000 + \
    ROCKET_VERSION_PATCH)

// EOF
