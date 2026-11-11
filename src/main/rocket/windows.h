/*
 * windows.h
 *
 * Windows-specific definitions.
 */

#pragma once

#ifdef ROCKET_OS_WINDOWS

#define NOMINMAX

#ifndef ROCKET_WINDOWS_FULL
  #define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

#endif // ROCKET_OS_WINDOWS

// EOF
