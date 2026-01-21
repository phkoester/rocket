/*
 * @file version.h
 */

#pragma once

#define ROCKET_VERSION_INFO "0.2.0" ///< SemVer string.

#define ROCKET_VERSION_MAJOR 0UL ///< Major.
#define ROCKET_VERSION_MINOR 2UL ///< Minor.
#define ROCKET_VERSION_PATCH 0UL ///< Patch.

/// Comparable version value.
#define ROCKET_VERSION ( \
    ROCKET_VERSION_MAJOR * 1'000'000UL + \
    ROCKET_VERSION_MINOR * 1'000UL + \
    ROCKET_VERSION_PATCH)

// EOF
