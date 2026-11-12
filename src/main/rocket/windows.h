/*
 * windows.h
 *
 * Windows-specific definitions.
 */

#pragma once

#define NOMINMAX

#ifndef ROCKET_WINDOWS_FULL
  #define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

// EOF
