/*
 * icu.h
 *
 * ICU-specific definitions.
 */

#pragma once

#if defined(ROCKET_ICU_CMAKE) || defined(ROCKET_OS_LINUX) || defined(ROCKET_OS_MACOS)
  #include <unicode/ubrk.h>
  #include <unicode/ucal.h>
  #include <unicode/uchar.h>
  #include <unicode/uloc.h>
  #include <unicode/ustring.h>
  #include <unicode/utext.h>
  #include <unicode/utf16.h>
  #include <unicode/utf8.h>
  #include <unicode/utypes.h>
  #include <unicode/uversion.h>
#elif defined(ROCKET_OS_WINDOWS)
  #include "windows.h" // Must come first, defines `NTDDI_VERSION`
  #include <icu.h>
#endif

// EOF
